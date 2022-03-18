/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
#include <mpi.h>
#include <unistd.h>
//
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5ldump.hpp"

const char *hdf5sig="\211HDF\r\n\032\n";

const char ncsig[]	 = {'C', 'D', 'F'};

inline std::string get_file_signature (std::string &path) {
	hid_t fid		 = -1;	// File ID
	hid_t faplid	 = -1;	// File access property ID
	hid_t nativevlid = -1;	// Native VOL ID
	std::ifstream fin;		// File stream for reading file signature
	char sig[8];			// File signature
	htri_t islog, isnc4;
	std::string ret;

	fin.open (path, std::ios_base::in);
	if (fin.is_open ()) {
		memset (sig, 0, sizeof (sig));
		fin.read (sig, 8);
		fin.close ();
		if (!memcmp (hdf5sig, sig, 8)) {
			// Always use native VOL
			nativevlid = H5VLpeek_connector_id_by_name ("native");
			faplid	   = H5Pcreate (H5P_FILE_ACCESS);
			H5Pset_vol (faplid, nativevlid, NULL);

			// Open the input file
			fid = H5Fopen (path.c_str (), H5F_ACC_RDONLY, faplid);
			CHECK_ID (fid)

			islog = H5Aexists (fid, H5VL_LOG_FILEI_ATTR_INT);
			isnc4 = H5Aexists (fid, "_NCProperties");

			if (islog) {
				ret = std::string ("HDF5-LogVOL");
			} else if (isnc4) {
				ret = std::string ("netCDF-4");
			} else {
				ret = std::string ("HDF5");
			}
		} else if (!memcmp (ncsig, sig, 3)) {
                             if (sig[3] == 5)  ret = std::string ("NetCDF-classic");
                        else if (sig[3] == 2)  ret = std::string ("NetCDF 64-bit offset");
                        else if (sig[3] == 1)  ret = std::string ("NetCDF 64-bit data");

		} else {
			ret = std::string ("unknown");
		}
	} else {
		ret = std::string ("Unknown");
		std::cout << "Error: cannot open " << path << std::endl;
	}
err_out:;
	if (fid >= 0) { H5Fclose (fid); }
	if (faplid >= 0) { H5Pclose (faplid); }
	return ret;
}

int main (int argc, char *argv[]) {
	herr_t err = 0;
	int rank, np;
	int opt;
	bool dumpdata  = false;					  // Dump data along with metadata
	bool showftype = false;					  // Show file type
	std::string inpath;						  // Input file path
	std::vector<H5VL_log_dset_info_t> dsets;  // Dataset infos
	std::string ftype;						  // File type

	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &np);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	if (np > 1) {
		if (rank == 0) { std::cout << "Warning: h5ldump is sequential" << std::endl; }
		return 0;
	}

	// Parse input
	while ((opt = getopt (argc, argv, "hkd")) != -1) {
		switch (opt) {
			case 'd':
				dumpdata = true;
				break;
			case 'k':
				showftype = true;
				break;
			case 'h':
			default:
				if (rank == 0) { std::cout << "Usage: h5ldump [-h|-k|-d] <input file path>" << std::endl; }
				err = -1;
				goto err_out;
		}
	}

	if (optind >= argc || argv[optind] == NULL) { /* input file is mandatory */
		if (rank == 0) { std::cout << "Usage: h5ldump [-h|-k|-d] <input file path>" << std::endl; }
		err = -1;
		goto err_out;
	}
	inpath = std::string (argv[optind]);

	// Make sure input file is HDF5 file
	ftype = get_file_signature (inpath);
	if (showftype) {
		std::cout << ftype << std::endl;
		goto err_out;
	}

	if (ftype != "Logvol") {
		std::cout << "Error: " << inpath << " is not a valid logvol file." << std::endl;
		err = -1;

		if (ftype == "HDF5") {
			std::cout << inpath << " is a regular HDF5 file." << std::endl;
			std::cout << "Use h5dump in HDF5 utilities to read regular HDF5 files." << std::endl;
		} else if (ftype == "NetCDF" || ftype == "NetCDF 4") {
			std::cout << inpath << " is a " << ftype << " file." << std::endl;
			std::cout << "Use ncdump in NetCDF utilities to read NetCDF files." << std::endl;
		} else {
			std::cout << "Type of " << inpath << " is unknown" << std::endl;
		}
	}

	// Get dataaset metadata
	err = h5ldump_visit (inpath, dsets);
	CHECK_ERR

	// Dump the logs
	err = h5ldump_file (inpath, dsets, dumpdata, 0);
	CHECK_ERR

	// Cleanup dataset contec
	for (auto &d : dsets) {
		if (d.dtype != -1) { H5Tclose (d.dtype); }
	}

err_out:;
	return err == 0 ? 0 : -1;
}

herr_t h5ldump_file (std::string path,
					 std::vector<H5VL_log_dset_info_t> &dsets,
					 bool dumpdata,
					 int indent) {
	herr_t err = 0;
	int mpierr;
	int i;
	MPI_File fh		 = MPI_FILE_NULL;
	hid_t fid		 = -1;	// File ID
	hid_t faplid	 = -1;	// File access property ID
	hid_t nativevlid = -1;	// Native VOL ID
	hid_t lgid		 = -1;	// Log group ID
	hid_t aid		 = -1;	// File attribute ID
	int ndset;				// Number of user datasets
	int nldset;				// Number of data datasets
	int nmdset;				// Number of metadata datasets
	int nsubfile;			// Number of subfiles
	int config;				// File config flags
	int att_buf[5];			// attirbute buffer

	// Always use native VOL
	nativevlid = H5VLpeek_connector_id_by_name ("native");
	faplid	   = H5Pcreate (H5P_FILE_ACCESS);
	H5Pset_vol (faplid, nativevlid, NULL);

	// Open the input file
	fid = H5Fopen (path.c_str (), H5F_ACC_RDONLY, faplid);
	CHECK_ID (fid)

	if (dumpdata) {
		mpierr =
			MPI_File_open (MPI_COMM_SELF, path.c_str (), MPI_MODE_RDONLY, MPI_INFO_NULL, &(fh));
		CHECK_MPIERR
	}

	// Read file metadata
	aid = H5Aopen (fid, H5VL_LOG_FILEI_ATTR_INT, H5P_DEFAULT);
	if (!aid) {
		std::cout << "Error: " << path << " is not a valid log-based VOL file." << std::endl;
		std::cout << "Use h5dump in HDF5 utilities to read traditional HDF5 files." << std::endl;
		ERR_OUT ("File not recognized")
	}
	err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
	CHECK_ERR
	ndset	 = att_buf[0];
	nldset	 = att_buf[1];
	nmdset	 = att_buf[2];
	config	 = att_buf[3];
	nsubfile = att_buf[4];

	std::cout << std::string (indent, ' ') << "File: " << path << std::endl;
	indent += 4;
	std::cout << std::string (indent, ' ') << "File properties: " << std::endl;
	indent += 4;
	std::cout << std::string (indent, ' ') << "Automatic write reqeusts merging: "
			  << ((config & H5VL_FILEI_CONFIG_METADATA_MERGE) ? "on" : "off") << std::endl;
	std::cout << std::string (indent, ' ')
			  << "Metadata encoding: " << ((config & H5VL_FILEI_CONFIG_SEL_ENCODE) ? "on" : "off")
			  << std::endl;
	std::cout << std::string (indent, ' ') << "Metadata compression: "
			  << ((config & H5VL_FILEI_CONFIG_SEL_DEFLATE) ? "on" : "off") << std::endl;
	std::cout << std::string (indent, ' ') << "Metadata deduplication: "
			  << ((config & H5VL_FILEI_CONFIG_METADATA_SHARE) ? "on" : "off") << std::endl;
	std::cout << std::string (indent, ' ')
			  << "Subfiling: " << ((config & H5VL_FILEI_CONFIG_SUBFILING) ? "on" : "off")
			  << std::endl;
	std::cout << std::string (indent, ' ') << "Number of user datasets: " << ndset << std::endl;
	std::cout << std::string (indent, ' ') << "Number of data datasets: " << nldset << std::endl;
	std::cout << std::string (indent, ' ') << "Number of metadata datasets: " << nmdset
			  << std::endl;
	std::cout << std::string (indent, ' ') << "Number of metadata datasets: " << nmdset
			  << std::endl;
	if (config & H5VL_FILEI_CONFIG_SUBFILING) {
		std::cout << std::string (indent, ' ') << "Number of subfiles: " << nsubfile << std::endl;
	}
	indent -= 4;

	if (config & H5VL_FILEI_CONFIG_SUBFILING) {
		for (i = 0; i < nsubfile; i++) {
			std::cout << std::string (indent, ' ') << "Subfile " << i << std::endl;
			err = h5ldump_file (path + ".subfiles/" + std::to_string (i) + ".h5", dsets, dumpdata,
								indent + 4);
			CHECK_ERR
			// std::cout << std::string (indent, ' ') << "End subfile " << i << std::endl;
		}
	} else {
		// Open the log group
		lgid = H5Gopen2 (fid, "__LOG", H5P_DEFAULT);
		CHECK_ID (lgid)
		if (lgid < 0) {
			std::cout << "Error: " << path << " is not a valid log-based VOL file." << std::endl;
			std::cout << "Use h5ldump in HDF5 utilities to read traditional HDF5 files."
					  << std::endl;
			ERR_OUT ("File not recognized")
		}
		// Iterate through metadata datasets
		for (i = 0; i < nmdset; i++) {
			std::cout << std::string (indent, ' ') << "Metadata dataset " << i << std::endl;
			err = h5ldump_mdset (lgid,
								 H5VL_LOG_FILEI_DSET_META + std::string ("_") + std::to_string (i),
								 dsets, fh, indent + 4);
			CHECK_ERR
			// std::cout << std::string (indent, ' ') << "End metadata dataset " << i << std::endl;
		}
	}
	indent -= 4;
	;
	// std::cout << std::string (indent, ' ') << "End file: " << path << std::endl;

err_out:;
	if (fh != MPI_FILE_NULL) { MPI_File_close (&fh); }
	if (aid >= 0) { H5Aclose (aid); }
	if (lgid >= 0) { H5Gclose (lgid); }
	if (fid >= 0) { H5Fclose (fid); }
	if (faplid >= 0) { H5Pclose (faplid); }
	return err;
}
