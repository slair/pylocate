// -*- coding: utf-8 -*-

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>

#include <QDebug>
#include <QDir>

int traverse( const char* start_folder )
{
	std::cout << "start_folder = " << start_folder << std::endl;

    QDir currentDir = start_folder;
	std::string se;

    currentDir.setFilter( QDir::AllDirs|QDir::AllEntries|QDir::Hidden|
		QDir::System|QDir::NoDotAndDotDot );

    QStringList entries = currentDir.entryList();
    for( QStringList::ConstIterator entry=entries.begin();
			entry!=entries.end();
			++entry) {

		se = entry->toStdString();
		std::cout << entry->toStdString() << std::endl;
		// std::cout << qPrintable(*entry) << std::endl;

		// qDebug().nospace() << qPrintable(*entry);
    }
    return 0;
}


static PyObject* updatedb_impl(PyObject* self, PyObject* args) {
	const char* start_folder;

	std::cout << "updatedb_impl:" << std::endl;

	if (!PyArg_ParseTuple(args, "s", &start_folder))
        return NULL;

	traverse(start_folder);

	Py_RETURN_NONE;
}


static PyMethodDef CPPMethods[] = {
    {"fast_updatedb",  updatedb_impl, METH_VARARGS, "Update DB"},
    {NULL, NULL, 0, NULL}	/* Страж */
};

static struct PyModuleDef cppmodule = {
    PyModuleDef_HEAD_INIT,
    "cpp",	/* имя модуля */
    NULL,	/* документация модуля, может иметь значение NULL */
    -1,		/* размер состояния модуля для каждого интерпретатора,
				или -1, если
				модуль сохраняет состояние в глобальных переменных. */
    CPPMethods
};

PyMODINIT_FUNC PyInit_cpp(void) {
    return PyModule_Create(&cppmodule);
}
