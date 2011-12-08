/* tell python that PyArg_ParseTuple(t#) means Py_ssize_t, not int */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#if (PY_VERSION_HEX < 0x02050000)
	typedef int Py_ssize_t;
#endif

/* This is required for compatibility with Python 2. */
#if PY_MAJOR_VERSION >= 3
	#include <bytesobject.h> 
	#define y "y"
#else
	#define PyBytes_FromStringAndSize PyString_FromStringAndSize
	#define y "t"
#endif

int curve25519_donna(char *mypublic, 
                     const char *secret, const char *basepoint);

static PyObject *
pycurve25519_makeprivate(PyObject *self, PyObject *args)
{
    char *in1;
    Py_ssize_t in1len;
    if (!PyArg_ParseTuple(args, y"#:clamp", &in1, &in1len))
        return NULL;
    if (in1len != 32) {
        PyErr_SetString(PyExc_ValueError, "input must be 32-byte string");
        return NULL;
    }
    in1[0] &= 248;
    in1[31] &= 127;
    in1[31] |= 64;
    return PyBytes_FromStringAndSize((char *)in1, 32);
}

static PyObject *
pycurve25519_makepublic(PyObject *self, PyObject *args)
{
    const char *private;
    char mypublic[32];
    char basepoint[32] = {9};
    Py_ssize_t privatelen;
    if (!PyArg_ParseTuple(args, y"#:makepublic", &private, &privatelen))
        return NULL;
    if (privatelen != 32) {
        PyErr_SetString(PyExc_ValueError, "input must be 32-byte string");
        return NULL;
    }
    curve25519_donna(mypublic, private, basepoint);
    return PyBytes_FromStringAndSize((char *)mypublic, 32);
}

static PyObject *
pycurve25519_makeshared(PyObject *self, PyObject *args)
{
    const char *myprivate, *theirpublic;
    char shared_key[32];
    Py_ssize_t myprivatelen, theirpubliclen;
    if (!PyArg_ParseTuple(args, y"#"y"#:generate",
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
    return PyBytes_FromStringAndSize((char *)shared_key, 32);
}


static PyMethodDef
curve25519_functions[] = {
    {"make_private", pycurve25519_makeprivate, METH_VARARGS, "data->private"},
    {"make_public", pycurve25519_makepublic, METH_VARARGS, "private->public"},
    {"make_shared", pycurve25519_makeshared, METH_VARARGS, "private+public->shared"},
    {NULL, NULL, 0, NULL},
};

#if PY_MAJOR_VERSION >= 3
    static struct PyModuleDef
    curve25519_module = {
        PyModuleDef_HEAD_INIT,
        "_curve25519",
        NULL,
        NULL,
        curve25519_functions,
    };

    PyObject *
    PyInit__curve25519(void)
    {
        return PyModule_Create(&curve25519_module);
    }
#else
    PyMODINIT_FUNC
    init_curve25519(void)
    {
          (void)Py_InitModule("_curve25519", curve25519_functions);
    }
#endif