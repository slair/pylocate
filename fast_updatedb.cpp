// -*- coding: utf-8 -*-

#include <iostream>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

static PyObject* updatedb_impl(PyObject* self, PyObject* args) {
	const char* start_folder;
	int res;

	std::cout << "updatedb_impl:" << std::endl;

	if (!PyArg_ParseTuple(args, "s", &start_folder))
        return NULL;

	std::cout << "start_folder = " << start_folder << std::endl;

    res = 1234;	// system(command);
    // return PyLong_FromLong(res);
	Py_RETURN_NONE;
}


static PyMethodDef CPPMethods[] = {
    {"fast_updatedb",  updatedb_impl, METH_VARARGS, "Update DB"},
    {NULL, NULL, 0, NULL}        /* Страж */
};

static struct PyModuleDef cppmodule = {
    PyModuleDef_HEAD_INIT,
    "cpp",   /* имя модуля */
    NULL, /* документация модуля, может иметь значение NULL */
    -1,       /* размер состояния модуля для каждого интерпретатора, или -1, если
                 модуль сохраняет состояние в глобальных переменных. */
    CPPMethods
};

PyMODINIT_FUNC PyInit_cpp(void) {
    return PyModule_Create(&cppmodule);
}
