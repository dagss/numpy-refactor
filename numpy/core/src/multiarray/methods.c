#define PY_SSIZE_T_CLEAN
#include <stdarg.h>
#include <Python.h>
#include "structmember.h"

#define _MULTIARRAYMODULE
#define NPY_NO_PREFIX
#include <npy_defs.h>
#include "numpy/arrayobject.h"
#include "numpy/arrayscalars.h"
#include "npy_api.h"

#include "npy_config.h"

#include "numpy/npy_3kcompat.h"

#include "common.h"
#include "ctors.h"
#include "calculation.h"
#include "descriptor.h"
#include "arrayobject.h"

#include "methods.h"


#define ASSERT_ONE_BASE(r) \
    assert(NULL == PyArray_BASE_ARRAY(r) || NULL == PyArray_BASE(r))

/* NpyArg_ParseKeywords
 *
 * Utility function that provides the keyword parsing functionality of
 * PyArg_ParseTupleAndKeywords without having to have an args argument.
 *
 */
static int
NpyArg_ParseKeywords(PyObject *keys, const char *format, char **kwlist, ...)
{
    PyObject *args = PyTuple_New(0);
    int ret;
    va_list va;

    if (args == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                "Failed to allocate new tuple");
        return 0;
    }
    va_start(va, kwlist);
    ret = PyArg_VaParseTupleAndKeywords(args, keys, format, kwlist, va);
    va_end(va);
    Py_DECREF(args);
    return ret;
}

/* Should only be used if x is known to be an nd-array */
#define _ARET(x) PyArray_Return((PyArrayObject *)(x))

static PyObject *
array_take(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int dimension = MAX_DIMS;
    PyObject *indices;
    PyArrayObject *out = NULL;
    NPY_CLIPMODE mode = NPY_RAISE;
    static char *kwlist[] = {"indices", "axis", "out", "mode", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O&O&O&", kwlist,
                                     &indices, PyArray_AxisConverter,
                                     &dimension,
                                     PyArray_OutputConverter,
                                     &out,
                                     PyArray_ClipmodeConverter,
                                     &mode))
        return NULL;

    return _ARET(PyArray_TakeFrom(self, indices, dimension, out, mode));
}

static PyObject *
array_fill(PyArrayObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    if (PyArray_FillWithScalar(self, obj) < 0) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
array_put(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *indices, *values;
    NPY_CLIPMODE mode = NPY_RAISE;
    static char *kwlist[] = {"indices", "values", "mode", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O&", kwlist,
                                     &indices, &values,
                                     PyArray_ClipmodeConverter,
                                     &mode))
        return NULL;
    return PyArray_PutTo(self, values, indices, mode);
}

static PyObject *
array_reshape(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"order", NULL};
    PyArray_Dims newshape;
    PyObject *ret;
    PyArray_ORDER order = PyArray_CORDER;
    Py_ssize_t n = PyTuple_Size(args);

    if (!NpyArg_ParseKeywords(kwds, "|O&", keywords,
                PyArray_OrderConverter, &order)) {
        return NULL;
    }

    if (n <= 1) {
        if (PyTuple_GET_ITEM(args, 0) == Py_None) {
            return PyArray_View(self, NULL, NULL);
        }
        if (!PyArg_ParseTuple(args, "O&", PyArray_IntpConverter,
                              &newshape)) {
            return NULL;
        }
    }
    else {
        if (!PyArray_IntpConverter(args, &newshape)) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_TypeError,
                                "invalid shape");
            }
            goto fail;
        }
    }
    ret = PyArray_Newshape(self, &newshape, order);
    PyDimMem_FREE(newshape.ptr);
    return ret;

 fail:
    PyDimMem_FREE(newshape.ptr);
    return NULL;
}

static PyObject *
array_squeeze(PyArrayObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }
    return PyArray_Squeeze(self);
}

static PyObject *
array_view(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *out_dtype = NULL;
    PyObject *out_type = NULL;
    PyArray_Descr *dtype = NULL;

    static char *kwlist[] = {"dtype", "type", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
                                     &out_dtype,
                                     &out_type))
        return NULL;

    /* If user specified a positional argument, guess whether it
       represents a type or a dtype for backward compatibility. */
    if (out_dtype) {
        /* type specified? */
        if (PyType_Check(out_dtype) &&
            PyType_IsSubtype((PyTypeObject *)out_dtype,
                             &PyArray_Type)) {
            if (out_type) {
                PyErr_SetString(PyExc_ValueError,
                                "Cannot specify output type twice.");
                return NULL;
            }
            out_type = out_dtype;
            out_dtype = NULL;
        }
    }

    if ((out_type) && (!PyType_Check(out_type) ||
                       !PyType_IsSubtype((PyTypeObject *)out_type,
                                         &PyArray_Type))) {
        PyErr_SetString(PyExc_ValueError,
                        "Type must be a sub-type of ndarray type");
        return NULL;
    }

    if ((out_dtype) &&
        (PyArray_DescrConverter(out_dtype, &dtype) == PY_FAIL)) {
        PyErr_SetString(PyExc_ValueError,
                        "Dtype must be a numpy data-type");
        return NULL;
    }

    return PyArray_View(self, dtype, (PyTypeObject*)out_type);
}

static PyObject *
array_argmax(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return _ARET(PyArray_ArgMax(self, axis, out));
}

static PyObject *
array_argmin(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return _ARET(PyArray_ArgMin(self, axis, out));
}

static PyObject *
array_max(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return PyArray_Max(self, axis, out);
}

static PyObject *
array_ptp(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return PyArray_Ptp(self, axis, out);
}


static PyObject *
array_min(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return PyArray_Min(self, axis, out);
}

static PyObject *
array_swapaxes(PyArrayObject *self, PyObject *args)
{
    int axis1, axis2;

    if (!PyArg_ParseTuple(args, "ii", &axis1, &axis2)) {
        return NULL;
    }
    return PyArray_SwapAxes(self, axis1, axis2);
}


/* steals typed reference */
/*NUMPY_API
  Get a subset of bytes from each element of the array
*/
NPY_NO_EXPORT PyObject *
PyArray_GetField(PyArrayObject *self, PyArray_Descr *typed, int offset)
{
    NpyArray_Descr *typedWrap;
    
    /* Move reference to the core object. */
    PyArray_Descr_REF_TO_CORE(typed, typedWrap);
    
    RETURN_PYARRAY(NpyArray_GetField(PyArray_ARRAY(self), typedWrap, offset));
}

static PyObject *
array_getfield(PyArrayObject *self, PyObject *args, PyObject *kwds)
{

    PyArray_Descr *dtype = NULL;
    int offset = 0;
    static char *kwlist[] = {"dtype", "offset", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|i", kwlist,
                                     PyArray_DescrConverter,
                                     &dtype, &offset)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    return PyArray_GetField(self, dtype, offset);
}


/*NUMPY_API
 * Set a subset of bytes from each element of the array
 *
 * Steals a reference to dtype.
*/
NPY_NO_EXPORT int
PyArray_SetField(PyArrayObject *self, PyArray_Descr *dtypeWrap,
                 int offset, PyObject *val)
{
    NpyArray_Descr *dtype;
    PyArrayObject *src;
    int result;
    
    /*
     * Special code to mimic Numeric behavior for
     * character arrays.
     */
    if (dtypeWrap->descr->type == PyArray_CHARLTR && PyArray_NDIM(self) > 0 \
        && PyString_Check(val)) {
        intp n_new, n_old;
        char *new_string;
        PyObject *tmp;

        n_new = PyArray_DIM(self, PyArray_NDIM(self)-1);
        n_old = PyString_Size(val);
        if (n_new > n_old) {
            new_string = (char *)malloc(n_new);
            memmove(new_string, PyString_AS_STRING(val), n_old);
            memset(new_string + n_old, ' ', n_new - n_old);
            tmp = PyString_FromStringAndSize(new_string, n_new);
            free(new_string);
            val = tmp;
        }
    }

    if (PyArray_Check(val)) {
        src = (PyArrayObject *)val;
        Py_INCREF(src);
    }
    else {
        Py_INCREF(dtypeWrap);
        src = (PyArrayObject *)PyArray_FromAny(val, dtypeWrap, 0,
                                               PyArray_NDIM(self),
                                               FORTRAN_IF(self),
                                               NULL);
    }
    if (src == NULL) {
        Py_DECREF(dtypeWrap);
        return -1;
    }

    /* Move the reference to the core object. */
    PyArray_Descr_REF_TO_CORE(dtypeWrap, dtype);
    
    result = NpyArray_SetField(PyArray_ARRAY(self), dtype, offset, 
                               PyArray_ARRAY(src));
    Py_DECREF(src);
    return result;
}

static PyObject *
array_setfield(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    PyArray_Descr *dtype = NULL;
    int offset = 0;
    PyObject *value;
    static char *kwlist[] = {"value", "dtype", "offset", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO&|i", kwlist,
                                     &value, PyArray_DescrConverter,
                                     &dtype, &offset)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    if (PyArray_SetField(self, dtype, offset, value) < 0) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

/* This doesn't change the descriptor just the actual data...
 */

/*NUMPY_API*/
NPY_NO_EXPORT PyObject *
PyArray_Byteswap(PyArrayObject *self, Bool inplace)
{
    RETURN_PYARRAY(NpyArray_Byteswap(PyArray_ARRAY(self), inplace));
}


static PyObject *
array_byteswap(PyArrayObject *self, PyObject *args)
{
    Bool inplace = FALSE;

    if (!PyArg_ParseTuple(args, "|O&", PyArray_BoolConverter, &inplace)) {
        return NULL;
    }
    return PyArray_Byteswap(self, inplace);
}

static PyObject *
array_tolist(PyArrayObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }
    return PyArray_ToList(self);
}


static PyObject *
array_tostring(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    NPY_ORDER order = NPY_CORDER;
    static char *kwlist[] = {"order", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                     PyArray_OrderConverter,
                                     &order)) {
        return NULL;
    }
    return PyArray_ToString(self, order);
}


/* This should grow an order= keyword to be consistent
 */

static PyObject *
array_tofile(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int ret;
    PyObject *file;
    FILE *fd;
    char *sep = "";
    char *format = "";
    static char *kwlist[] = {"file", "sep", "format", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|ss", kwlist,
                                     &file, &sep, &format)) {
        return NULL;
    }

    if (PyBytes_Check(file) || PyUnicode_Check(file)) {
        file = npy_PyFile_OpenFile(file, "wb");
        if (file == NULL) {
            return NULL;
        }
    }
    else {
        Py_INCREF(file);
    }
#if defined(NPY_PY3K)
    fd = npy_PyFile_Dup(file, "wb");
#else
    fd = PyFile_AsFile(file);
#endif
    if (fd == NULL) {
        PyErr_SetString(PyExc_IOError, "first argument must be a " \
                        "string or open file");
        Py_DECREF(file);
        return NULL;
    }
    ret = PyArray_ToFile(self, fd, sep, format);
#if defined(NPY_PY3K)
    fclose(fd);
#endif
    Py_DECREF(file);
    if (ret < 0) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
array_toscalar(PyArrayObject *self, PyObject *args) {
    int n, nd;
    n = PyTuple_GET_SIZE(args);

    if (n == 1) {
        PyObject *obj;
        obj = PyTuple_GET_ITEM(args, 0);
        if (PyTuple_Check(obj)) {
            args = obj;
            n = PyTuple_GET_SIZE(args);
        }
    }

    if (n == 0) {
        if (PyArray_NDIM(self) == 0 || PyArray_SIZE(self) == 1)
            return PyArray_DESCR(self)->f->getitem(PyArray_BYTES(self), 
                                                   PyArray_ARRAY(self));
        else {
            PyErr_SetString(PyExc_ValueError,
                            "can only convert an array "    \
                            " of size 1 to a Python scalar");
            return NULL;
        }
    }
    else if (n != PyArray_NDIM(self) && (n > 1 || PyArray_NDIM(self) == 0)) {
        PyErr_SetString(PyExc_ValueError,
                        "incorrect number of indices for "      \
                        "array");
        return NULL;
    }
    else if (n == 1) { /* allows for flat getting as well as 1-d case */
        intp value, loc, index, factor;
        intp factors[MAX_DIMS];
        value = PyArray_PyIntAsIntp(PyTuple_GET_ITEM(args, 0));
        if (error_converting(value)) {
            PyErr_SetString(PyExc_ValueError, "invalid integer");
            return NULL;
        }
        factor = PyArray_SIZE(self);
        if (value < 0) value += factor;
        if ((value >= factor) || (value < 0)) {
            PyErr_SetString(PyExc_ValueError,
                            "index out of bounds");
            return NULL;
        }
        if (PyArray_NDIM(self) == 1) {
            value *= PyArray_STRIDE(self, 0);
            return PyArray_DESCR(self)->f->getitem(PyArray_BYTES(self) + value,
                                                   PyArray_ARRAY(self));
        }
        nd = PyArray_NDIM(self);
        factor = 1;
        while (nd--) {
            factors[nd] = factor;
            factor *= PyArray_DIM(self, nd);
        }
        loc = 0;
        for (nd = 0; nd < PyArray_NDIM(self); nd++) {
            index = value / factors[nd];
            value = value % factors[nd];
            loc += PyArray_STRIDE(self, nd)*index;
        }

        return PyArray_DESCR(self)->f->getitem(PyArray_BYTES(self) + loc,
                                               PyArray_ARRAY(self));

    }
    else {
        intp loc, index[MAX_DIMS];
        nd = PyArray_IntpFromSequence(args, index, MAX_DIMS);
        if (nd < n) {
            return NULL;
        }
        loc = 0;
        while (nd--) {
            if (index[nd] < 0) {
                index[nd] += PyArray_DIM(self, nd);
            }
            if (index[nd] < 0 ||
                index[nd] >= PyArray_DIM(self, nd)) {
                PyErr_SetString(PyExc_ValueError,
                                "index out of bounds");
                return NULL;
            }
            loc += PyArray_STRIDE(self, nd)*index[nd];
        }
        return PyArray_DESCR(self)->f->getitem(PyArray_BYTES(self) + loc, 
                                               PyArray_ARRAY(self));
    }
}

static PyObject *
array_setscalar(PyArrayObject *self, PyObject *args) {
    int n, nd;
    int ret = -1;
    PyObject *obj;
    n = PyTuple_GET_SIZE(args) - 1;

    if (n < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "itemset must have at least one argument");
        return NULL;
    }
    obj = PyTuple_GET_ITEM(args, n);
    if (n == 0) {
        if (PyArray_NDIM(self) == 0 || PyArray_SIZE(self) == 1) {
            ret = PyArray_DESCR(self)->f->setitem(obj, PyArray_BYTES(self), 
                                                  PyArray_ARRAY(self));
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                            "can only place a scalar for an "
                            " array of size 1");
            return NULL;
        }
    }
    else if (n != PyArray_NDIM(self) && (n > 1 || PyArray_NDIM(self) == 0)) {
        PyErr_SetString(PyExc_ValueError,
                        "incorrect number of indices for "      \
                        "array");
        return NULL;
    }
    else if (n == 1) { /* allows for flat setting as well as 1-d case */
        intp value, loc, index, factor;
        intp factors[MAX_DIMS];
        PyObject *indobj;

        indobj = PyTuple_GET_ITEM(args, 0);
        if (PyTuple_Check(indobj)) {
            PyObject *res;
            PyObject *newargs;
            PyObject *tmp;
            int i, nn;
            nn = PyTuple_GET_SIZE(indobj);
            newargs = PyTuple_New(nn+1);
            Py_INCREF(obj);
            for (i = 0; i < nn; i++) {
                tmp = PyTuple_GET_ITEM(indobj, i);
                Py_INCREF(tmp);
                PyTuple_SET_ITEM(newargs, i, tmp);
            }
            PyTuple_SET_ITEM(newargs, nn, obj);
            /* Call with a converted set of arguments */
            res = array_setscalar(self, newargs);
            Py_DECREF(newargs);
            return res;
        }
        value = PyArray_PyIntAsIntp(indobj);
        if (error_converting(value)) {
            PyErr_SetString(PyExc_ValueError, "invalid integer");
            return NULL;
        }
        if (value >= PyArray_SIZE(self)) {
            PyErr_SetString(PyExc_ValueError,
                            "index out of bounds");
            return NULL;
        }
        if (PyArray_NDIM(self) == 1) {
            value *= PyArray_STRIDE(self, 0);
            ret = PyArray_DESCR(self)->f->setitem(obj, PyArray_BYTES(self) + value,
                                                  PyArray_ARRAY(self));
            goto finish;
        }
        nd = PyArray_NDIM(self);
        factor = 1;
        while (nd--) {
            factors[nd] = factor;
            factor *= PyArray_DIM(self, nd);
        }
        loc = 0;
        for (nd = 0; nd < PyArray_NDIM(self); nd++) {
            index = value / factors[nd];
            value = value % factors[nd];
            loc += PyArray_STRIDE(self, nd)*index;
        }

        ret = PyArray_DESCR(self)->f->setitem(obj, PyArray_BYTES(self) + loc, 
                                              PyArray_ARRAY(self));
    }
    else {
        intp loc, index[MAX_DIMS];
        PyObject *tupargs;
        tupargs = PyTuple_GetSlice(args, 0, n);
        nd = PyArray_IntpFromSequence(tupargs, index, MAX_DIMS);
        Py_DECREF(tupargs);
        if (nd < n) {
            return NULL;
        }
        loc = 0;
        while (nd--) {
            if (index[nd] < 0) {
                index[nd] += PyArray_DIM(self, nd);
            }
            if (index[nd] < 0 ||
                index[nd] >= PyArray_DIM(self, nd)) {
                PyErr_SetString(PyExc_ValueError,
                                "index out of bounds");
                return NULL;
            }
            loc += PyArray_STRIDE(self, nd)*index[nd];
        }
        ret = PyArray_DESCR(self)->f->setitem(obj, PyArray_BYTES(self) + loc, 
                                              PyArray_ARRAY(self));
    }

 finish:
    if (ret < 0) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
array_cast(PyArrayObject *self, PyObject *args)
{
    PyArray_Descr *descr = NULL;
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "O&", PyArray_DescrConverter,
                          &descr)) {
        Py_XDECREF(descr);
        return NULL;
    }

    if (PyArray_EquivTypes(descr, PyArray_Descr_WRAP( PyArray_DESCR(self) ))) {
        obj = _ARET(PyArray_NewCopy(self,NPY_ANYORDER));
        Py_XDECREF(descr);
        return obj;
    }
    if (descr->descr->names != NULL) {
        int flags;
        flags = NPY_FORCECAST;
        if (PyArray_ISFORTRAN(self)) {
            flags |= NPY_FORTRAN;
        }
        return PyArray_FromArray(self, descr, flags);
    }
    return PyArray_CastToType(self, descr, PyArray_ISFORTRAN(self));
}

/* default sub-type implementation */


static PyObject *
array_wraparray(PyArrayObject *self, PyObject *args)
{
    PyObject *arr;

    if (PyTuple_Size(args) < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "only accepts 1 argument");
        return NULL;
    }
    arr = PyTuple_GET_ITEM(args, 0);
    if (arr == NULL) {
        return NULL;
    }
    if (!PyArray_Check(arr)) {
        PyErr_SetString(PyExc_TypeError,
                        "can only be called with ndarray object");
        return NULL;
    }

    if (Py_TYPE(self) != Py_TYPE(arr)){
        Npy_INCREF(PyArray_DESCR(arr));
        RETURN_PYARRAY(NpyArray_NewView(PyArray_DESCR(arr),
                                        PyArray_NDIM(arr),
                                        PyArray_DIMS(arr),
                                        PyArray_STRIDES(arr),
                                        PyArray_ARRAY(arr), 0,
                                        NPY_FALSE));
    } else {
        /*The type was set in __array_prepare__*/
        Py_INCREF(arr);
        return arr;
    }
}


static PyObject *
array_preparearray(PyArrayObject *self, PyObject *args)
{
    PyObject *arr;
    PyArrayObject *ret;

    if (PyTuple_Size(args) < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "only accepts 1 argument");
        return NULL;
    }
    arr = PyTuple_GET_ITEM(args, 0);
    if (!PyArray_Check(arr)) {
        PyErr_SetString(PyExc_TypeError,
                        "can only be called with ndarray object");
        return NULL;
    }

    Npy_INCREF(PyArray_DESCR(arr));
    ASSIGN_TO_PYARRAY(ret,
        NpyArray_NewFromDescr(PyArray_DESCR(arr),
                              PyArray_NDIM(arr),
                              PyArray_DIMS(arr),
                              PyArray_STRIDES(arr), PyArray_DATA(arr),
                              PyArray_FLAGS(arr), NPY_FALSE,
                              Py_TYPE(self), self));
    if (ret == NULL) {
        return NULL;
    }
    PyArray_BASE_ARRAY(ret) = PyArray_ARRAY(arr);
    Npy_INCREF(PyArray_BASE_ARRAY(ret));

    return (PyObject *)ret;
}


static PyObject *
array_getarray(PyArrayObject *self, PyObject *args)
{
    PyArray_Descr *newtype = NULL;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "|O&", PyArray_DescrConverter,
                          &newtype)) {
        Py_XDECREF(newtype);
        return NULL;
    }

    /* convert to PyArray_Type */
    if (!PyArray_CheckExact(self)) {
        PyArrayObject *new;
        Npy_INCREF(PyArray_DESCR(self));
        ASSIGN_TO_PYARRAY(new,
                          NpyArray_NewView(PyArray_DESCR(self),
                                           PyArray_NDIM(self),
                                           PyArray_DIMS(self),
                                           PyArray_STRIDES(self),
                                           PyArray_ARRAY(self), 0,
                                           NPY_TRUE));
        self = new;
    }
    else {
        Py_INCREF(self);
    }

    if ((newtype == NULL) ||
        PyArray_EquivTypes(PyArray_Descr_WRAP(PyArray_DESCR(self)), newtype)) {
        return (PyObject *)self;
    }
    else {
        ret = PyArray_CastToType(self, newtype, 0);
        Py_DECREF(self);
        return ret;
    }
}


static PyObject *
array_copy(PyArrayObject *self, PyObject *args)
{
    PyArray_ORDER fortran=PyArray_CORDER;
    if (!PyArg_ParseTuple(args, "|O&", PyArray_OrderConverter,
                          &fortran)) {
        return NULL;
    }

    return PyArray_NewCopy(self, fortran);
}

#include <stdio.h>
static PyObject *
array_resize(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"refcheck", NULL};
    Py_ssize_t size = PyTuple_Size(args);
    int refcheck = 1;
    PyArray_Dims newshape;
    PyObject *ret, *obj;


    if (!NpyArg_ParseKeywords(kwds, "|i", kwlist,  &refcheck)) {
        return NULL;
    }

    if (size == 0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else if (size == 1) {
        obj = PyTuple_GET_ITEM(args, 0);
        if (obj == Py_None) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        args = obj;
    }
    if (!PyArray_IntpConverter(args, &newshape)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError, "invalid shape");
        }
        return NULL;
    }

    ret = PyArray_Resize(self, &newshape, refcheck, PyArray_CORDER);
    PyDimMem_FREE(newshape.ptr);
    if (ret == NULL) {
        return NULL;
    }
    Py_DECREF(ret);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
array_repeat(PyArrayObject *self, PyObject *args, PyObject *kwds) {
    PyObject *repeats;
    int axis = MAX_DIMS;
    static char *kwlist[] = {"repeats", "axis", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O&", kwlist,
                                     &repeats, PyArray_AxisConverter,
                                     &axis)) {
        return NULL;
    }
    return _ARET(PyArray_Repeat(self, repeats, axis));
}

static PyObject *
array_choose(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"out", "mode", NULL};
    PyObject *choices;
    PyArrayObject *out = NULL;
    NPY_CLIPMODE clipmode = NPY_RAISE;
    Py_ssize_t n = PyTuple_Size(args);

    if (n <= 1) {
        if (!PyArg_ParseTuple(args, "O", &choices)) {
            return NULL;
        }
    }
    else {
        choices = args;
    }

    if (!NpyArg_ParseKeywords(kwds, "|O&O&", keywords,
                PyArray_OutputConverter, &out,
                PyArray_ClipmodeConverter, &clipmode)) {
        return NULL;
    }

    return _ARET(PyArray_Choose(self, choices, out, clipmode));
}

static PyObject *
array_sort(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis=-1;
    int val;
    PyArray_SORTKIND which = PyArray_QUICKSORT;
    PyObject *order = NULL;
    PyArray_Descr *saved = NULL;
    static char *kwlist[] = {"axis", "kind", "order", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iO&O", kwlist, &axis,
                                     PyArray_SortkindConverter, &which,
                                     &order)) {
        return NULL;
    }
    if (order == Py_None) {
        order = NULL;
    }
    if (order != NULL) {
        NpyArray_Descr *newd;
        PyObject *new_name;
        PyObject *_numpy_internal;
        saved = PyArray_Descr_WRAP(PyArray_DESCR(self));
        if (saved->descr->names == NULL) {
            PyErr_SetString(PyExc_ValueError, "Cannot specify " \
                            "order when the array has no fields.");
            return NULL;
        }
        _numpy_internal = PyImport_ImportModule("numpy.core._internal");
        if (_numpy_internal == NULL) {
            return NULL;
        }
        new_name = PyObject_CallMethod(_numpy_internal, "_newnames",
                                       "OO", saved, order);
        Py_DECREF(_numpy_internal);
        if (new_name == NULL) {
            return NULL;
        }
        
        /* Notice, switching from PyArrayDescr to NpyArrayDescr usage */
        newd = NpyArray_DescrNew(saved->descr);
        NpyArray_DescrSetNames(newd, arraydescr_seq_to_nameslist(new_name));
        PyArray_DESCR(self) = newd;
        Py_DECREF(new_name);
    }

    val = PyArray_Sort(self, axis, which);
    if (order != NULL) {
        Npy_XDECREF(PyArray_DESCR(self));
        PyArray_DESCR(self) = saved->descr;
    }
    if (val < 0) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
array_argsort(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = -1;
    PyArray_SORTKIND which = PyArray_QUICKSORT;
    PyObject *order = NULL, *res;
    NpyArray_Descr *newd, *saved=NULL;
    static char *kwlist[] = {"axis", "kind", "order", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O", kwlist,
                                     PyArray_AxisConverter, &axis,
                                     PyArray_SortkindConverter, &which,
                                     &order)) {
        return NULL;
    }
    if (order == Py_None) {
        order = NULL;
    }
    if (order != NULL) {
        PyObject *new_name;
        PyObject *_numpy_internal;
        char** new_names;
        int result;
        saved = PyArray_DESCR(self);
        if (saved->names == NULL) {
            PyErr_SetString(PyExc_ValueError, "Cannot specify " \
                            "order when the array has no fields.");
            return NULL;
        }
        _numpy_internal = PyImport_ImportModule("numpy.core._internal");
        if (_numpy_internal == NULL) {
            return NULL;
        }
        new_name = PyObject_CallMethod(_numpy_internal, "_newnames",
                                       "OO", Npy_INTERFACE(saved), order);
        Py_DECREF(_numpy_internal);
        if (new_name == NULL) {
            return NULL;
        }
        newd = NpyArray_DescrNew(saved);
        new_names = arraydescr_seq_to_nameslist(new_name);
        result = NpyArray_DescrReplaceNames(newd, new_names);
        /* The replace should not fail. */
        assert(result != 0);
        PyArray_DESCR(self) = newd;
        Py_DECREF(new_name);
    }

    res = PyArray_ArgSort(self, axis, which);
    if (order != NULL) {
        Npy_XDECREF(PyArray_DESCR(self));
        PyArray_DESCR(self) = saved;
    }
    return _ARET(res);
}

static PyObject *
array_searchsorted(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"keys", "side", NULL};
    PyObject *keys;
    NPY_SEARCHSIDE side = NPY_SEARCHLEFT;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O&:searchsorted",
                                     kwlist, &keys,
                                     PyArray_SearchsideConverter, &side)) {
        return NULL;
    }
    return _ARET(PyArray_SearchSorted(self, keys, side));
}

static void
_deepcopy_call(char *iptr, char *optr, NpyArray_Descr *dtype,
               PyObject *deepcopy, PyObject *visit)
{
    if (!NpyDataType_REFCHK(dtype)) {
        return;
    }
    else if (NpyDataType_HASFIELDS(dtype)) {
        const char *key = NULL;
        NpyArray_DescrField *value;
        NpyDict_Iter pos;
        
        NpyDict_IterInit(&pos);
        while (NpyDict_IterNext(dtype->fields, &pos, (void **)&key, (void **)&value)) {
            if (NULL != value->title && !strcmp(key, value->title)) {
                continue;
            }
            _deepcopy_call(iptr + value->offset, optr + value->offset, value->descr,
                           deepcopy, visit);
        }
    }
    else {
        PyObject *itemp, *otemp;
        PyObject *res;
        NPY_COPY_VOID_PTR(&itemp, iptr);
        NPY_COPY_VOID_PTR(&otemp, optr);
        Py_XINCREF(itemp);
        /* call deepcopy on this argument */
        res = PyObject_CallFunctionObjArgs(deepcopy, itemp, visit, NULL);
        Py_XDECREF(itemp);
        Py_XDECREF(otemp);
        NPY_COPY_VOID_PTR(optr, &res);
    }

}


static PyObject *
array_deepcopy(PyArrayObject *self, PyObject *args)
{
    PyObject* visit;
    char *optr;
    NpyArrayIterObject *it;
    PyObject *copy, *ret, *deepcopy;

    if (!PyArg_ParseTuple(args, "O", &visit)) {
        return NULL;
    }
    ret = PyArray_Copy(self);
    if (NpyDataType_REFCHK(PyArray_DESCR(self))) {
        copy = PyImport_ImportModule("copy");
        if (copy == NULL) {
            return NULL;
        }
        deepcopy = PyObject_GetAttrString(copy, "deepcopy");
        Py_DECREF(copy);
        if (deepcopy == NULL) {
            return NULL;
        }
        it = NpyArray_IterNew(PyArray_ARRAY(self));
        if (it == NULL) {
            Py_DECREF(deepcopy);
            return NULL;
        }
        optr = PyArray_DATA(ret);
        while(it->index < it->size) {
            _deepcopy_call(it->dataptr, optr, PyArray_DESCR(self), deepcopy, visit);
            optr += PyArray_ITEMSIZE(self);
            NpyArray_ITER_NEXT(it);
        }
        Py_DECREF(deepcopy);
        Npy_DECREF(it);
    }
    return _ARET(ret);
}

/* Convert Array to flat list (using getitem) */
static PyObject *
_getlist_pkl(PyArrayObject *self)
{
    PyObject *theobject;
    NpyArrayIterObject *iter = NULL;
    PyObject *list;
    PyArray_GetItemFunc *getitem;

    getitem = PyArray_DESCR(self)->f->getitem;
    iter = NpyArray_IterNew(PyArray_ARRAY(self));
    if (iter == NULL) {
        return NULL;
    }
    list = PyList_New(iter->size);
    if (list == NULL) {
        Npy_DECREF(iter);
        return NULL;
    }
    while (iter->index < iter->size) {
        theobject = getitem(iter->dataptr, PyArray_ARRAY(self));
        PyList_SET_ITEM(list, (int) iter->index, theobject);
        NpyArray_ITER_NEXT(iter);
    }
    Npy_DECREF(iter);
    return list;
}

static int
_setlist_pkl(PyArrayObject *self, PyObject *list)
{
    PyObject *theobject;
    NpyArrayIterObject *iter = NULL;
    PyArray_SetItemFunc *setitem;

    setitem = PyArray_DESCR(self)->f->setitem;
    iter = NpyArray_IterNew(PyArray_ARRAY(self));
    if (iter == NULL) {
        return -1;
    }
    while(iter->index < iter->size) {
        theobject = PyList_GET_ITEM(list, (int) iter->index);
        setitem(theobject, iter->dataptr, PyArray_ARRAY(self));
        NpyArray_ITER_NEXT(iter);
    }
    Npy_XDECREF(iter);
    return 0;
}


static PyObject *
array_reduce(PyArrayObject *self, PyObject *NPY_UNUSED(args))
{
    /* version number of this pickle type. Increment if we need to
       change the format. Be sure to handle the old versions in
       array_setstate. */
    const int version = 1;
    PyObject *ret = NULL, *state = NULL, *obj = NULL, *mod = NULL;
    PyObject *mybool, *thestr = NULL;
    PyArray_Descr *descr;

    /* Return a tuple of (callable object, arguments, object's state) */
    /*  We will put everything in the object's state, so that on UnPickle
        it can use the string object as memory without a copy */

    ret = PyTuple_New(3);
    if (ret == NULL) {
        return NULL;
    }
    mod = PyImport_ImportModule("numpy.core.multiarray");
    if (mod == NULL) {
        Py_DECREF(ret);
        return NULL;
    }
    obj = PyObject_GetAttrString(mod, "_reconstruct");
    Py_DECREF(mod);
    PyTuple_SET_ITEM(ret, 0, obj);
    PyTuple_SET_ITEM(ret, 1,
                     Py_BuildValue("ONc",
                                   (PyObject *)Py_TYPE(self),
                                   Py_BuildValue("(N)",
                                                 PyInt_FromLong(0)),
                                   /* dummy data-type */
                                   'b'));

    /* Now fill in object's state.  This is a tuple with
       5 arguments

       1) an integer with the pickle version.
       2) a Tuple giving the shape
       3) a PyArray_Descr Object (with correct bytorder set)
       4) a Bool stating if Fortran or not
       5) a Python object representing the data (a string, or
       a list or any user-defined object).

       Notice because Python does not describe a mechanism to write
       raw data to the pickle, this performs a copy to a string first
    */

    state = PyTuple_New(5);
    if (state == NULL) {
        Py_DECREF(ret);
        return NULL;
    }
    PyTuple_SET_ITEM(state, 0, PyInt_FromLong(version));
    PyTuple_SET_ITEM(state, 1, PyObject_GetAttrString((PyObject *)self,
                                                      "shape"));
    descr = (PyArray_Descr *)Npy_INTERFACE( PyArray_DESCR(self) );
    Py_INCREF(descr);
    PyTuple_SET_ITEM(state, 2, (PyObject *)descr);
    mybool = (PyArray_ISFORTRAN(self) ? Py_True : Py_False);
    Py_INCREF(mybool);
    PyTuple_SET_ITEM(state, 3, mybool);
    if (NpyDataType_FLAGCHK(PyArray_DESCR(self), NPY_LIST_PICKLE)) {
        thestr = _getlist_pkl(self);
    }
    else {
        thestr = PyArray_ToString(self, NPY_ANYORDER);
    }
    if (thestr == NULL) {
        Py_DECREF(ret);
        Py_DECREF(state);
        return NULL;
    }
    PyTuple_SET_ITEM(state, 4, thestr);
    PyTuple_SET_ITEM(ret, 2, state);
    return ret;
}

static PyObject *
array_setstate(PyArrayObject *self, PyObject *args)
{
    PyObject *shape;
    PyArray_Descr *typecode;
    int version = 1;
    int fortran;
    PyObject *rawdata;
    char *datastr;
    Py_ssize_t len;
    intp size, dimensions[MAX_DIMS];
    int nd;
    int incref_base = 1;

    /* This will free any memory associated with a and
       use the string in setstate as the (writeable) memory.
    */
    if (!PyArg_ParseTuple(args, "(iO!O!iO)", &version, &PyTuple_Type,
                          &shape, &PyArrayDescr_Type, &typecode,
                          &fortran, &rawdata)) {
        PyErr_Clear();
        version = 0;
        if (!PyArg_ParseTuple(args, "(O!O!iO)", &PyTuple_Type,
                              &shape, &PyArrayDescr_Type, &typecode,
                              &fortran, &rawdata)) {
            return NULL;
        }
    }

    /* If we ever need another pickle format, increment the version
       number. But we should still be able to handle the old versions.
       We've only got one right now. */
    if (version != 1 && version != 0) {
        PyErr_Format(PyExc_ValueError,
                     "can't handle version %d of numpy.ndarray pickle",
                     version);
        return NULL;
    }

    Npy_XDECREF(PyArray_DESCR(self));
    PyArray_DESCR(self) = typecode->descr;
    Npy_INCREF(typecode->descr);
    nd = PyArray_IntpFromSequence(shape, dimensions, MAX_DIMS);
    if (nd < 0) {
        return NULL;
    }
    size = PyArray_MultiplyList(dimensions, nd);
    if (PyArray_ITEMSIZE(self) == 0) {
        PyErr_SetString(PyExc_ValueError, "Invalid data-type size.");
        return NULL;
    }
    if (size < 0 || size > MAX_INTP / PyArray_ITEMSIZE(self)) {
        PyErr_NoMemory();
        return NULL;
    }

    if (PyDataType_FLAGCHK(typecode, NPY_LIST_PICKLE)) {
        if (!PyList_Check(rawdata)) {
            PyErr_SetString(PyExc_TypeError,
                            "object pickle not returning list");
            return NULL;
        }
    }
    else {
#if defined(NPY_PY3K)
        /* Backward compatibility with Python 2 Numpy pickles */
        if (PyUnicode_Check(rawdata)) {
            PyObject *tmp;
            tmp = PyUnicode_AsLatin1String(rawdata);
            rawdata = tmp;
            incref_base = 0;
        }
#endif

        if (!PyBytes_Check(rawdata)) {
            PyErr_SetString(PyExc_TypeError,
                            "pickle not returning string");
            return NULL;
        }

        if (PyBytes_AsStringAndSize(rawdata, &datastr, &len))
            return NULL;

        if ((len != (PyArray_ITEMSIZE(self) * size))) {
            PyErr_SetString(PyExc_ValueError,
                            "buffer size does not"  \
                            " match array size");
            return NULL;
        }
    }

    if ((PyArray_FLAGS(self) & OWNDATA)) {
        if (PyArray_BYTES(self) != NULL) {
            PyDataMem_FREE(PyArray_BYTES(self));
        }
        PyArray_FLAGS(self) &= ~OWNDATA;
    }
    Npy_XDECREF(PyArray_BASE_ARRAY(self));
    Py_XDECREF(PyArray_BASE(self));
    PyArray_BASE_ARRAY(self) = NULL;
    PyArray_BASE(self) = NULL;

    PyArray_FLAGS(self) &= ~UPDATEIFCOPY;

    if (PyArray_DIMS(self) != NULL) {
        PyDimMem_FREE(PyArray_DIMS(self));
        PyArray_DIMS(self) = NULL;
    }

    PyArray_FLAGS(self) = DEFAULT;

    PyArray_NDIM(self) = nd;

    if (nd > 0) {
        PyArray_DIMS(self) = PyDimMem_NEW(nd * 2);
        PyArray_STRIDES(self) = PyArray_DIMS(self) + nd;
        memcpy(PyArray_DIMS(self), dimensions, sizeof(intp)*nd);
        (void) npy_array_fill_strides(PyArray_STRIDES(self), dimensions, nd,
                                      (size_t) PyArray_ITEMSIZE(self),
                                      (fortran ? FORTRAN : CONTIGUOUS),
                                      &(PyArray_FLAGS(self)));
    }

    if (!PyDataType_FLAGCHK(typecode, NPY_LIST_PICKLE)) {
        int swap=!PyArray_ISNOTSWAPPED(self);
        PyArray_BYTES(self) = datastr;
        if (!_IsAligned(self) || swap) {
            intp num = PyArray_NBYTES(self);
            PyArray_BYTES(self) = PyDataMem_NEW(num);
            if (PyArray_BYTES(self) == NULL) {
                PyArray_NDIM(self) = 0;
                PyDimMem_FREE(PyArray_DIMS(self));
                return PyErr_NoMemory();
            }
            if (swap) { /* byte-swap on pickle-read */
                intp numels = num / PyArray_ITEMSIZE(self);
                PyArray_DESCR(self)->f->copyswapn(PyArray_BYTES(self), PyArray_ITEMSIZE(self),
                                          datastr, PyArray_ITEMSIZE(self),
                                          numels, 1, PyArray_ARRAY(self));
                if (!PyArray_ISEXTENDED(self)) {
                    PyArray_DESCR(self) = NpyArray_DescrFromType(PyArray_TYPE(self));
                }
                else {
                    PyArray_DESCR(self) = NpyArray_DescrNew(typecode->descr);
                    if (PyArray_DESCR(self)->byteorder == PyArray_BIG) {
                        PyArray_DESCR(self)->byteorder = PyArray_LITTLE;
                    }
                    else if (PyArray_DESCR(self)->byteorder == PyArray_LITTLE) {
                        PyArray_DESCR(self)->byteorder = PyArray_BIG;
                    }
                }
                Py_DECREF(typecode);
            }
            else {
                memcpy(PyArray_BYTES(self), datastr, num);
            }
            PyArray_FLAGS(self) |= OWNDATA;
            PyArray_BASE_ARRAY(self) = NULL;
            PyArray_BASE(self) = NULL;
        }
        else {
            PyArray_BASE(self) = rawdata;
            if (incref_base) {
                Py_INCREF(PyArray_BASE(self));
            }
            ASSERT_ONE_BASE(self);
        }
    }
    else {
        PyArray_BYTES(self) = PyDataMem_NEW(PyArray_NBYTES(self));
        if (PyArray_BYTES(self) == NULL) {
            PyArray_NDIM(self) = 0;
            PyArray_BYTES(self) = PyDataMem_NEW(PyArray_ITEMSIZE(self));
            if (PyArray_DIMS(self)) {
                PyDimMem_FREE(PyArray_DIMS(self));
            }
            return PyErr_NoMemory();
        }
        if (NpyDataType_FLAGCHK(PyArray_DESCR(self), NPY_NEEDS_INIT)) {
            memset(PyArray_BYTES(self), 0, PyArray_NBYTES(self));
        }
        PyArray_FLAGS(self) |= OWNDATA;
        PyArray_BASE_ARRAY(self) = NULL;
        PyArray_BASE(self) = NULL;
        if (_setlist_pkl(self, rawdata) < 0) {
            return NULL;
        }
    }

    PyArray_UpdateFlags(self, UPDATE_ALL);

    Py_INCREF(Py_None);
    return Py_None;
}

/*NUMPY_API*/
NPY_NO_EXPORT int
PyArray_Dump(PyObject *self, PyObject *file, int protocol)
{
    PyObject *cpick = NULL;
    PyObject *ret;
    if (protocol < 0) {
        protocol = 2;
    }

#if defined(NPY_PY3K)
    cpick = PyImport_ImportModule("pickle");
#else
    cpick = PyImport_ImportModule("cPickle");
#endif
    if (cpick == NULL) {
        return -1;
    }
    if (PyBytes_Check(file) || PyUnicode_Check(file)) {
        file = npy_PyFile_OpenFile(file, "wb");
        if (file == NULL) {
            return -1;
        }
    }
    else {
        Py_INCREF(file);
    }
    ret = PyObject_CallMethod(cpick, "dump", "OOi", self, file, protocol);
    Py_XDECREF(ret);
    Py_DECREF(file);
    Py_DECREF(cpick);
    if (PyErr_Occurred()) {
        return -1;
    }
    return 0;
}

/*NUMPY_API*/
NPY_NO_EXPORT PyObject *
PyArray_Dumps(PyObject *self, int protocol)
{
    PyObject *cpick = NULL;
    PyObject *ret;
    if (protocol < 0) {
        protocol = 2;
    }
#if defined(NPY_PY3K)
    cpick = PyImport_ImportModule("pickle");
#else
    cpick = PyImport_ImportModule("cPickle");
#endif
    if (cpick == NULL) {
        return NULL;
    }
    ret = PyObject_CallMethod(cpick, "dumps", "Oi", self, protocol);
    Py_DECREF(cpick);
    return ret;
}


static PyObject *
array_dump(PyArrayObject *self, PyObject *args)
{
    PyObject *file = NULL;
    int ret;

    if (!PyArg_ParseTuple(args, "O", &file)) {
        return NULL;
    }
    ret = PyArray_Dump((PyObject *)self, file, 2);
    if (ret < 0) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
array_dumps(PyArrayObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }
    return PyArray_Dumps((PyObject *)self, 2);
}


static PyObject *
array_transpose(PyArrayObject *self, PyObject *args)
{
    PyObject *shape = Py_None;
    Py_ssize_t n = PyTuple_Size(args);
    PyArray_Dims permute;
    PyObject *ret;

    if (n > 1) {
        shape = args;
    }
    else if (n == 1) {
        shape = PyTuple_GET_ITEM(args, 0);
    }

    if (shape == Py_None) {
        ret = PyArray_Transpose(self, NULL);
    }
    else {
        if (!PyArray_IntpConverter(shape, &permute)) {
            return NULL;
        }
        ret = PyArray_Transpose(self, &permute);
        PyDimMem_FREE(permute.ptr);
    }

    return ret;
}

/* Return typenumber from dtype2 unless it is NULL, then return
   NPY_DOUBLE if dtype1->type_num is integer or bool
   and dtype1->type_num otherwise.
*/
static int
_get_type_num_double(NpyArray_Descr *dtype1, NpyArray_Descr *dtype2)
{
    if (dtype2 != NULL) {
        return dtype2->type_num;
    }
    /* For integer or bool data-types */
    if (dtype1->type_num < NPY_FLOAT) {
        return NPY_DOUBLE;
    }
    else {
        return dtype1->type_num;
    }
}

#define _CHKTYPENUM(typ) ((typ) ? (typ)->descr->type_num : PyArray_NOTYPE)

static PyObject *
array_mean(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int num;
    static char *kwlist[] = {"axis", "dtype", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    num = _get_type_num_double(PyArray_DESCR(self), (NULL != dtype) ? dtype->descr : NULL);
    Py_XDECREF(dtype);
    return PyArray_Mean(self, axis, num, out);
}

static PyObject *
array_sum(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int rtype;
    static char *kwlist[] = {"axis", "dtype", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    rtype = _CHKTYPENUM(dtype);
    Py_XDECREF(dtype);
    return PyArray_Sum(self, axis, rtype, out);
}


static PyObject *
array_cumsum(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int rtype;
    static char *kwlist[] = {"axis", "dtype", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    rtype = _CHKTYPENUM(dtype);
    Py_XDECREF(dtype);
    return PyArray_CumSum(self, axis, rtype, out);
}

static PyObject *
array_prod(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int rtype;
    static char *kwlist[] = {"axis", "dtype", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    rtype = _CHKTYPENUM(dtype);
    Py_XDECREF(dtype);
    return PyArray_Prod(self, axis, rtype, out);
}

static PyObject *
array_cumprod(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int rtype;
    static char *kwlist[] = {"axis", "dtype", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    rtype = _CHKTYPENUM(dtype);
    Py_XDECREF(dtype);
    return PyArray_CumProd(self, axis, rtype, out);
}


static PyObject *
array_dot(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *b;
    static PyObject *numpycore = NULL;

    if (!PyArg_ParseTuple(args, "O", &b)) {
        return NULL;
    }

    /* Since blas-dot is exposed only on the Python side, we need to grab it
     * from there */
    if (numpycore == NULL) {
        numpycore = PyImport_ImportModule("numpy.core");
        if (numpycore == NULL) {
            return NULL;
        }
    }

    return PyObject_CallMethod(numpycore, "dot", "OO", self, b);
}


static PyObject *
array_any(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return PyArray_Any(self, axis, out);
}


static PyObject *
array_all(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&", kwlist,
                                     PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out))
        return NULL;

    return PyArray_All(self, axis, out);
}


static PyObject *
array_stddev(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int num;
    int ddof = 0;
    static char *kwlist[] = {"axis", "dtype", "out", "ddof", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&i", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out, &ddof)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    num = _get_type_num_double(PyArray_DESCR(self), (NULL != dtype) ? dtype->descr : NULL);
    Py_XDECREF(dtype);
    return __New_PyArray_Std(self, axis, num, out, 0, ddof);
}


static PyObject *
array_variance(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int num;
    int ddof = 0;
    static char *kwlist[] = {"axis", "dtype", "out", "ddof", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&O&i", kwlist,
                                     PyArray_AxisConverter,
                                     &axis, PyArray_DescrConverter2,
                                     &dtype,
                                     PyArray_OutputConverter,
                                     &out, &ddof)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    num = _get_type_num_double(PyArray_DESCR(self), (NULL != dtype) ? dtype->descr : NULL);
    Py_XDECREF(dtype);
    return __New_PyArray_Std(self, axis, num, out, 1, ddof);
}


static PyObject *
array_compress(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis = MAX_DIMS;
    PyObject *condition;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"condition", "axis", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O&O&", kwlist,
                                     &condition, PyArray_AxisConverter,
                                     &axis,
                                     PyArray_OutputConverter,
                                     &out)) {
        return NULL;
    }
    return _ARET(PyArray_Compress(self, condition, axis, out));
}


static PyObject *
array_nonzero(PyArrayObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }
    return PyArray_Nonzero(self);
}


static PyObject *
array_trace(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis1 = 0, axis2 = 1, offset = 0;
    PyArray_Descr *dtype = NULL;
    PyArrayObject *out = NULL;
    int rtype;
    static char *kwlist[] = {"offset", "axis1", "axis2", "dtype", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iiiO&O&", kwlist,
                                     &offset, &axis1, &axis2,
                                     PyArray_DescrConverter2, &dtype,
                                     PyArray_OutputConverter, &out)) {
        Py_XDECREF(dtype);
        return NULL;
    }

    rtype = _CHKTYPENUM(dtype);
    Py_XDECREF(dtype);
    return _ARET(PyArray_Trace(self, offset, axis1, axis2, rtype, out));
}

#undef _CHKTYPENUM


static PyObject *
array_clip(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *min = NULL, *max = NULL;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"min", "max", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO&", kwlist,
                                     &min, &max,
                                     PyArray_OutputConverter,
                                     &out)) {
        return NULL;
    }
    if (max == NULL && min == NULL) {
        PyErr_SetString(PyExc_ValueError, "One of max or min must be given.");
        return NULL;
    }
    return _ARET(PyArray_Clip(self, min, max, out));
}


static PyObject *
array_conjugate(PyArrayObject *self, PyObject *args)
{

    PyArrayObject *out = NULL;
    if (!PyArg_ParseTuple(args, "|O&",
                          PyArray_OutputConverter,
                          &out)) {
        return NULL;
    }
    return PyArray_Conjugate(self, out);
}


static PyObject *
array_diagonal(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int axis1 = 0, axis2 = 1, offset = 0;
    static char *kwlist[] = {"offset", "axis1", "axis2", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist,
                                     &offset, &axis1, &axis2)) {
        return NULL;
    }
    return _ARET(PyArray_Diagonal(self, offset, axis1, axis2));
}


static PyObject *
array_flatten(PyArrayObject *self, PyObject *args)
{
    PyArray_ORDER fortran = PyArray_CORDER;

    if (!PyArg_ParseTuple(args, "|O&", PyArray_OrderConverter, &fortran)) {
        return NULL;
    }
    return PyArray_Flatten(self, fortran);
}


static PyObject *
array_ravel(PyArrayObject *self, PyObject *args)
{
    PyArray_ORDER fortran = PyArray_CORDER;

    if (!PyArg_ParseTuple(args, "|O&", PyArray_OrderConverter,
                          &fortran)) {
        return NULL;
    }
    return PyArray_Ravel(self, fortran);
}


static PyObject *
array_round(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    int decimals = 0;
    PyArrayObject *out = NULL;
    static char *kwlist[] = {"decimals", "out", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iO&", kwlist,
                                     &decimals, PyArray_OutputConverter,
                                     &out)) {
        return NULL;
    }
    return _ARET(PyArray_Round(self, decimals, out));
}



static PyObject *
array_setflags(PyArrayObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"write", "align", "uic", NULL};
    PyObject *write = Py_None;
    PyObject *align = Py_None;
    PyObject *uic = Py_None;
    int flagback = PyArray_FLAGS(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", kwlist,
                                     &write, &align, &uic))
        return NULL;

    if (align != Py_None) {
        if (PyObject_Not(align)) {
            PyArray_FLAGS(self) &= ~ALIGNED;
        }
        else if (_IsAligned(self)) {
            PyArray_FLAGS(self) |= ALIGNED;
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                            "cannot set aligned flag of mis-"\
                            "aligned array to True");
            return NULL;
        }
    }

    if (uic != Py_None) {
        if (PyObject_IsTrue(uic)) {
            PyArray_FLAGS(self) = flagback;
            PyErr_SetString(PyExc_ValueError,
                            "cannot set UPDATEIFCOPY "       \
                            "flag to True");
            return NULL;
        }
        else {
            PyArray_FLAGS(self) &= ~UPDATEIFCOPY;
            Py_XDECREF(PyArray_BASE(self));
            PyArray_BASE(self) = NULL;
            Npy_XDECREF(PyArray_BASE_ARRAY(self));
            PyArray_BASE_ARRAY(self) = NULL;
        }
    }

    if (write != Py_None) {
        if (PyObject_IsTrue(write))
            if (_IsWriteable(self)) {
                PyArray_FLAGS(self) |= WRITEABLE;
            }
            else {
                PyArray_FLAGS(self) = flagback;
                PyErr_SetString(PyExc_ValueError,
                                "cannot set WRITEABLE " \
                                "flag to True of this " \
                                "array");               \
                return NULL;
            }
        else
            PyArray_FLAGS(self) &= ~WRITEABLE;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
array_newbyteorder(PyArrayObject *self, PyObject *args)
{
    char endian = PyArray_SWAP;
    PyArray_Descr *new;

    if (!PyArg_ParseTuple(args, "|O&", PyArray_ByteorderConverter,
                          &endian)) {
        return NULL;
    }
    new = PyArray_DescrNewByteorder( Npy_INTERFACE( PyArray_DESCR(self) ), endian);
    if (!new) {
        return NULL;
    }
    return PyArray_View(self, new, NULL);

}

NPY_NO_EXPORT PyMethodDef array_methods[] = {

    /* for subtypes */
    {"__array__",
        (PyCFunction)array_getarray,
        METH_VARARGS, NULL},
    {"__array_prepare__",
        (PyCFunction)array_preparearray,
        METH_VARARGS, NULL},
    {"__array_wrap__",
        (PyCFunction)array_wraparray,
        METH_VARARGS, NULL},

    /* for the copy module */
    {"__copy__",
        (PyCFunction)array_copy,
        METH_VARARGS, NULL},
    {"__deepcopy__",
        (PyCFunction)array_deepcopy,
        METH_VARARGS, NULL},

    /* for Pickling */
    {"__reduce__",
        (PyCFunction) array_reduce,
        METH_VARARGS, NULL},
    {"__setstate__",
        (PyCFunction) array_setstate,
        METH_VARARGS, NULL},
    {"dumps",
        (PyCFunction) array_dumps,
        METH_VARARGS, NULL},
    {"dump",
        (PyCFunction) array_dump,
        METH_VARARGS, NULL},

    /* Original and Extended methods added 2005 */
    {"all",
        (PyCFunction)array_all,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"any",
        (PyCFunction)array_any,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"argmax",
        (PyCFunction)array_argmax,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"argmin",
        (PyCFunction)array_argmin,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"argsort",
        (PyCFunction)array_argsort,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"astype",
        (PyCFunction)array_cast,
        METH_VARARGS, NULL},
    {"byteswap",
        (PyCFunction)array_byteswap,
        METH_VARARGS, NULL},
    {"choose",
        (PyCFunction)array_choose,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"clip",
        (PyCFunction)array_clip,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"compress",
        (PyCFunction)array_compress,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"conj",
        (PyCFunction)array_conjugate,
        METH_VARARGS, NULL},
    {"conjugate",
        (PyCFunction)array_conjugate,
        METH_VARARGS, NULL},
    {"copy",
        (PyCFunction)array_copy,
        METH_VARARGS, NULL},
    {"cumprod",
        (PyCFunction)array_cumprod,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"cumsum",
        (PyCFunction)array_cumsum,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"diagonal",
        (PyCFunction)array_diagonal,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"dot",
        (PyCFunction)array_dot,
        METH_VARARGS, NULL},
    {"fill",
        (PyCFunction)array_fill,
        METH_VARARGS, NULL},
    {"flatten",
        (PyCFunction)array_flatten,
        METH_VARARGS, NULL},
    {"getfield",
        (PyCFunction)array_getfield,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"item",
        (PyCFunction)array_toscalar,
        METH_VARARGS, NULL},
    {"itemset",
        (PyCFunction) array_setscalar,
        METH_VARARGS, NULL},
    {"max",
        (PyCFunction)array_max,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"mean",
        (PyCFunction)array_mean,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"min",
        (PyCFunction)array_min,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"newbyteorder",
        (PyCFunction)array_newbyteorder,
        METH_VARARGS, NULL},
    {"nonzero",
        (PyCFunction)array_nonzero,
        METH_VARARGS, NULL},
    {"prod",
        (PyCFunction)array_prod,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"ptp",
        (PyCFunction)array_ptp,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"put",
        (PyCFunction)array_put,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"ravel",
        (PyCFunction)array_ravel,
        METH_VARARGS, NULL},
    {"repeat",
        (PyCFunction)array_repeat,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"reshape",
        (PyCFunction)array_reshape,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"resize",
        (PyCFunction)array_resize,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"round",
        (PyCFunction)array_round,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"searchsorted",
        (PyCFunction)array_searchsorted,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"setfield",
        (PyCFunction)array_setfield,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"setflags",
        (PyCFunction)array_setflags,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"sort",
        (PyCFunction)array_sort,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"squeeze",
        (PyCFunction)array_squeeze,
        METH_VARARGS, NULL},
    {"std",
        (PyCFunction)array_stddev,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"sum",
        (PyCFunction)array_sum,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"swapaxes",
        (PyCFunction)array_swapaxes,
        METH_VARARGS, NULL},
    {"take",
        (PyCFunction)array_take,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"tofile",
        (PyCFunction)array_tofile,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"tolist",
        (PyCFunction)array_tolist,
        METH_VARARGS, NULL},
    {"tostring",
        (PyCFunction)array_tostring,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"trace",
        (PyCFunction)array_trace,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"transpose",
        (PyCFunction)array_transpose,
        METH_VARARGS, NULL},
    {"var",
        (PyCFunction)array_variance,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"view",
        (PyCFunction)array_view,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

#undef _ARET
