/**
 * @copyright (c) 2017 King Abdullah University of Science and Technology (KAUST).
 *                     All rights reserved.
 **/
/**
 * @file codelet_zgytlr.c
 *
 *  HiCMA codelets kernel
 *  HiCMA is a software package provided by King Abdullah University of Science and Technology (KAUST)
 *
 * @version 0.1.0
 * @author Kadir Akbudak
 * @date 2017-11-16
 * @precisions normal z -> c d s
 **/
#include "morse.h"
#include "runtime/starpu/chameleon_starpu.h"
//#include "runtime/starpu/include/runtime_codelet_z.h"
#include "hcore_z.h"

#include "runtime/starpu/runtime_codelets.h"
ZCODELETS_HEADER(compressdiag)


//CHAMELEON_CL_CB(zgytlr,        starpu_matrix_get_nx(task->handles[0]), starpu_matrix_get_ny(task->handles[0]), 0,                                                M*N)
/*   MORSE_TASK_zgytlr - Generate a tile for random matrix. */

void HICMA_TASK_zcompress_diag( const MORSE_option_t *options,
                        int m, int n,
                        const MORSE_desc_t *AUV,
                        const MORSE_desc_t *AD, int ADm, int ADn,
                        const MORSE_desc_t *Ark,
                        int Am, int An, 
                        int lda,
                        int ldu,
                        int ldv,
                        int bigM, int m0, int n0,
                        int maxrank, double tol,
                        int compress_diag,
                        const MORSE_desc_t *Dense
                        )
{
    //fprintf(stderr, "At HICMA_TASK_zcompress_diag\n");
    struct starpu_codelet *codelet = &cl_zcompressdiag;
    //void (*callback)(void*) = options->profiling ? cl_zgytlr_callback : NULL;
    void (*callback)(void*) = NULL;
    int nAUV = AUV->nb;

    MORSE_BEGIN_ACCESS_DECLARATION;
    MORSE_ACCESS_W(AUV, Am, An);
    MORSE_ACCESS_W(AD, ADm, ADn);
    MORSE_ACCESS_W(Dense, Am, An);
    MORSE_ACCESS_W(Ark, Am, An);
    MORSE_END_ACCESS_DECLARATION;

    // printf("%s:%d: Am:%d An:%d lda:%d bigM:%d m0:%d n0:%d\n ", __FILE__, __LINE__, Am, An, lda, bigM, m0, n0);

        //printf("%s %d: Am:%d An:%d ADm:%d ADn:%d ptr:%p\n", __func__, __LINE__, Am, An, ADm, ADn, ptr);
    starpu_insert_task(
            starpu_mpi_codelet(codelet),
            STARPU_VALUE,    &m,                      sizeof(int),
            STARPU_VALUE,    &n,                      sizeof(int),
            STARPU_VALUE,    &nAUV,                      sizeof(int),
            STARPU_W,         RTBLKADDR(AUV, double, Am, An),
            STARPU_W,         RTBLKADDR(AD, double, ADm, ADn),
            STARPU_W,         RTBLKADDR(Ark, double, Am, An),
            STARPU_R,         RTBLKADDR(Dense, double, Am, An), // _R must be _W SERIOUSLY. BUT _W STALLS ON SHAHEEN. FIXME
            STARPU_VALUE,  &lda,                      sizeof(int),
            STARPU_VALUE,  &ldu,                      sizeof(int),
            STARPU_VALUE,  &ldv,                      sizeof(int),
            STARPU_VALUE, &bigM,                      sizeof(int),
            STARPU_VALUE,   &Am,                      sizeof(int),
            STARPU_VALUE,   &An,                      sizeof(int),
            STARPU_VALUE,   &maxrank,                  sizeof(int),
            STARPU_VALUE,   &tol,                      sizeof(double),
            STARPU_VALUE,   &compress_diag,            sizeof(int),
            STARPU_PRIORITY,    options->priority,
            STARPU_CALLBACK,    callback,
#if defined(CHAMELEON_CODELETS_HAVE_NAME)
            STARPU_NAME, "hcore_zcompress_diag",
#endif
            0);
}

/*   cl_zgytlr_cpu_func - Generate a tile for random matrix. */

#if !defined(CHAMELEON_SIMULATION)
static void cl_zcompress_cpu_func(void *descr[], void *cl_arg)
{
    int m;
    int n;
    int nAUV;
    double *AUV;
    double *AD;
    double *Ark;
    double *Dense;
    int lda;
    int ldu;
    int ldv;
    int bigM;
    int m0;
    int n0;
    int maxrank;
    double tol;
    int compress_diag;

    AUV = (double *)STARPU_MATRIX_GET_PTR(descr[0]);
    AD = (double *)STARPU_MATRIX_GET_PTR(descr[1]);
    Ark = (double *)STARPU_MATRIX_GET_PTR(descr[2]);
    Dense = (double *)STARPU_MATRIX_GET_PTR(descr[3]);


    starpu_codelet_unpack_args(cl_arg, &m, &n, &nAUV, &lda, &ldu, &ldv, &bigM, &m0, &n0, &maxrank, &tol, &compress_diag );

    double *AU = AUV;
    int nAU = nAUV/2;
    size_t nelm_AU = (size_t)lda * (size_t)nAU;
    double *AV = &(AUV[nelm_AU]);

    //printf("(%d,%d)%d %s %d %d\n", m0/m,n0/n,MORSE_My_Mpi_Rank(), __func__, __LINE__, AD == Dense);
    HCORE_zcompress( m, n,
            AU,
            AV,
            AD,
            Ark,
            lda, ldu, ldv, 
            bigM, m0, n0,
            maxrank, tol,
            compress_diag,
            Dense
            );
}
#endif /* !defined(CHAMELEON_SIMULATION) */

/*
 * Codelet definition
 */
CODELETS_CPU(zcompressdiag, 4, cl_zcompress_cpu_func)
