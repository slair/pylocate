// -*- coding: utf-8 -*-

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>

#include <QSql>
#include <QSqlError>
#include <QSqlDatabase>

#include "sqlitedriver.h"

PyObject*	obj_cfg;
QStringList	cfg_exclude_folders;
bool		cfg_debug;
QString		cfg_fn_database;


//~ long get_folder_id(session, const QFileInfo* folder_Info)
//~ {
//~ struct stat stat1, stat2;
//~ QFileInfo fi1(path1), fi2(path2),
//~ stat(fi1.absoluteDir().absolutePath().toUtf8().constData(), &stat1);
//~ stat(fi2.absoluteDir().absolutePath().toUtf8().constData(), &stat2);
//~ return stat1.st_dev == stat2.st_dev;
//~ }


int traverse( const char* start_folder )
{
	std::cout << "start_folder = " << start_folder << std::endl;

	QStringList paths;
	paths << start_folder;

	// todo: get session for sqlite
	SQLiteDriver* driver = new SQLiteDriver();
	qDebug() << QSqlDatabase::drivers( );
	QSqlDatabase db = QSqlDatabase::addDatabase(driver);
	db.setDatabaseName(cfg_fn_database);
	if (!db.open()) {
		qDebug() << "FAILURE! db.open() "<< db.lastError().text();
		return NULL;
	}

	while (!paths.isEmpty()) {

		QDir currentDir = paths.constFirst();
		paths.removeFirst();

		currentDir.setFilter( QDir::AllDirs|QDir::AllEntries|QDir::Hidden|
							  QDir::System|QDir::NoDotAndDotDot );

		QFileInfoList entries = currentDir.entryInfoList();

		if (entries.isEmpty()) continue;

		QFileInfo currentDir_Info = QFileInfo(currentDir, QDir::separator());

		// todo: folder_id = get_folder_id(session, currentDir_Info)

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
					if (cfg_debug) {
						std::cout << "Folder '" <<
								  new_dir.toStdString() <<
								  "' excluded by config" <<
								  std::endl;
					}
				}
			} else {
			}

			std::cout << it.toStdString() <<
					  entry->fileName().toStdString() << std::endl;
		}
	}
	return 0;
}

int argc = 0;
char* argv[1];
QCoreApplication app(argc, argv);

static PyObject* updatedb_impl(PyObject* self, PyObject* args)
{
	const char* start_folder;

	if (!PyArg_ParseTuple(args, "sO", &start_folder, &obj_cfg)) {
		qDebug() << "FAILURE! "
				 "PyArg_ParseTuple(args, ""sO"", &start_folder, &obj_cfg)";
		return NULL;
	}

	// get cfg.debug flag
	PyObject* obj_cfg_debug = PyObject_GetAttrString(obj_cfg, "debug");
	int int_ocd = PyObject_IsTrue(obj_cfg_debug);
	if (int_ocd==0) {
		cfg_debug = false;
	} else if (int_ocd==1) {
		cfg_debug = true;
	} else {
		qDebug() << "FAILURE! int_ocd =" << int_ocd;
	}
	if (cfg_debug) {
		qDebug() << "cfg_debug =" << cfg_debug;
	}

	// get cfg.fn_database
	PyObject* obj_cfg_fn_database = PyObject_GetAttrString(obj_cfg,
									"fn_database");
	cfg_fn_database = PyUnicode_AsUTF8(obj_cfg_fn_database);
	if (cfg_debug) {
		qDebug() << "cfg_fn_database =" << cfg_fn_database;
	}

	// get cfg.exclude_folders
	PyObject* obj_cfg_exclude_folders = PyObject_GetAttrString(obj_cfg,
										"exclude_folders");
	Py_ssize_t ef_size = PyList_Size(obj_cfg_exclude_folders);
	for(Py_ssize_t i=0; i<ef_size; i++) {
		PyObject* ef_pystr = PyList_GetItem(obj_cfg_exclude_folders, i);
		const char *c_str = PyUnicode_AsUTF8(ef_pystr);
		QString out = QString(c_str);
		cfg_exclude_folders << QDir::toNativeSeparators(QDir::cleanPath(out));
	}
	if (cfg_debug) {
		qDebug() << "cfg_exclude_folders =" << cfg_exclude_folders;
	}

	traverse(start_folder);

	Py_RETURN_NONE;
}


static PyMethodDef CPPMethods[] = {
	{"fast_updatedb",  updatedb_impl, METH_VARARGS, "__doc__:Update DB"},
	{NULL, NULL, 0, NULL}	/* Страж */
};

static struct PyModuleDef cppmodule = {
	PyModuleDef_HEAD_INIT,
	"_fudb",	/* имя модуля */
	NULL,	/* документация модуля, может иметь значение NULL */
	-1,		/* размер состояния модуля для каждого интерпретатора,
				или -1, если
				модуль сохраняет состояние в глобальных переменных. */
	CPPMethods
};

PyMODINIT_FUNC PyInit__fudb(void)  	// cppcheck: disable=unusedFunction
{
	return PyModule_Create(&cppmodule);
}
