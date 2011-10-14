
/* tell python that PyArg_ParseTuple(t#) means Py_ssize_t, not int */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#endif

int curve25519_donna(u_int8_t *mypublic, 
                     const u_int8_t *secret, const u_int8_t *basepoint);

static PyObject *
pycurve25519_makeprivate(PyObject *self, PyObject *args)
{
    u_int8_t *in1;
    Py_ssize_t in1len;
    if (!PyArg_ParseTuple(args, "t#:clamp", &in1, &in1len))
        return NULL;
    if (in1len != 32) {
        PyErr_SetString(PyExc_ValueError, "input must be 32-byte string");
        return NULL;
    }
    in1[0] &= 248;
    in1[31] &= 127;
    in1[31] |= 64;
    return PyString_FromStringAndSize((char *)in1, 32);
}

static PyObject *
pycurve25519_makepublic(PyObject *self, PyObject *args)
{
    const u_int8_t *private;
    u_int8_t mypublic[32];
    u_int8_t basepoint[32] = {9};
    Py_ssize_t privatelen;
    if (!PyArg_ParseTuple(args, "t#:makepublic", &private, &privatelen))
        return NULL;
    if (privatelen != 32) {
        PyErr_SetString(PyExc_ValueError, "input must be 32-byte string");
        return NULL;
    }
    curve25519_donna(mypublic, private, basepoint);
    return PyString_FromStringAndSize((char *)mypublic, 32);
}

static PyObject *
pycurve25519_makeshared(PyObject *self, PyObject *args)
{
    const u_int8_t *myprivate, *theirpublic;
    u_int8_t shared_key[32];
    Py_ssize_t myprivatelen, theirpubliclen;
    if (!PyArg_ParseTuple(args, "t#t#:generate",
                          &myprivate, &myprivatelen, &theirpublic, &theirpubliclen))
        return NULL;
    if (myprivatelen != 32) {
        PyErr_SetString(PyExc_ValueError, "input must be 32-byte string");
        return NULL;
    }
    if (theirpubliclen != 32) {
        PyErr_SetString(PyExc_ValueError, "input must be 32-byte string");
        return NULL;
    }
    curve25519_donna(shared_key, myprivate, theirpublic);
    return PyString_FromStringAndSize((char *)shared_key, 32);
}
    

static PyMethodDef curve25519_functions[] = {
    {"make_private", pycurve25519_makeprivate, METH_VARARGS, "data->private"},
    {"make_public", pycurve25519_makepublic, METH_VARARGS, "private->public"},
    {"make_shared", pycurve25519_makeshared, METH_VARARGS, "private+public->shared"},
    {NULL, NULL, 0, NULL},
};

PyMODINIT_FUNC
init_curve25519(void)
{
    (void)Py_InitModule("_curve25519", curve25519_functions);
}
