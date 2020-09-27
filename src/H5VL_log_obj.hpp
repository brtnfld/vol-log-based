#pragma once

#include <H5VLpublic.h>

struct H5VL_log_file_t;
typedef struct H5VL_log_obj_t {
	H5I_type_t type;
	void *uo;					 // Under obj
	hid_t uvlid;				 // Under VolID
	struct H5VL_log_file_t *fp;	 // File object
} H5VL_log_obj_t;

extern const H5VL_object_class_t H5VL_log_object_g;

void *H5VL_log_object_open (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5I_type_t *opened_type,
							hid_t dxpl_id,
							void **req);
herr_t H5VL_log_object_copy (void *src_obj,
							 const H5VL_loc_params_t *src_loc_params,
							 const char *src_name,
							 void *dst_obj,
							 const H5VL_loc_params_t *dst_loc_params,
							 const char *dst_name,
							 hid_t ocpypl_id,
							 hid_t lcpl_id,
							 hid_t dxpl_id,
							 void **req);
herr_t H5VL_log_object_get (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5VL_object_get_t get_type,
							hid_t dxpl_id,
							void **req,
							va_list arguments);
herr_t H5VL_log_object_specific (void *obj,
								 const H5VL_loc_params_t *loc_params,
								 H5VL_object_specific_t specific_type,
								 hid_t dxpl_id,
								 void **req,
								 va_list arguments);
herr_t H5VL_log_object_optional (
	void *obj, H5VL_object_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments);


// Internal
extern void *H5VL_log_obj_open_with_uo (void *obj, void *uo, H5I_type_t type, const H5VL_loc_params_t *loc_params);