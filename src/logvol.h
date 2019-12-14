#ifndef _logVOL_H_
#define _logVOL_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: This is a "pass through" VOL connector, which forwards each
 *      VOL callback to an underlying connector.
 *
 *      It is designed as an example VOL connector for developers to
 *      use when creating new connectors, especially connectors that
 *      are outside of the HDF5 library.  As such, it should _NOT_
 *      include _any_ private HDF5 header files.  This connector should
 *      therefore only make public HDF5 API calls and use standard C /
 *              POSIX calls.
 */

/* Header files needed */
/* (Public HDF5 and standard C / POSIX only) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "hdf5.h"

/* Identifier for the pass-through VOL connector */
#define H5VL_log  (H5VL_log_register())
#ifdef __cplusplus
extern "C" {
#endif
 hid_t H5VL_log_register(void);
#ifdef __cplusplus
}
#endif

#define CHECK_ERR { \
    if (err != NC_NOERR) { \
        printf("Error at line %d in %s: (%s)\n", \
        __LINE__,__FILE__,logstrerrno(err)); \
        return -1; \
    } \
}

#define CHECK_ERRN { \
    if (err != NC_NOERR) { \
        printf("Error at line %d in %s: (%s)\n", \
        __LINE__,__FILE__,logstrerrno(err)); \
        return NULL; \
    } \
}

#define CHECK_ERRJ { \
    if (err != NC_NOERR) { \
        printf("Error at line %d in %s: (%s)\n", \
        __LINE__,__FILE__,logstrerrno(err)); \
        goto errout; \
    } \
}

#define CHECK_HERR { \
    if (herr < 0) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        return -1; \
    } \
}

#define CHECK_HERRN { \
    if (herr < 0) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        return NULL; \
    } \
}

#define RET_ERR(A) { \
    printf("Error at line %d in %s: %s\n", \
    __LINE__,__FILE__, A); \
    return -1; \
}

#define RET_ERRN(A) { \
    printf("Error at line %d in %s: %s\n", \
    __LINE__,__FILE__, A); \
    return NULL; \
}

/************/
/* Typedefs */
/************/

/* Pass-through VOL connector info */
typedef struct H5VL_log_info_t {
    hid_t under_vol_id;         /* VOL ID for under VOL */
    void *under_vol_info;       /* VOL info for under VOL */
    MPI_Comm comm;
} H5VL_log_info_t;

/* The pass through VOL info object */
typedef struct H5VL_log_file_t {
    int rank;
    int refcnt;
    bool closing;
    hid_t dlogid;
    hid_t mlogid;
} H5VL_log_file_t;

/* The pass through VOL info object */
typedef struct H5VL_log_group_t {
    H5VL_log_file_t *fp;
} H5VL_log_group_t;

/* The pass through VOL info object */
typedef struct H5VL_log_dataset_t {
    int id;
    H5VL_log_file_t *fp;
} H5VL_log_dataset_t;

/* The pass through VOL info object */
typedef struct H5VL_log_obj_t {
    hid_t under_vol_id;
    void *under_obj;
} H5VL_log_obj_t;

extern MPI_Datatype h5t_to_mpi_type(hid_t type_id);
extern void sortreq(int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts);
extern int intersect(int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb);
extern void mergereq(int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts);
extern void sortblock(int ndim, hssize_t len, hsize_t **starts);
extern bool hlessthan(int ndim, hsize_t *a, hsize_t *b);

extern const H5VL_file_class_t H5VL_log_file_g;
extern const H5VL_dataset_class_t H5VL_log_dataset_g;
extern const H5VL_attr_class_t H5VL_log_attr_g;
extern const H5VL_group_class_t H5VL_log_group_g;
extern const H5VL_class_t H5VL_log_g;

#endif