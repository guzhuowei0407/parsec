/*
 * Copyright (c) 2013-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */
/**
 *
 * @file insert_function_internal.h
 *
 * @version 2.0.0
 *
 **/

#ifndef INSERT_FUNCTION_INTERNAL_H_HAS_BEEN_INCLUDED
#define INSERT_FUNCTION_INTERNAL_H_HAS_BEEN_INCLUDED

BEGIN_C_DECLS

#include "parsec/parsec_internal.h"
#include "parsec/data.h"
#include "parsec/data_internal.h"
#include "parsec/datarepo.h"
#include "parsec/data_distribution.h"
#include "parsec/interfaces/superscalar/insert_function.h"

typedef struct parsec_dtd_task_param_s parsec_dtd_task_param_t;
typedef struct parsec_dtd_function_s   parsec_dtd_function_t;

extern int dtd_init; /* flag to indicate whether dtd_init() is called or not */
extern int dump_traversal_info; /* For printing traversal info */
extern int dump_function_info; /* For printing function_structure info */
extern int hashtable_trace_keyin;
extern int hashtable_trace_keyout;

/* To flag the task we are trying to complete as a local one */
#define PARSEC_ACTION_COMPLETE_LOCAL_TASK 0x08000000

/* Macros to figure out offset of paramters attached to a task */
#define GET_HEAD_OF_PARAM_LIST(TASK) (parsec_dtd_task_param_t *) ( ((char *)TASK) + sizeof(parsec_dtd_task_t) + (TASK->super.function->nb_flows * sizeof(parsec_dtd_parent_info_t)) + (TASK->super.function->nb_flows * sizeof(parsec_dtd_descendant_info_t))  + (TASK->super.function->nb_flows * sizeof(parsec_dtd_flow_info_t)) )

#define GET_VALUE_BLOCK(HEAD, PARAM_COUNT) ((char *)HEAD) + PARAM_COUNT * sizeof(parsec_dtd_task_param_t)

#define SET_LAST_ACCESSOR(TILE) TILE->last_user.flow_index  = -1;                       \
                                TILE->last_user.op_type     = -1;                       \
                                TILE->last_user.task        = NULL;                     \
                                TILE->last_user.alive       = TASK_IS_NOT_ALIVE;        \
                                parsec_atomic_unlock(&TILE->last_user.atomic_lock);     \
                                                                                        \
                                TILE->last_writer.flow_index  = -1;                     \
                                TILE->last_writer.op_type     = -1;                     \
                                TILE->last_writer.task        = NULL;                   \
                                TILE->last_writer.alive       = TASK_IS_NOT_ALIVE;      \
                                parsec_atomic_unlock(&TILE->last_writer.atomic_lock);   \

#define READ_FROM_TILE(TO, FROM) TO.task       = FROM.task;                             \
                                 TO.flow_index = FROM.flow_index;                       \
                                 TO.op_type    = FROM.op_type;                          \
                                 TO.alive      = FROM.alive;                            \


#define LOCAL_DATA 200 /* function_id is uint8_t */

#define TASK_IS_ALIVE       (uint8_t)1
#define TASK_IS_NOT_ALIVE   (uint8_t)0

#define FLUSHED 1
#define NOT_FLUSHED  0

#define DATA_IS_LOCAL  0
#define DATA_IS_FLOATING  1
#define DATA_IS_BEING_TRACKED  2
#define INVALIDATED  3

#define MAX_RANK_INFO 32

/* Structure used to pack arguments of insert_task() */
struct parsec_dtd_task_param_s {
    void                    *pointer_to_tile;
    parsec_dtd_task_param_t *next;
};

typedef void (parsec_handle_wait_t)( parsec_context_t *context, parsec_handle_t *parsec_handle );

#define SUCCESSOR_ITERATED (1<<0)
#define TASK_INSERTED (1<<1)
#define DATA_RELEASED (1<<2)
#define RELEASE_REMOTE_DATA (1<<3)

/*
 * Contains info about each flow of a task
 * We assume each task will have at most MAX_FLOW
 * number of flows.
 */
typedef struct parsec_dtd_min_flow_info_s {
    int16_t  arena_index;
    int      op_type;  /* Operation type on the data */
    int      flags;
    /*
    0 successor_iterated;
    1 task_inserted;
    2 data_released;
    3 release remote data
    */
    parsec_dtd_tile_t *tile;
} parsec_dtd_min_flow_info_t;

typedef struct parsec_dtd_flow_info_s {
    int16_t  arena_index;
    int      op_type;  /* Operation type on the data */
    int      flags;
    /*
    0 successor_iterated;
    1 task_inserted;
    2 data_released;
    3 release remote data
    */
    parsec_dtd_tile_t *tile;
    int rank_sent_to[MAX_RANK_INFO]; /* currently support 1024 nodes */
} parsec_dtd_flow_info_t;

/* All the fields store info about the descendant
 */
typedef struct parsec_dtd_descendant_info_s {
    int                op_type;
    uint8_t            flow_index;
    parsec_dtd_task_t *task;
} parsec_dtd_descendant_info_t;

typedef struct parsec_dtd_parent_info_s {
    uint8_t            op_type;
    uint8_t            flow_index;
    parsec_dtd_task_t *task;
} parsec_dtd_parent_info_t;

#define PARENT_OF(TASK, INDEX) (parsec_dtd_parent_info_t *) ( ((char *)TASK) + sizeof(parsec_dtd_task_t) + (INDEX*sizeof(parsec_dtd_parent_info_t)) )

#define DESC_OF(TASK, INDEX) (parsec_dtd_descendant_info_t *) ( ((char *)TASK) + sizeof(parsec_dtd_task_t) + (TASK->super.function->nb_flows*sizeof(parsec_dtd_parent_info_t)) + (INDEX*sizeof(parsec_dtd_descendant_info_t)) )

#define FLOW_OF(TASK, INDEX) (((TASK->super.parsec_handle->context->my_rank) == (TASK->rank)) ? (parsec_dtd_flow_info_t *) ( ((char *)TASK) + \
                                                        sizeof(parsec_dtd_task_t) + \
                                                       (TASK->super.function->nb_flows*sizeof(parsec_dtd_parent_info_t)) + \
                                                       (TASK->super.function->nb_flows*sizeof(parsec_dtd_descendant_info_t)) + \
                                                       (INDEX*sizeof(parsec_dtd_flow_info_t)) ) : \
                                                       (parsec_dtd_flow_info_t *) ( ((char *)TASK) + \
                                                        sizeof(parsec_dtd_task_t) + \
                                                       (TASK->super.function->nb_flows*sizeof(parsec_dtd_parent_info_t)) + \
                                                       (TASK->super.function->nb_flows*sizeof(parsec_dtd_descendant_info_t)) + \
                                                       (INDEX*sizeof(parsec_dtd_min_flow_info_t)) ))

struct parsec_dtd_task_s {
    parsec_execution_context_t   super;
    int32_t                      rank;
    int32_t                      flow_count;
    /* for testing PTG inserting task in DTD */
    parsec_execution_context_t  *orig_task;
};

/* For creating objects of class parsec_dtd_task_t */
PARSEC_DECLSPEC OBJ_CLASS_DECLARATION(parsec_dtd_task_t);

/** Tile structure **/
typedef struct parsec_dtd_tile_user_s {
    parsec_atomic_lock_t atomic_lock;
    int32_t              op_type;
    int32_t              alive;
    parsec_dtd_task_t   *task;
    uint8_t              flow_index;
}parsec_dtd_tile_user_t;

struct parsec_dtd_tile_s {
    parsec_hashtable_item_t super;
    int16_t                 arena_index;
    int16_t                 flushed;
    int32_t                 rank;
    parsec_data_key_t       key;
    parsec_data_copy_t     *data_copy;
    parsec_ddesc_t         *ddesc;
    parsec_dtd_tile_user_t  last_user;
    parsec_dtd_tile_user_t  last_writer;
};
/* For creating objects of class parsec_dtd_tile_t */
PARSEC_DECLSPEC OBJ_CLASS_DECLARATION(parsec_dtd_tile_t);

/* for testing abstraction for PaRsec */
struct hook_info{
    parsec_hook_t *hook;
};

/**
 * We use one hash table for both remote tasks
 * and remote_deps related to remote tasks.
 */
typedef struct parsec_dtd_two_hash_table_s {
    hash_table       *task_and_rem_dep_h_table;
    parsec_atomic_lock_t atomic_lock;
} parsec_dtd_two_hash_table_t;

typedef struct parsec_dtd_tile_hash_table_s {
    hash_table *tile_h_table;
} parsec_dtd_tile_hash_table_t;

/**
 * internal_parsec_handle
 */
struct parsec_dtd_handle_s {
    parsec_handle_t         super;
    parsec_thread_mempool_t *mempool_owner;
    int                     enqueue_flag;
    int                     task_id;
    int                     task_window_size;
    int32_t                 task_threshold_size;
    int                     total_tasks_to_be_exec;
    uint32_t                local_task_inserted;
    uint8_t                 function_counter;
    uint8_t                 flow_set_flag[PARSEC_DTD_NB_FUNCTIONS];
    parsec_handle_wait_t    *wait_func;
    parsec_mempool_t        *hash_table_bucket_mempool;
    parsec_dtd_two_hash_table_t *two_hash_table;
    hash_table              *function_h_table;
    /* ring of initial ready tasks */
    parsec_execution_context_t **startup_list;
    /* from here to end is for the testing interface */
    struct hook_info actual_hook[PARSEC_DTD_NB_FUNCTIONS];
};

/*
 * Extension of parsec_function_t class
 */
struct parsec_dtd_function_s {
    parsec_function_t     super;
    parsec_dtd_funcptr_t *fpointer;
    parsec_mempool_t      context_mempool;
    parsec_mempool_t      remote_task_mempool;
    int                   index_of_rank_info;
    int8_t                dep_datatype_index;
    int8_t                dep_out_index;
    int8_t                dep_in_index;
    int8_t                count_of_params;
    int                   ref_count;
    long unsigned int     size_of_param;
};

/* Function prototypes */
void
parsec_detach_all_dtd_handles_from_context( parsec_context_t *context );

void
parsec_dtd_template_release( const parsec_function_t *function );

void
parsec_dtd_handle_release( parsec_handle_t *parsec_handle );

void
parsec_dtd_handle_retain( parsec_handle_t *parsec_handle );

void
parsec_dtd_handle_destruct( parsec_handle_t *parsec_handle );

int
parsec_dtd_enqueue( parsec_handle_t *handle, void * );

parsec_dtd_task_t *
parsec_dtd_create_and_initialize_task( parsec_dtd_handle_t *parsec_dtd_handle,
                                       parsec_function_t   *function,
                                       int rank );

void
parsec_dtd_set_params_of_task( parsec_dtd_task_t *this_task, parsec_dtd_tile_t *tile,
                               int tile_op_type, int *flow_index, void **current_val,
                               parsec_dtd_task_param_t *current_param, int arg_size );

void
parsec_insert_dtd_task( parsec_dtd_task_t *this_task );

void
parsec_dtd_startup( parsec_context_t *context,
                    parsec_handle_t *parsec_handle,
                    parsec_execution_context_t **pready_list );

int
data_lookup_of_dtd_task( parsec_execution_unit_t *,
                         parsec_execution_context_t * );

void
parsec_dtd_ordering_correctly_1( parsec_execution_unit_t * eu,
                                 const parsec_execution_context_t * this_task,
                                 uint32_t action_mask,
                                 parsec_ontask_function_t * ontask,
                                 void *ontask_arg );

void
parsec_dtd_schedule_tasks( parsec_dtd_handle_t *__parsec_handle );

/* Function to remove tile from hash_table
 */
void
parsec_dtd_tile_remove( parsec_ddesc_t *ddesc, uint32_t key );

/* Function to find tile in hash_table
 */
parsec_dtd_tile_t *
parsec_dtd_tile_find( parsec_ddesc_t *ddesc, uint32_t key );

void
parsec_dtd_tile_release( parsec_dtd_tile_t *tile );

uint32_t
hash_key( uintptr_t key, int size );

void
parsec_dtd_tile_insert( uint32_t key,
                        parsec_dtd_tile_t *tile,
                        parsec_ddesc_t    *ddesc );

parsec_dtd_function_t *
parsec_dtd_function_find( parsec_dtd_handle_t  *parsec_handle,
                          uint64_t key );

parsec_function_t*
parsec_dtd_create_function( parsec_dtd_handle_t *__parsec_handle, parsec_dtd_funcptr_t* fpointer,
                            char* name, int count_of_params, long unsigned int size_of_param,
                            int flow_count );

void
parsec_dtd_add_profiling_info( parsec_handle_t *parsec_handle,
                               int function_id, char *name );

void
parsec_dtd_add_profiling_info_generic( parsec_handle_t *parsec_handle,
                            char* name,
                            int *keyin, int *keyout);

void
parsec_dtd_task_release( parsec_dtd_handle_t  *parsec_handle,
                         uint32_t             key );

void
parsec_execute_and_come_back( parsec_context_t *context,
                              parsec_handle_t  *parsec_handle,
                              int task_threshold_count );

dep_t *
parsec_dtd_find_and_return_dep( parsec_dtd_task_t *parent_task, parsec_dtd_task_t *desc_task,
                                int parent_flow_index, int desc_flow_index );

void
parsec_dtd_insert_task( parsec_dtd_handle_t *parsec_handle,
                        uint64_t            key,
                        void                *value );

void *
parsec_dtd_find_task( parsec_dtd_handle_t *parsec_handle,
                      uint64_t            key );

void *
parsec_dtd_find_and_remove_task( parsec_dtd_handle_t *parsec_handle,
                                 uint64_t            key );

void
parsec_dtd_insert_remote_dep( parsec_dtd_handle_t *parsec_handle,
                              uint64_t            key,
                              void                *value );

int
parsec_dtd_task_is_local( parsec_dtd_task_t *task );

int
parsec_dtd_task_is_remote( parsec_dtd_task_t *task );

void
parsec_dtd_remote_task_release( parsec_dtd_task_t *this_task );

parsec_hook_return_t
parsec_dtd_release_local_task( parsec_dtd_task_t *this_task );

int
parsec_dtd_copy_data_to_matrix( parsec_execution_unit_t    *eu,
                                parsec_execution_context_t *this_task );

void
parsec_dtd_function_release( parsec_dtd_handle_t  *parsec_handle,
                             uint64_t key );

void
parsec_dtd_tile_retain( parsec_dtd_tile_t *tile );

void
parsec_dtd_tile_release( parsec_dtd_tile_t *tile );

/* Initiate and Finish dtd environment
 * parsec_dtd_init() should be called right after parsec_init()
 * parsec_dtd_fini() right before parsec_fini()
 */
void
parsec_dtd_init();

void
parsec_dtd_fini();

static inline void
parsec_dtd_retain_floating_data( parsec_data_copy_t *data )
{
    OBJ_RETAIN(data);
}

static inline void
parsec_dtd_release_floating_data( parsec_data_copy_t *data )
{
    OBJ_RELEASE(data);
}

/***************************************************************************//**
 *
 * Function to lock last_user of a tile
 *
 * @param[in,out]   last_user
 *                      User we are trying to lock
 * @ingroup         DTD_INTERFACE_INTERNAL
 *
 ******************************************************************************/
static inline void
parsec_dtd_last_user_lock( parsec_dtd_tile_user_t *last_user )
{
    parsec_atomic_lock(&last_user->atomic_lock);
}

/***************************************************************************//**
 *
 * Function to unlock last_user of a tile
 *
 * @param[in,out]   last_user
 *                      User we are trying to unlock
 * @ingroup         DTD_INTERFACE_INTERNAL
 *
 ******************************************************************************/
static inline void
parsec_dtd_last_user_unlock( parsec_dtd_tile_user_t *last_user )
{
    parsec_atomic_unlock(&last_user->atomic_lock);
}

/***************************************************************************//**
 *
 * Function to lock last_user of a tile
 *
 * @param[in,out]   last_user
 *                      User we are trying to lock
 * @ingroup         DTD_INTERFACE_INTERNAL
 *
 ******************************************************************************/
static inline void
parsec_dtd_two_hash_table_lock( parsec_dtd_two_hash_table_t *two_hash_table )
{
    parsec_atomic_lock(&two_hash_table->atomic_lock);
}

/***************************************************************************//**
 *
 * Function to unlock last_user of a tile
 *
 * @param[in,out]   last_user
 *                      User we are trying to unlock
 * @ingroup         DTD_INTERFACE_INTERNAL
 *
 ******************************************************************************/
static inline void
parsec_dtd_two_hash_table_unlock( parsec_dtd_two_hash_table_t *two_hash_table )
{
    parsec_atomic_unlock(&two_hash_table->atomic_lock);
}

END_C_DECLS

#endif  /* INSERT_FUNCTION_INTERNAL_H_HAS_BEEN_INCLUDED */
