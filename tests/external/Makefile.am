#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

C_SUBDIRS =

if ENABLE_SHARED
if HAVE_QMCPACK
    C_SUBDIRS += qmcpack
endif
if HAVE_HDF5API
    C_SUBDIRS += hdf5api
endif
if HAVE_OPENPMD
    C_SUBDIRS += openpmd
endif
endif

#DIST_SUBDIRS = qmcpack hdf5_testAPI openpmd
#SUBDIRS = $(C_SUBDIRS)

ptest:
	@for d in $(C_SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptest $$* || exit 1 ; \
	done

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

.PHONY: ptest tests
