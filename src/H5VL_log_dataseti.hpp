#pragma once

#include <H5VLpublic.h>
#include <mpi.h>

#include "H5VL_log_obj.hpp"
#include "H5VL_log_wrap.hpp"
#include "H5VL_logi_idx.hpp"


typedef struct H5VL_log_copy_ctx {
	char *src;
	char *dst;
	size_t size;
} H5VL_log_copy_ctx;

void *H5VL_log_dataseti_open_with_uo (void *obj,
									 void *uo,
									 const H5VL_loc_params_t *loc_params,
									 hid_t dxpl_id);
void *H5VL_log_dataseti_wrap (void *uo, H5VL_log_wrap_ctx_t *cp);
herr_t H5VL_log_dataset_readi_gen_rtypes (std::vector<H5VL_log_search_ret_t> blocks,
												 MPI_Datatype *ftype,
												 MPI_Datatype *mtype,
												 std::vector<H5VL_log_copy_ctx> &overlaps);
