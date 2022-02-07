// -*- coding: utf-8 -*-

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>

#ifdef MS_WINDOWS
#include <windows.h>
//~ #include <pathcch.h>
#endif

#include <QCoreApplication>
#include <QDebug>
void myMessageOutput(QtMsgType type,
					 const QMessageLogContext &context, const QString &msg)
{
	QTextStream cout(stdout, QIODevice::WriteOnly);
	cout << msg << Qt::endl;
}

#include <QDir>

#include <QSql>
#include <QSqlError>
#include <QSqlDatabase>

#include <sys/types.h>
#include <sys/stat.h>

#include "sqlitedriver.h"

PyObject*	obj_cfg;
QStringList	cfg_exclude_folders;
bool		cfg_debug;
QString		cfg_fn_database;


int get_folder_id(const QFileInfo* folder_Info)
{
	int res = -1;

	//~ struct _stat folder_stat;
	QString folder_fullpath = QDir::toNativeSeparators(
								  folder_Info->absoluteDir().absolutePath());
	int st_dev = 0;

	//~ _stat(folder_fullpath.toUtf8().constData(), &folder_stat);

#if defined(MS_WINDOWS)
	const wchar_t *path = (const wchar_t *)folder_fullpath.utf16();
	HANDLE hFile;
	BY_HANDLE_FILE_INFORMATION fileInfo;
	hFile = CreateFileW(
				path,
				FILE_READ_ATTRIBUTES,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,
				NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		qDebug() << "FAILURE! INVALID_HANDLE_VALUE";
	} else {
		if (!GetFileInformationByHandle(hFile, &fileInfo)) {
			qDebug() << "FAILURE! GetFileInformationByHandle(hFile, &fileInfo)";
		}
	}
	CloseHandle(hFile);
	st_dev = fileInfo.dwVolumeSerialNumber;
	//~ qDebug() << folder_fullpath << "st_dev =" <<
	//~ fileInfo.dwVolumeSerialNumber;
#endif
	return res;
}


int traverse( const char* start_folder )
{
	if (cfg_debug) {
		qDebug() << "! start_folder = " << start_folder;
	}

	QStringList paths;
	paths << start_folder;

	// get db for sqlite
	SQLiteDriver* driver = new SQLiteDriver();
	QSqlDatabase db = QSqlDatabase::addDatabase(driver);
	db.setDatabaseName(cfg_fn_database);
	if (!db.open()) {
		qDebug() << "FAILURE! db.open() "<< db.lastError().text();
		return NULL;
	} else {
		if (cfg_debug) {
			qDebug() << "db opened";
		}
	}


	while (!paths.isEmpty()) {
		int levelrec;

		QString current_dir = paths.constFirst();

		levelrec = current_dir.count(QDir::separator());
		if (cfg_debug) {
			qDebug() << "current_dir =" << current_dir;
			//~ qDebug() << "QDir::separator() =" << QDir::separator();
			qDebug() << "levelrec =" << levelrec;
		}
		if (levelrec>1) {
			qDebug() << "paths.size() =" << paths.size();
			break;
		}

		paths.removeFirst();

		QDir Dir_current = current_dir;
		Dir_current.setFilter( QDir::AllDirs|QDir::AllEntries|QDir::Hidden|
							   QDir::System|QDir::NoDotAndDotDot );
		QFileInfoList entries = Dir_current.entryInfoList();

		if (entries.isEmpty()) continue;

		QFileInfo Dir_current_Info = QFileInfo(Dir_current, QDir::separator());

		int folder_id = get_folder_id(&Dir_current_Info);

		QString it;
		for( QFileInfoList::ConstIterator entry=entries.begin();
			 entry!=entries.end(); entry++) {

			if (entry->isFile()) {
				it = "file";
			} else if (entry->isJunction()) {
				it = "junction";
			} else if (entry->isSymbolicLink()) {
				it = "symboliclink";
			} else if (entry->isDir()) {
				it = "";
				QString new_dir = QDir::toNativeSeparators(
									  QDir::cleanPath(
										  Dir_current.absolutePath() +
										  QDir::separator() +
										  entry->fileName()
									  ));

				if (!cfg_exclude_folders.contains(new_dir)) {
					paths << new_dir;
				} else {
					if (cfg_debug) {
						qDebug() << "Folder" <<
								 new_dir <<
								 "excluded by config";
					}
				}
			} else {
				it = "unknown";
			}

			if (it!="") {
				qDebug().noquote() << it <<
								   entry->fileName();
			}
		}
	}
	if (db.isOpen()) {
		db.close();
		if (cfg_debug) {
			qDebug() << "db closed";
		}
	}
	return 1;
}

//~ int argc = 0;
//~ char* argv[1];
//~ QCoreApplication app(argc, argv);

static PyObject* updatedb_impl(PyObject* self, PyObject* args)
{
	qInstallMessageHandler(myMessageOutput);

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

	int r = traverse(start_folder);

	if (cfg_debug) {
		qDebug() << "r =" << r;
	}

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
