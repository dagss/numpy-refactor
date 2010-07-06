/*
 * DON'T INCLUDE THIS DIRECTLY.
 */

#ifndef NPY_NDARRAYOBJECT_H
#define NPY_NDARRAYOBJECT_H
#ifdef __cplusplus
#define CONFUSE_EMACS {
#define CONFUSE_EMACS2 }
extern "C" CONFUSE_EMACS
#undef CONFUSE_EMACS
#undef CONFUSE_EMACS2
/* ... otherwise a semi-smart identer (like emacs) tries to indent
       everything when you're typing */
#endif

#include "ndarraytypes.h"

/* Includes the "function" C-API -- these are all stored in a
   list of pointers --- one for each file
   The two lists are concatenated into one in multiarray.

   They are available as import_array()
*/

#include "__multiarray_api.h"


/* C-API that requries previous API to be defined */

#define PyArray_DescrCheck(op) (((PyObject*)(op))->ob_type==&PyArrayDescr_Type)

#define PyArray_Check(op) PyObject_TypeCheck(op, &PyArray_Type)
#define PyArray_CheckExact(op) (((PyObject*)(op))->ob_type == &PyArray_Type)

#define PyArray_HasArrayInterfaceType(op, type, context, out)                 \
        ((((out)=PyArray_FromStructInterface(op)) != Py_NotImplemented) ||    \
         (((out)=PyArray_FromInterface(op)) != Py_NotImplemented) ||          \
         (((out)=PyArray_FromArrayAttr(op, type, context)) !=                 \
          Py_NotImplemented))

#define PyArray_HasArrayInterface(op, out)                                    \
        PyArray_HasArrayInterfaceType(op, NULL, NULL, out)

#define PyArray_IsZeroDim(op) (PyArray_Check(op) && (PyArray_NDIM(op) == 0))

#define PyArray_IsScalar(obj, cls)                                            \
        (PyObject_TypeCheck(obj, &Py##cls##ArrType_Type))

#define PyArray_CheckScalar(m) (PyArray_IsScalar(m, Generic) ||               \
                                PyArray_IsZeroDim(m))

#define PyArray_IsPythonNumber(obj)                                           \
        (PyInt_Check(obj) || PyFloat_Check(obj) || PyComplex_Check(obj) ||    \
         PyLong_Check(obj) || PyBool_Check(obj))

#define PyArray_IsPythonScalar(obj)                                           \
        (PyArray_IsPythonNumber(obj) || PyString_Check(obj) ||                \
         PyUnicode_Check(obj))

#define PyArray_IsAnyScalar(obj)                                              \
        (PyArray_IsScalar(obj, Generic) || PyArray_IsPythonScalar(obj))

#define PyArray_CheckAnyScalar(obj) (PyArray_IsPythonScalar(obj) ||           \
                                     PyArray_CheckScalar(obj))

#define PyArray_IsIntegerScalar(obj) (PyInt_Check(obj)                        \
              || PyLong_Check(obj)                                            \
              || PyArray_IsScalar((obj), Integer))


#define PyArray_GETCONTIGUOUS(m) (PyArray_ISCONTIGUOUS(m) ?                   \
                                  Py_INCREF(m), (m) :                         \
                                  (PyArrayObject *)(PyArray_Copy(m)))

#define PyArray_SAMESHAPE(a1,a2) NpyArray_SAMESHAPE(PAA(a1), PAA(a2))

#define PyArray_SIZE(m) NpyArray_SIZE(PAA(m))
#define PyArray_NBYTES(m) NpyArray_NBYTES(PAA(m))
#define PyArray_FROM_O(m) PyArray_FromAny(m, NULL, 0, 0, 0, NULL)

#define PyArray_FROM_OF(m,flags) PyArray_CheckFromAny(m, NULL, 0, 0, flags,   \
                                                      NULL)

#define PyArray_FROM_OT(m,type) PyArray_FromAny(m,                            \
                                PyArray_DescrFromType(type), 0, 0, 0, NULL);

#define PyArray_FROM_OTF(m, type, flags)                                      \
        PyArray_FromAny(m, PyArray_DescrFromType(type), 0, 0,                 \
                        (((flags) & NPY_ENSURECOPY) ?                         \
                         ((flags) | NPY_DEFAULT) : (flags)), NULL)

#define PyArray_FROMANY(m, type, min, max, flags)                             \
        PyArray_FromAny(m, PyArray_DescrFromType(type), min, max,             \
                        (((flags) & NPY_ENSURECOPY) ?                         \
                         (flags) | NPY_DEFAULT : (flags)), NULL)

#define PyArray_ZEROS(m, dims, type, fortran)                                 \
        PyArray_Zeros(m, dims, PyArray_DescrFromType(type), fortran)

#define PyArray_EMPTY(m, dims, type, fortran)                                 \
        PyArray_Empty(m, dims, PyArray_DescrFromType(type), fortran)

#define PyArray_FILLWBYTE(obj, val) memset(PyArray_DATA(obj), val,            \
                                           PyArray_NBYTES(obj))

#define PyArray_REFCOUNT(obj) (((PyObject *)(obj))->ob_refcnt)
#define NPY_REFCOUNT PyArray_REFCOUNT
#define NPY_MAX_ELSIZE (2 * NPY_SIZEOF_LONGDOUBLE)

#define PyArray_ContiguousFromAny(op, type, min_depth, max_depth)             \
        PyArray_FromAny(op, PyArray_DescrFromType(type), min_depth,           \
                              max_depth, NPY_DEFAULT, NULL)

#define PyArray_EquivArrTypes(a1, a2)                                         \
        PyArray_EquivTypes(PyArray_DESCR(a1), PyArray_DESCR(a2))

#define PyArray_EquivByteorders(b1, b2)                                       \
        (((b1) == (b2)) || (PyArray_ISNBO(b1) == PyArray_ISNBO(b2)))

#define PyArray_SimpleNew(nd, dims, typenum)                                  \
        PyArray_New(&PyArray_Type, nd, dims, typenum, NULL, NULL, 0, 0, NULL)

#define PyArray_SimpleNewFromData(nd, dims, typenum, data)                    \
        PyArray_New(&PyArray_Type, nd, dims, typenum, NULL,                   \
                    data, 0, NPY_CARRAY, NULL)

#define PyArray_SimpleNewFromDescr(nd, dims, descr)                           \
        PyArray_NewFromDescr(&PyArray_Type, descr, nd, dims,                  \
                             NULL, NULL, 0, NULL)

#define PyArray_ToScalar(data, arr)                                           \
        PyArray_Scalar(data, PyArray_DESCR(arr), (PyObject *)arr)


/* These might be faster without the dereferencing of obj
   going on inside -- of course an optimizing compiler should
   inline the constants inside a for loop making it a moot point
*/

#define PyArray_GETPTR1(obj, i) ((void *)(PyArray_BYTES(obj) +                \
                                         (i)*PyArray_STRIDES(obj)[0]))

#define PyArray_GETPTR2(obj, i, j) ((void *)(PyArray_BYTES(obj) +             \
                                            (i)*PyArray_STRIDES(obj)[0] +     \
                                            (j)*PyArray_STRIDES(obj)[1]))

#define PyArray_GETPTR3(obj, i, j, k) ((void *)(PyArray_BYTES(obj) +          \
                                            (i)*PyArray_STRIDES(obj)[0] +     \
                                            (j)*PyArray_STRIDES(obj)[1] +     \
                                            (k)*PyArray_STRIDES(obj)[2]))

#define PyArray_GETPTR4(obj, i, j, k, l) ((void *)(PyArray_BYTES(obj) +       \
                                            (i)*PyArray_STRIDES(obj)[0] +     \
                                            (j)*PyArray_STRIDES(obj)[1] +     \
                                            (k)*PyArray_STRIDES(obj)[2] +     \
                                            (l)*PyArray_STRIDES(obj)[3]))

#define PyArray_XDECREF_ERR(obj) \
        if (obj && (PyArray_FLAGS(obj) & NPY_UPDATEIFCOPY)) {                 \
                PyArray_FLAGS(obj->base_arr) |= NPY_WRITEABLE;                \
                PyArray_FLAGS(obj) &= ~NPY_UPDATEIFCOPY;                      \
        }                                                                     \
        Py_XDECREF(obj)

#define PyArray_DESCR_REPLACE(descr) do {                                     \
                PyArray_Descr *_new_;                                         \
                _new_ = PyArray_DescrNew(descr);                              \
                Py_XDECREF(descr);                                            \
                descr = _new_;                                                \
        } while(0)

/* Copy should always return contiguous array */
#define PyArray_Copy(obj) PyArray_NewCopy(obj, NPY_CORDER)

#define PyArray_FromObject(op, type, min_depth, max_depth)                    \
        PyArray_FromAny(op, PyArray_DescrFromType(type), min_depth,           \
                              max_depth, NPY_BEHAVED | NPY_ENSUREARRAY, NULL)

#define PyArray_ContiguousFromObject(op, type, min_depth, max_depth)          \
        PyArray_FromAny(op, PyArray_DescrFromType(type), min_depth,           \
                              max_depth, NPY_DEFAULT | NPY_ENSUREARRAY, NULL)

#define PyArray_CopyFromObject(op, type, min_depth, max_depth)                \
        PyArray_FromAny(op, PyArray_DescrFromType(type), min_depth,           \
                        max_depth, NPY_ENSURECOPY | NPY_DEFAULT |             \
                        NPY_ENSUREARRAY, NULL)

#define PyArray_Cast(mp, type_num)                                            \
        PyArray_CastToType(mp, PyArray_DescrFromType(type_num), 0)

#define PyArray_Take(ap, items, axis)                                         \
        PyArray_TakeFrom(ap, items, axis, NULL, NPY_RAISE)

#define PyArray_Put(ap, items, values)                                        \
        PyArray_PutTo(ap, items, values, NPY_RAISE)

/* Compatibility with old Numeric stuff -- don't use in new code */

#define PyArray_FromDimsAndData(nd, d, type, data)                            \
        PyArray_FromDimsAndDataAndDescr(nd, d, PyArray_DescrFromType(type),   \
                                        data)

#include "old_defines.h"

/*
   Check to see if this key in the dictionary is the "title"
   entry of the tuple (i.e. a duplicate dictionary entry in the fields
   dict.
*/

#define NPY_TITLE_KEY(key, value) ((PyTuple_GET_SIZE((value))==3) && \
                                   (PyTuple_GET_ITEM((value), 2) == (key)))


/* Define python version independent deprecation macro */

#if PY_VERSION_HEX >= 0x02050000
#define DEPRECATE(msg) PyErr_WarnEx(PyExc_DeprecationWarning,msg,1)
#else
#define DEPRECATE(msg) PyErr_Warn(PyExc_DeprecationWarning,msg)
#endif


#ifdef __cplusplus
}
#endif


#endif /* NPY_NDARRAYOBJECT_H */
