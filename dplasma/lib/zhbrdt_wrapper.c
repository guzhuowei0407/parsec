/*
 * Copyright (c) 2011-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * @precisions normal z -> s d c
 *
 */


#include <core_blas.h>
#include "parsec/parsec_config.h"
#include "dplasma.h"
#include "data_dist/matrix/matrix.h"
#include "dplasma/lib/dplasmatypes.h"
#include "dplasma/lib/dplasmaaux.h"
#include "parsec/private_mempool.h"

#include "zhbrdt.h"

parsec_taskpool_t* dplasma_zhbrdt_New(tiled_matrix_desc_t* A /* data A */)
{
    parsec_zhbrdt_taskpool_t *parsec_zhbrdt = NULL;

    parsec_zhbrdt = parsec_zhbrdt_new(A, A->mb-1);

    dplasma_add2arena_rectangle( parsec_zhbrdt->arenas[PARSEC_zhbrdt_DEFAULT_ARENA],
                                 (A->nb)*(A->mb)*sizeof(parsec_complex64_t), 16,
                                 parsec_datatype_double_complex_t,
                                 A->mb, A->nb, -1 );
    return (parsec_taskpool_t*)parsec_zhbrdt;
}

void dplasma_zhbrdt_Destruct( parsec_taskpool_t *tp )
{
    parsec_taskpool_free(tp);
}

