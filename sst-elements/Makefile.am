ACLOCAL_AMFLAGS = -I config
AM_CXXFLAGS = -Wall -g -Wno-deprecated-declarations -Wno-uninitialized -Wno-unused-local-typedefs -Wno-sign-compare -std=c++17 -fopenmp -fconcepts
AM_CPPFLAGS = -I $(top_srcdir)/src -I /usr/local/include \
	$(MPI_CPPFLAGS) \
	$(PYTHON_CPPFLAGS) $(CPPFLAGS)
  
compdir = $(pkglibdir)
#compdir = $(cwd)
comp_LTLIBRARIES = libbyod.la
#sstdir = $(includedir)/sst/elements/memHierarchy

libbyod_la_SOURCES = \
	src_cpp/test.cc \
	src_cpp/boilerplate_new.cc \
	src_cpp/CPU/streaming_cpu.cc \
	src_cpp/OptoElectronic/Submodules/modulators.cc \
	src_cpp/OptoElectronic/adc.cc \
	src_cpp/OptoElectronic/dac.cc \
	src_cpp/OptoElectronic/clements_svd.cc \
	src_cpp/OptoElectronic/clements.cc \
	src_cpp/OptoElectronic/amplitude_modulator.cc \
	src_cpp/OptoElectronic/photo_detector.cc

libbyod_la_LDFLAGS = -module -avoid-version -L$(pkglibdir) -lblas -llapack

#BUILT_SOURCES = pybyod.inc

# This sed script converts 'od' output to a comma-separated list of byte-
# values, suitable for #include'ing into an array definition.
# This can be done much more simply with xxd or hexdump, but those tools
# are not installed by default on all supported platforms.
#
# od:	-v:		Print all data
#		-t x1:	Print as byte-values, in hex
# sed:	Script 1:  Remove base-address column from od output
# 		Script 2:  Remove trailing blank line resulting from script 1
# 		Script 3:  Add '0x' prefix, and ',' suffix to each value
%.inc: %.py
	od -v -t x1 < $< | sed -e 's/^[^ ]*[ ]*//g' -e '/^\s*$$/d' -e 's/\([0-9a-f]*\)[ $$]*/0x\1,/g' > $@

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ARO_LIBRARY SST_ARO_LIBRARY_LIBDIR=$(pkglibdir)


clean-local: clean-local-check
clean-local-check:
	-rm -rf pytorchinterface.inc
