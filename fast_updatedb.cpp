// -*- coding: utf-8 -*-

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>

#include <QDebug>
#include <QDir>


PyObject* cfgObj;
QStringList cfg_exclude_folders;


int traverse( const char* start_folder )
{
	std::cout << "start_folder = " << start_folder << std::endl;

	QStringList paths;
	paths << start_folder;

	while (!paths.isEmpty()) {

		QDir currentDir = paths.constFirst();
		paths.removeFirst();

		currentDir.setFilter( QDir::AllDirs|QDir::AllEntries|QDir::Hidden|
			QDir::System|QDir::NoDotAndDotDot );

		QFileInfoList entries = currentDir.entryInfoList();
		for( QFileInfoList::ConstIterator entry=entries.begin();
				entry!=entries.end(); entry++) {
			QString it;
			if (entry->isFile()) {
				it = "file ";
			} else if (entry->isJunction()) {
				it = "junction ";
			} else if (entry->isSymbolicLink()) {
				it = "symboliclink ";
			} else if (entry->isDir()) {
				it = "dir ";
				QString new_dir = QDir::toNativeSeparators(
									QDir::cleanPath(
										currentDir.absolutePath() +
										QDir::separator() +
										entry->fileName()
									));
				if (!cfg_exclude_folders.contains(new_dir)) {
					//~ paths << new_dir;
				} else {
					std::cout << "Folder '" <<
						new_dir.toStdString() << "' excluded by config" <<
						std::endl;
				}
			} else {
			}
			std::cout << it.toStdString() <<
				entry->fileName().toStdString() << std::endl;
		}
	}
    return 0;
}


static PyObject* updatedb_impl(PyObject* self, PyObject* args) {

	const char* start_folder;

	// std::cout << "updatedb_impl:" << std::endl;

	if (!PyArg_ParseTuple(args, "sO", &start_folder, &cfgObj))
        return NULL;

	PyObject* ef_pylistObj = PyObject_GetAttrString(cfgObj, "exclude_folders");
	Py_ssize_t ef_size = PyList_Size(ef_pylistObj);

	for(Py_ssize_t i=0; i<ef_size; i++) {
		PyObject* ef_pystr = PyList_GetItem(ef_pylistObj, i);
		const char *c_str = PyUnicode_AsUTF8(ef_pystr);
		QString out = QString(c_str);
		cfg_exclude_folders << QDir::toNativeSeparators(QDir::cleanPath(out));
	}
	qDebug() << "cfg_exclude_folders =" << cfg_exclude_folders;

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

PyMODINIT_FUNC PyInit_cpp(void) {	// cppcheck: disable=unusedFunction
    return PyModule_Create(&cppmodule);
}
