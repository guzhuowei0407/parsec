#!/usr/bin/env python

""" PURE PYTHON interface to the PaRSEC Trace Tables generated by pbt2ptt.

The recommended shorthand for the name is "PTT".
Therefore, the preferred import method is "import parsec_trace_tables as ptt"

This module is especially suitable for use when a separate Python program
has done the reading of the trace and has stored the trace in this format
in some sort of cross-process format. The format natively supported by pandas
and PTT is HDF5, and can be easily written to and read from using the functions
to_hdf and from_hdf.
"""

import sys
import os
import re
import glob
import shutil
import numpy as np
import pandas as pd
from common_utils import *

import warnings # because these warnings are annoying, and I can find no way around them.
warnings.filterwarnings('ignore', category=pd.io.pytables.PerformanceWarning)
warnings.simplefilter(action = "ignore", category = FutureWarning)

default_descriptors = ['hostname', 'exe', 'ncores', 'N', 'NB', 'sched', 'gflops']

class ParsecTraceTables(object):
    class_version = 1.0 # created 2013.10.22 after move to pandas
    basic_event_columns = ['node_id', 'thread_id',  'handle_id', 'type',
                           'begin', 'end', 'duration', 'flags', 'id']
    HDF_TOP_LEVEL_NAMES = ['event_types', 'event_names', 'event_attributes',
                           'nodes', 'threads', 'information', 'errors']

    # the init function should not ordinarily be used
    # it is better to use from_hdf(), from_native(), or autoload()
    def __init__(self, events, event_types, event_names, event_attributes,
                 nodes, threads, information, errors):
        self.__version__ = self.__class__.class_version
        # core data
        self.events = events
        self.event_types = event_types
        self.event_names = event_names
        self.event_attributes = event_attributes
        self.nodes = nodes
        self.threads = threads
        self.information = information
        self.errors = errors
        # metadata
        self.basic_columns = ParsecTraceTables.basic_event_columns

    def to_hdf(self, filename, table=False, append=False, overwrite=True,
               complevel=0, complib='blosc'):
        if not overwrite and os.path.exists(filename):
            return False
        store = pd.HDFStore(filename + '.tmp', 'w')
        for name in ParsecTraceTables.HDF_TOP_LEVEL_NAMES:
            store.put(name, self.__dict__[name])
        store.put('events', self.events, table=table, append=append)
        store.close()
        # do atomic move once it's finished writing,
        # so as to allow Ctrl-Cs without secret breakage
        shutil.move(filename + '.tmp', filename)
        return True

    # this allows certain 'acceptable' attribute abbreviations
    # and automatically searches the 'information' dictionary
    def __getattr__(self, name):
        try:
            return nice_val(self.information, raw_key(self.information, name))
        except:
            return object.__getattribute__(self, name)
    def __getitem__(self, name):
        return self.__getattr__(name)

    def __repr__(self):
        return describe_dict(self.information)

    def name(self, infos=default_descriptors, add_infos=None):
        if not infos:
            infos = []
        if add_infos:
            infos += add_infos
        return describe_dict(self.information, keys=infos, sep='-')

    def unique_name(self):
        infos = ['start_time'] + default_descriptors
        return self.name(infos=infos)

    # use with care - does an eval() on self'user text' when 'user text' starts with '.'
    def filter_events(self, filter_strings):
        events = self.events
        for filter_str in filter_strings:
            key, value = filter_str.split('==')
            if str(value).startswith('.'):
                # do eval
                eval_str = 'self' + str(value)
                value = eval(eval_str)
            events = events[:][events[key] == value]
        return events

    def close(self):
        try:
            self._store.close()
        except:
            pass

    def __del__(self):
        self.close()


def from_hdf(filename, skeleton_only=False, keep_store=False):
    """ Loads a PaRSEC trace from an existing HDF5 format file."""
    store = pd.HDFStore(filename, 'r')
    top_level = list()
    if not skeleton_only:
        events = store['events']
    else:
        events = pd.DataFrame()
    try:
        for name in ParsecTraceTables.HDF_TOP_LEVEL_NAMES:
            top_level.append(store[name])
    except KeyError as ke:
        print(ke)
        print(store['information'])
        raise ke
    trace = ParsecTraceTables(events, *top_level)
    if keep_store:
        trace._store = store
    else:
        trace._store = None
        store.close()
    return trace

### END CORE FUNCTIONALITY
#####################################
### BEGIN UTILITY FUNCTIONS

def raw_key(dict_, name):
    """ Converts a simple key name into the actual key name by PaRSEC rules."""
    try:
        dict_[name]
        return name
    except:
        pass
    try:
        dict_[str(name).upper()]
        return str(name).upper()
    except:
        pass
    try:
        dict_['PARAM_' + str(name).upper()]
        return 'PARAM_' + str(name).upper()
    except:
        return name


def describe_dict(dict_, keys=default_descriptors, sep=' ', key_val_sep=None,
                  include_key=False, key_length=3, val_length=sys.maxint,
                  float_formatter='{:.1f}'):
    description = str()
    used_keys = []
    for key in keys:
        real_key = raw_key(dict_, key)
        try:
            if real_key in used_keys:
                continue # exclude duplicates
            used_keys.append(real_key)
            # get the value before we add the key to the description,
            # in case the key isn't present and we raise an exception
            value = nice_val(dict_, real_key)

            if include_key and key_length > 0:
                description += '{}'.format(key[:key_length].lower())
            if key_val_sep is not None:
                description += str(key) + key_val_sep

            try:
                if '.' in str(value):
                    description += float_formatter.format(value) + sep
                else:
                    description += str(value)[:val_length] + sep
            except:
                description += str(value)[:val_length] + sep
        except KeyError as e:
            pass # key doesn't exist - just ignore
    return description[:-len(sep)] # remove last 'sep'


def nice_val(dict_, key):
    """ Edits return values for common usage."""
    if key == 'exe':
        m = re.match('.*testing_(\w+)', dict_[key])
        return m.group(1)
    if key == 'hostname':
        return dict_[key].split('.')[0]
    else:
        return dict_[key]


def find_trace_sets(traces, on=['cmdline']): #['N', 'M', 'NB', 'MB', 'IB', 'sched', 'exe', 'hostname'] ):
    trace_sets = dict()
    for trace in traces:
        name = ''
        for info in on:
            name += str(trace.__getattr__(info)).replace('/', '') + '_'
        try:
            name = name[:-1]
            trace_sets[name].append(trace)
        except:
            trace_sets[name] = [trace]
    return trace_sets

# Does a best-effort merge on sets of traces.
# Merges only the events, threads, and nodes, along with
# the top-level "information" struct.
# Intended for use after 'find_trace_sets'
# dangerous for use with groups of traces that do not
# really belong to a reasonably-defined set.
# In particular, the event_type, event_name, and event_attributes
# DataFrames are chosen from the first trace in the set - no
# attempt is made to merge them at this time.
def automerge_trace_sets(trace_sets):
    merged_traces = list()
    for p_set in trace_sets:
        merged_trace = p_set[0]
        for trace in p_set[1:]:
            # ADD UNIQUE ID
            #
            # add start time as id to every row in events and threads DataFrames
            # so that it is still possible to 'split' the merged trace
            # based on start_time id, which should differ for every run...
            if trace == p_set[1]:
                start_time_array = np.empty(len(merged_trace.events), dtype=int)
                start_time_array.fill(merged_trace.start_time)
                merged_trace.events['start_time'] = pd.Series(start_time_array)
                merged_trace.threads['start_time'] = pd.Series(
                    start_time_array[:len(merged_trace.threads)])
            start_time_array = np.empty(len(trace.events), dtype=int)
            start_time_array.fill(trace.start_time)
            events = trace.events
            events['start_time'] = pd.Series(start_time_array)
            threads = trace.threads
            threads['start_time'] = pd.Series(start_time_array[:len(threads)])
            # CONCATENATE EVENTS
            merged_trace.events = pd.concat([merged_trace.events, events])
            merged_trace.nodes = pd.concat([merged_trace.nodes, trace.nodes])
            merged_trace.threads = pd.concat([merged_trace.threads, threads])
        merged_trace.information = match_dicts([trace.information for trace in p_set])
        merged_traces.append(merged_trace)
    return merged_traces


dot_prof_regex = re.compile('(\w+).*(\.prof)(.*)-([a-zA-Z0-9]{6})')

def find_h5_conflicts(filenames):
    """ Takes a list of 'prof' filenames and returns any .h5 conflicts.

    The determination is based on the six-char unique identifier
    that usually ends the .prof filenames, which is generated by MPI.
    The id is different for each rank, but since there will only be
    one .h5 for a full distributed trace, the 6-char string from rank 0
    is generally chosen as the string to use in the .h5 name.

    This function only really looks for names that end in that ID + '.h5'.
    It could easily be modified to search for any filename including that
    string that is not one of the filenames passed.

    If None is returned, there were no conflicts found.
    """
    for filename in filenames:
        matches = dot_prof_regex.match(filename)
        if matches:
            unique_six_char_str = matches.group(4)
            h5_conflicts = glob.glob('*' + unique_six_char_str + '.h5')
            if len(h5_conflicts) > 0:
                return h5_conflicts
    return None


old_ptt_core = '.h5-'
pbt_core = '.prof-' # this is here and not in pbt2ptt b/c pure-python things still need to test
ptt_ext = '.h5'

def is_ptt(filename):
    return filename.endswith(ptt_ext) or is_old_ptt(filename)
def is_old_ptt(filename):
    return old_ptt_core in filename
def is_pbt(filename):
    return pbt_core in filename
def get_basic_ptt_name(filename):
    if is_ptt(filename):
        return filename
    return filename.replace(pbt_core, '-') + ptt_ext
