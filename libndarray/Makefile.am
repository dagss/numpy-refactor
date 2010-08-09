AUTOMAKE_OPTIONS = foreign

# Headers which are installed to support the library
INSTINCLUDES = \
        src/numpy_api.h

# Headers which are not installed
OTHERINCLUDES = \
	src/_neighborhood_iterator_imp.h \
	src/noprefix.h \
	src/npy_arrayobject.h \
	src/npy_common.h \
	src/npy_cpu.h \
	src/npy_defs.h \
        src/npy_descriptor.h \
        src/npy_dict.h \
        src/npy_endian.h \
        src/npy_internal.h \
        src/npy_iterators.h \
        src/npy_math.h \
        src/npy_object.h \
	src/numospy.h \
	src/utils.h

# Sources to build library
LIBSOURCES = \
        src/npy_arrayobject.c \
        src/npy_arraytypes.c \
        src/npy_calculation.c \
        src/npy_common.c \
        src/npy_conversion_utils.c \
        src/npy_convert.c \
        src/npy_convert_datatype.c \
        src/npy_ctors.c \
        src/npy_datetime.c \
        src/npy_descriptor.c \
        src/npy_dict.c \
        src/npy_flagsobject.c \
        src/npy_getset.c \
        src/npy_item_selection.c \
        src/npy_iterators.c \
        src/npy_mapping.c \
        src/npy_methods.c \
        src/npy_multiarraymodule.c \
        src/npy_refcount.c \
        src/npy_shape.c \
        src/npy_usertypes.c \
        src/numpyos.c

# libtool libraries to build
lib_LTLIBRARIES = libndarray.la

# Library sources for libndarray.la
libndarray_la_SOURCES = $(LIBSOURCES)

# Headers to install
include_HEADERS = $(INSTINCLUDES)

# Other distributed headers
noinst_HEADERS = $(OTHERINCLUDES)