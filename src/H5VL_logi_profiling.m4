dnl Process this m4 file to produce 'C' language file.
dnl
dnl If you see this line, you can ignore the next one.
/* Do not edit this file. It is produced from the corresponding .m4 source */
dnl
/*
 *  Copyright (C) 2021, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */
dnl
include(`foreach.m4')`'dnl
include(`foreach_idx.m4')`'dnl
include(`list_len.m4')`'dnl
include(`H5VL_logi_profiling_timers.m4')`'dnl
define(`upcase', `translit(`$*', `a-z', `A-Z')')`'dnl
define(`CONCATE',`$1$2')`'dnl
changecom(`##', `')`'dnl
dnl
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_logi_profiling.hpp"
#include <mpi.h>
#include <libgen.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "H5VL_log_file.hpp"
#include "H5VL_logi.hpp"

static double tmax[H5VL_LOG_NTIMER], tmin[H5VL_LOG_NTIMER], tmean[H5VL_LOG_NTIMER], tvar[H5VL_LOG_NTIMER], tvar_local[H5VL_LOG_NTIMER];

/*
 * Report performance profiling
 */

const char * const tname[H5VL_LOG_NTIMER]={
foreach(`t', H5VL_LOG_TIMERS, `"t",
')dnl
};

void H5VL_log_profile_add_time (void *file, int id, double t) {
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

    assert (id >= 0 && id < H5VL_LOG_NTIMER);
    fp->tlocal[id] += t;
    fp->clocal[id]++;
}

void H5VL_log_profile_sub_time (void *file, int id, double t) {
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

    assert (id >= 0 && id < H5VL_LOG_NTIMER);
    fp->tlocal[id] -= t;
}

// Note: This only work if everyone calls H5Fclose
void H5VL_log_profile_print (void *file) {
    int i;
    int np, rank, flag;
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
    std::string fname   = basename ((char *)(fp->name.c_str ()));

    MPI_Initialized (&flag);
    if (!flag) { MPI_Init (NULL, NULL); }

    MPI_Comm_size (fp->comm, &np);
    MPI_Comm_rank (fp->comm, &rank);

    MPI_Reduce (fp->tlocal, tmax, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_MAX, 0, fp->comm);
    MPI_Reduce (fp->tlocal, tmin, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_MIN, 0, fp->comm);
    MPI_Allreduce (fp->tlocal, tmean, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_SUM, fp->comm);
    for (i = 0; i < H5VL_LOG_NTIMER; i++) {
        tmean[i] /= np;
        tvar_local[i] = (fp->tlocal[i] - tmean[i]) * (fp->tlocal[i] - tmean[i]);
    }
    MPI_Reduce (tvar_local, tvar, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_SUM, 0, fp->comm);

    if (rank == 0) {
        for (i = 0; i < H5VL_LOG_NTIMER; i++) {
            printf ("LOGVOL_%s: %s_time_mean: %lf\n", fname.c_str (), tname[i], tmean[i]);
            printf ("LOGVOL_%s: %s_time_max: %lf\n", fname.c_str (), tname[i], tmax[i]);
            printf ("LOGVOL_%s: %s_time_min: %lf\n", fname.c_str (), tname[i], tmin[i]);
            printf ("LOGVOL_%s: %s_time_var: %lf\n\n", fname.c_str (), tname[i], tvar[i]);
        }
    }

    MPI_Reduce (fp->clocal, tmax, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_MAX, 0, fp->comm);
    MPI_Reduce (fp->clocal, tmin, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_MIN, 0, fp->comm);
    MPI_Allreduce (fp->clocal, tmean, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_SUM, fp->comm);
    for (i = 0; i < H5VL_LOG_NTIMER; i++) {
        tmean[i] /= np;
        tvar_local[i] = (fp->clocal[i] - tmean[i]) * (fp->clocal[i] - tmean[i]);
    }
    MPI_Reduce (tvar_local, tvar, H5VL_LOG_NTIMER, MPI_DOUBLE, MPI_SUM, 0, fp->comm);

    if (rank == 0) {
        for (i = 0; i < H5VL_LOG_NTIMER; i++) {
            printf ("LOGVOL_%s: %s_count_mean: %lf\n", fname.c_str (), tname[i], tmean[i]);
            printf ("LOGVOL_%s: %s_count_max: %lf\n", fname.c_str (), tname[i], tmax[i]);
            printf ("LOGVOL_%s: %s_count_min: %lf\n", fname.c_str (), tname[i], tmin[i]);
            printf ("LOGVOL_%s: %s_count_var: %lf\n\n", fname.c_str (), tname[i], tvar[i]);
        }
    }
}
void H5VL_log_profile_reset (void *file) {
    int i;
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

    for (i = 0; i < H5VL_LOG_NTIMER; i++) {
        fp->tlocal[i] = 0;
        fp->clocal[i] = 0;
    }
}
