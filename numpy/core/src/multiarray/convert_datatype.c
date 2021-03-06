#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#define _MULTIARRAYMODULE
#define NPY_NO_PREFIX
#include "numpy/arrayobject.h"
#include "numpy/arrayscalars.h"
#include "npy_api.h"

#include "npy_config.h"

#include "numpy/npy_3kcompat.h"

#include "common.h"
#include "scalartypes.h"
#include "mapping.h"
#include "ctors.h"
#include "arrayobject.h"


#include "convert_datatype.h"


NPY_NO_EXPORT NpyArray_Descr *
PyArray_DescrFromObjectUnwrap(PyObject *op, NpyArray_Descr *mintype);


/*NUMPY_API
 * For backward compatibility
 *
 * Cast an array using typecode structure.
 * steals reference to at --- cannot be NULL
 */
NPY_NO_EXPORT PyObject *
PyArray_CastToType(PyArrayObject *mp, PyArray_Descr *at, int fortran)
{
    NpyArray_Descr *atCore;    
    PyArray_Descr_REF_TO_CORE(at, atCore);
    
    RETURN_PYARRAY(NpyArray_CastToType(PyArray_ARRAY(mp), atCore, fortran));
}



/*NUMPY_API
 * Get a cast function to cast from the input descriptor to the
 * output type_number (must be a registered data-type).
 * Returns NULL if un-successful.
 */
NPY_NO_EXPORT PyArray_VectorUnaryFunc *
PyArray_GetCastFunc(PyArray_Descr *descr, int type_num)
{
    return (PyArray_VectorUnaryFunc *)NpyArray_GetCastFunc(descr->descr, type_num);
}


/*
 * Must be broadcastable.
 * This code is very similar to PyArray_CopyInto/PyArray_MoveInto
 * except casting is done --- PyArray_BUFSIZE is used
 * as the size of the casting buffer.
 */

/*NUMPY_API
 * Cast to an already created array.
 */
NPY_NO_EXPORT int
PyArray_CastTo(PyArrayObject *out, PyArrayObject *mp)
{
    return NpyArray_CastTo(PyArray_ARRAY(out), PyArray_ARRAY(mp));
}



/*NUMPY_API
 * Cast to an already created array.  Arrays don't have to be "broadcastable"
 * Only requirement is they have the same number of elements.
 */
NPY_NO_EXPORT int
PyArray_CastAnyTo(PyArrayObject *out, PyArrayObject *mp)
{
    return NpyArray_CastAnyTo(PyArray_ARRAY(out), PyArray_ARRAY(mp));
}



/*NUMPY_API
 *Check the type coercion rules.
 */
NPY_NO_EXPORT int
PyArray_CanCastSafely(int fromtype, int totype)
{
    return NpyArray_CanCastSafely(fromtype, totype);
}



/*NUMPY_API
 * leaves reference count alone --- cannot be NULL
 */
NPY_NO_EXPORT Bool
PyArray_CanCastTo(PyArray_Descr *from, PyArray_Descr *to)
{
    return NpyArray_CanCastTo(from->descr, to->descr);
}



/*NUMPY_API
 * See if array scalars can be cast.
 */
NPY_NO_EXPORT Bool
PyArray_CanCastScalar(PyTypeObject *from, PyTypeObject *to)
{
    int fromtype;
    int totype;
    
    /* TODO: Should _typenum_fromtypeobj be converted or is it available in core? */
    fromtype = _typenum_fromtypeobj((PyObject *)from, 0);
    totype = _typenum_fromtypeobj((PyObject *)to, 0);
    if (fromtype == NPY_NOTYPE || totype == NPY_NOTYPE) {
        return NPY_FALSE;
    }
    return (npy_bool) NpyArray_CanCastSafely(fromtype, totype);
}



/*NUMPY_API
 * Is the typenum valid?
 */
NPY_NO_EXPORT int
PyArray_ValidType(int type)
{
    return NpyArray_ValidType(type);
}



/* Backward compatibility only */
/* In both Zero and One

***You must free the memory once you are done with it
using PyDataMem_FREE(ptr) or you create a memory leak***

If arr is an Object array you are getting a
BORROWED reference to Zero or One.
Do not DECREF.
Please INCREF if you will be hanging on to it.

The memory for the ptr still must be freed in any case;
*/

static int
_check_object_rec(NpyArray_Descr *descr)
{
    if (NpyDataType_HASFIELDS(descr) && NpyDataType_REFCHK(descr)) {
        PyErr_SetString(PyExc_TypeError, "Not supported for this data-type.");
        return -1;
    }
    return 0;
}

/*NUMPY_API
  Get pointer to zero of correct type for array.
*/
NPY_NO_EXPORT char *
PyArray_Zero(PyArrayObject *arr)
{
    char *zeroval;
    int ret, storeflags;
    PyObject *obj;

    if (_check_object_rec(PyArray_DESCR(arr)) < 0) {
        return NULL;
    }
    zeroval = PyDataMem_NEW(PyArray_ITEMSIZE(arr));
    if (zeroval == NULL) {
        PyErr_SetNone(PyExc_MemoryError);
        return NULL;
    }

    obj=PyInt_FromLong((long) 0);
    if (PyArray_ISOBJECT(arr)) {
        memcpy(zeroval, &obj, sizeof(PyObject *));
        Py_DECREF(obj);
        return zeroval;
    }
    storeflags = PyArray_FLAGS(arr);
    PyArray_FLAGS(arr) |= BEHAVED;
    ret = PyArray_DESCR(arr)->f->setitem(obj, zeroval, PyArray_ARRAY(arr));
    PyArray_FLAGS(arr) = storeflags;
    Py_DECREF(obj);
    if (ret < 0) {
        PyDataMem_FREE(zeroval);
        return NULL;
    }
    return zeroval;
}

/*NUMPY_API
  Get pointer to one of correct type for array
*/
NPY_NO_EXPORT char *
PyArray_One(PyArrayObject *arr)
{
    char *oneval;
    int ret, storeflags;
    PyObject *obj;

    if (_check_object_rec(PyArray_DESCR(arr)) < 0) {
        return NULL;
    }
    oneval = PyDataMem_NEW(PyArray_ITEMSIZE(arr));
    if (oneval == NULL) {
        PyErr_SetNone(PyExc_MemoryError);
        return NULL;
    }

    obj = PyInt_FromLong((long) 1);
    if (PyArray_ISOBJECT(arr)) {
        memcpy(oneval, &obj, sizeof(PyObject *));
        Py_DECREF(obj);
        return oneval;
    }

    storeflags = PyArray_FLAGS(arr);
    PyArray_FLAGS(arr) |= BEHAVED;
    ret = PyArray_DESCR(arr)->f->setitem(obj, oneval, PyArray_ARRAY(arr));
    PyArray_FLAGS(arr) = storeflags;
    Py_DECREF(obj);
    if (ret < 0) {
        PyDataMem_FREE(oneval);
        return NULL;
    }
    return oneval;
}

/* End deprecated */

/*NUMPY_API
 * Return the typecode of the array a Python object would be converted to
 */
NPY_NO_EXPORT int
PyArray_ObjectType(PyObject *op, int minimum_type)
{
    NpyArray_Descr *intype;
    NpyArray_Descr *outtype;
    int ret;

    intype = NpyArray_DescrFromType(minimum_type);
    if (intype == NULL) {
        PyErr_Clear();
    }
    outtype = _array_find_type(op, intype, MAX_DIMS);
    ret = outtype->type_num;
    Npy_DECREF(outtype);
    Npy_XDECREF(intype);
    return ret;
}

/* Raises error when len(op) == 0 */

/*NUMPY_API*/
NPY_NO_EXPORT PyArrayObject **
PyArray_ConvertToCommonType(PyObject *op, int *retn)
{
    int i, n, allscalars = 0;
    PyArrayObject **mps = NULL;
    PyObject *otmp;
    NpyArray_Descr *intype = NULL, *stype = NULL;
    NpyArray_Descr *newtype = NULL;
    NPY_SCALARKIND scalarkind = NPY_NOSCALAR, intypekind = NPY_NOSCALAR;

    *retn = n = PySequence_Length(op);
    if (n == 0) {
        PyErr_SetString(PyExc_ValueError, "0-length sequence.");
    }
    if (PyErr_Occurred()) {
        *retn = 0;
        return NULL;
    }
    mps = (PyArrayObject **)PyDataMem_NEW(n*sizeof(PyArrayObject *));
    if (mps == NULL) {
        *retn = 0;
        return (void*)PyErr_NoMemory();
    }

    if (PyArray_Check(op)) {
        for (i = 0; i < n; i++) {
            mps[i] = (PyArrayObject *) array_big_item((PyArrayObject *)op, i);
        }
        if (!PyArray_ISCARRAY(op)) {
            for (i = 0; i < n; i++) {
                PyObject *obj;
                obj = PyArray_NewCopy(mps[i], NPY_CORDER);
                Py_DECREF(mps[i]);
                mps[i] = (PyArrayObject *)obj;
            }
        }
        return mps;
    }

    for (i = 0; i < n; i++) {
        otmp = PySequence_GetItem(op, i);
        if (!PyArray_CheckAnyScalar(otmp)) {
            newtype = PyArray_DescrFromObjectUnwrap(otmp, intype);
            Npy_XDECREF(intype);
            intype = newtype;
            mps[i] = NULL;
            intypekind = PyArray_ScalarKind(intype->type_num, NULL);
        }
        else {
            newtype = PyArray_DescrFromObjectUnwrap(otmp, stype);
            Npy_XDECREF(stype);
            stype = newtype;
            scalarkind = PyArray_ScalarKind(newtype->type_num, NULL);
            mps[i] = (PyArrayObject *)Py_None;
            Py_INCREF(Py_None);
        }
        Py_XDECREF(otmp);
    }
    if (intype==NULL) {
        /* all scalars */
        allscalars = 1;
        intype = stype;
        Npy_INCREF(intype);
        for (i = 0; i < n; i++) {
            Py_XDECREF(mps[i]);
            mps[i] = NULL;
        }
    }
    else if ((stype != NULL) && (intypekind != scalarkind)) {
        /*
         * we need to upconvert to type that
         * handles both intype and stype
         * also don't forcecast the scalars.
         */
        if (!PyArray_CanCoerceScalar(stype->type_num,
                                     intype->type_num,
                                     scalarkind)) {
            newtype = NpyArray_SmallType(intype, stype);
            Npy_XDECREF(intype);
            intype = newtype;
        }
        for (i = 0; i < n; i++) {
            Py_XDECREF(mps[i]);
            mps[i] = NULL;
        }
    }


    /* Make sure all arrays are actual array objects. */
    for (i = 0; i < n; i++) {
        int flags = CARRAY;

        if ((otmp = PySequence_GetItem(op, i)) == NULL) {
            goto fail;
        }
        if (!allscalars && ((PyObject *)(mps[i]) == Py_None)) {
            /* forcecast scalars */
            flags |= FORCECAST;
            Py_DECREF(Py_None);
        }
        Npy_INCREF(intype);
        mps[i] = (PyArrayObject*)
            PyArray_FromAnyUnwrap(otmp, intype, 0, 0, flags, NULL);
        Py_DECREF(otmp);
        if (mps[i] == NULL) {
            goto fail;
        }
    }
    Npy_DECREF(intype);
    Npy_XDECREF(stype);
    return mps;

 fail:
    Npy_XDECREF(intype);
    Npy_XDECREF(stype);
    *retn = 0;
    for (i = 0; i < n; i++) {
        Py_XDECREF(mps[i]);
    }
    PyDataMem_FREE(mps);
    return NULL;
}

