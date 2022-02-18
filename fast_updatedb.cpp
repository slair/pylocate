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
#include <QtSql>

#include <sys/types.h>
#include <sys/stat.h>

#include "sqlitedriver.h"

PyObject*	obj_cfg;
QStringList	cfg_exclude_folders;
bool		cfg_debug;
bool		cfg_type_use_magic;
QString		cfg_fn_database;

SQLiteDriver* driver = new SQLiteDriver();
QSqlDatabase db = QSqlDatabase::addDatabase(driver);


int put_type_id_to_db(const QString* type)
{
	int res = -1;
	QSqlQuery q;
	q.prepare("INSERT INTO types(name) VALUES (:name)");
	q.bindValue(":name", *type);
	q.exec();
	db.commit();
	QString query = QString("SELECT id FROM types WHERE name='%1'")
					.arg(*type);
	q.exec(query);
	if (q.next()) {
		res = q.value(0).toInt();
		//~ qDebug() << "select after insert" << res;
	} else {
		qDebug() << "! FAILURE" << query;
	}
	return res;
}

int get_type_id_from_db(const QString* type)
{
	int res = -1;
	QSqlQuery q;

	QString query_string = QString("SELECT id FROM types WHERE "
								   "name='%1'").arg(*type);
	//~ qDebug() << query_string;
	q.exec(query_string);

	if (q.next()) {
		res = q.value(0).toInt();
		//~ qDebug() << "select" << res;
	} else {
		res = put_type_id_to_db(type);
	}
	return res;
}

int get_type_id(QFileInfoList::ConstIterator entry, const QString* type)
{
	int res = -1;
	if (type->isEmpty()) {
		if (cfg_type_use_magic) {
			// todo: use magic to determine file type
		} else {
			// todo: somehow determine file type
		}
	} else {
		// todo: search id of type
		res = get_type_id_from_db(type);
	}
	return res;
}


#ifdef MS_WINDOWS
bool get_BY_HANDLE_FILE_INFORMATION(const QString* item,
									BY_HANDLE_FILE_INFORMATION* info)
{
	const wchar_t *path = (const wchar_t *)item->utf16();
	HANDLE hFile;
	hFile = CreateFileW(
				path,
				FILE_READ_ATTRIBUTES,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,
				NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		qDebug() << "! FAILURE INVALID_HANDLE_VALUE";
		CloseHandle(hFile);
		return false;
	} else {
		if (!GetFileInformationByHandle(hFile, info)) {
			qDebug() << "! FAILURE GetFileInformationByHandle("
					 "hFile, &fileInfo)";
			CloseHandle(hFile);
			return false;
		}
	}
	CloseHandle(hFile);
	//~ res = fileInfo.dwVolumeSerialNumber;
	//~ qDebug() << folder_fullpath << "st_dev =" << st_dev;
	return true;
}
#endif /* MS_WINDOWS */


unsigned int get_st_dev(const QString* item)
{
	unsigned int res = 0;
#ifdef MS_WINDOWS
	BY_HANDLE_FILE_INFORMATION fileInfo;
	if (get_BY_HANDLE_FILE_INFORMATION(item, &fileInfo)) {
		res = fileInfo.dwVolumeSerialNumber;
	}
#endif /* MS_WINDOWS */
	return res;
}


int get_folder_id(const QString* folder)
{
	int res = -1;
	//~ struct _stat folder_stat;
	//~ QString folder_fullpath = folder;
	unsigned int st_dev = get_st_dev(folder);

	QSqlQuery q;
	QString query_string = QString("SELECT id FROM folders WHERE name='%1' "
								   "and dev=%2").arg(*folder).arg(st_dev);
	//~ qDebug() << query_string;
	q.exec(query_string);

	if (q.next()) {
		res = q.value(0).toInt();
		//~ qDebug() << "select" << res;
	} else {
		q.prepare("INSERT INTO folders(name, dev) VALUES (:name, :dev)");
		q.bindValue(":name", *folder);
		q.bindValue(":dev", st_dev);
		q.exec();
		db.commit();
		q.exec(QString("SELECT id FROM folders WHERE name='%1' and "
					   "dev=%2").arg(*folder).arg(st_dev));
		if (q.next()) {
			res = q.value(0).toInt();
			//~ qDebug() << "select after insert" << res;
		}
	}
	if (cfg_debug) {
		qDebug() << "int get_folder_id(" << *folder << ") = " << res;
	}
	return res;
}

#ifdef MS_WINDOWS
QString NTFSreadlink(QFileInfoList::ConstIterator entry)
{
	QString res = entry->fileName();
//~ def NTFSreadlink(path):
	//~ reparse_point_handle = CreateFileW(path,
	//~ 0,
	//~ 0,
	//~ None,
	//~ OPEN_EXISTING,
	//~ FILE_FLAG_OPEN_REPARSE_POINT |
	//~ FILE_FLAG_BACKUP_SEMANTICS,
	//~ None)
	//~ if reparse_point_handle == INVALID_HANDLE_VALUE:
	//~ raise WinError()
	//~ target_buffer = c_buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
	//~ n_bytes_returned = DWORD()
	//~ io_result = DeviceIoControl(reparse_point_handle,
	//~ FSCTL_GET_REPARSE_POINT,
	//~ None, 0,
	//~ target_buffer, len(target_buffer),
	//~ byref(n_bytes_returned),
	//~ None)
	//~ CloseHandle(reparse_point_handle)
	//~ if not io_result:
	//~ raise WinError()
	//~ rdb = REPARSE_DATA_BUFFER.from_buffer(target_buffer)
	//~ if rdb.ReparseTag == IO_REPARSE_TAG_SYMLINK:
	//~ return rdb.SymbolicLinkReparseBuffer.PrintName
	//~ elif rdb.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT:
	//~ return rdb.MountPointReparseBuffer.PrintName
	//~ raise ValueError("not a link")
	return res;
}

//~ #define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) (NARROW)(VALUE)
static __int64 secs_between_epochs = 11644473600; /* Seconds between
													1.1.1601 and 1.1.1970 */

static void
FILE_TIME_to_time_t_nsec(const FILETIME *in_ptr, time_t *time_out, int* nsec_out)
{
	/* XXX endianness. Shouldn't matter, as all Windows implementations
		are little-endian */
	/* Cannot simply cast and dereference in_ptr,
	   since it might not be aligned properly */
	__int64 in;
	memcpy(&in, in_ptr, sizeof(in));
	*nsec_out = (int)(in % 10000000) * 100; /* FILETIME is in units of
		100 nsec. */
	*time_out = Py_SAFE_DOWNCAST((in / 10000000) - secs_between_epochs,
								 __int64, time_t);
}
#endif /* MS_WINDOWS */

void add_or_update_fsitem(const int folder_id,
						  const int type_id,	// -1 сами придумаем via magic
						  const QString* target,
						  QFileInfoList::ConstIterator entry,
						  const QDateTime* dt_scan
						 )
{
	int my_type_id;
	if (type_id==-1) {
		my_type_id = get_type_id(entry, &QString(""));
	} else {
		my_type_id = type_id;
	}
	QSqlQuery q;
	QString query_string = QString("SELECT id, inode, nlink, dev, size, "
								   "type_id, target, description, atime, "
								   "mtime, ctime FROM fsitems WHERE "
								   "folder_id=%1 and name='%2'")
						   .arg(folder_id).arg(entry->fileName());
	//~ qDebug() << query_string;
	q.exec(query_string);
	if (q.next()) {
		// todo: check
		int id = q.value(0).toInt();
		// todo: update
	} else {
		// todo: insert new record

		q.prepare("INSERT INTO fsitems(folder_id, name, inode, nlink, dev, "
				  "stime, size, type_id, target, description, atime, mtime, "
				  "ctime) VALUES (:folder_id, :name, :inode, :nlink, :dev, "
				  ":stime, :size, :type_id, :target, :description, :atime, "
				  ":mtime, :ctime)");
		BY_HANDLE_FILE_INFORMATION fileInfo;
		if (!get_BY_HANDLE_FILE_INFORMATION(&(entry->filePath()), &fileInfo)) {
			qDebug() << "! FAILURE get_BY_HANDLE_FILE_INFORMATION(" <<
					 entry->filePath() << ", &fileInfo))";
			return;
		}
		q.bindValue(":folder_id", folder_id);
		q.bindValue(":name", entry->fileName());
		q.bindValue(":inode", (((uint64_t)fileInfo.nFileIndexHigh) << 32)
					+ fileInfo.nFileIndexLow);
		q.bindValue(":nlink", (uint64_t)fileInfo.nNumberOfLinks);
		q.bindValue(":dev", (uint64_t)fileInfo.dwVolumeSerialNumber);
		q.bindValue(":stime", *dt_scan);
		q.bindValue(":size", (((__int64)fileInfo.nFileSizeHigh)<<32)
					+ fileInfo.nFileSizeLow);
		q.bindValue(":type_id", my_type_id);
		q.bindValue(":target", *target);
		q.bindValue(":description", "");	// todo: get description

		time_t atime, mtime, ctime;
		int atime_nsec, mtime_nsec, ctime_nsec;
		FILE_TIME_to_time_t_nsec(&fileInfo.ftLastAccessTime,
								 &atime, &atime_nsec);
		FILE_TIME_to_time_t_nsec(&fileInfo.ftLastWriteTime,
								 &mtime, &mtime_nsec);
		FILE_TIME_to_time_t_nsec(&fileInfo.ftCreationTime,
								 &ctime, &ctime_nsec);
		QDateTime ts;
		ts.setTime_t(atime);
		qDebug() << atime << " -> " << ts;
		q.bindValue(":atime", ts);
		ts.setTime_t(mtime);
		q.bindValue(":mtime", ts);
		ts.setTime_t(ctime);
		q.bindValue(":ctime", ts);
		q.exec();
		//~ db.commit();
		//~ qDebug() << q.lastQuery();
		/*
		db.commit();
		q.exec(QString("SELECT id FROM folders WHERE name='%1' and dev=%2").arg(folder_fullpath).arg(st_dev));
		if (q.next()) {
			res = q.value(0).toInt();
			qDebug() << "select after insert" << res;
		}
		*/
	}

	//~ folder_id	= folder_id,
	//~ name		= item,
	//~ inode		= item_stat.st_ino,
	//~ nlink		= item_stat.st_nlink,
	//~ dev			= item_stat.st_dev,
	//~ stime		= dt_scan,
	//~ size		= item_stat.st_size,
	//~ type_id		= type_id,
	//~ target		= target,
	//~ description	= get_description(fp_item),
	//~ atime		= datetime.fromtimestamp(item_stat.st_atime),
	//~ mtime		= datetime.fromtimestamp(item_stat.st_mtime),
	//~ ctime		= datetime.fromtimestamp(item_stat.st_ctime)
	/*
	QDateTime dateTime = QDateTime::fromString(q.value(1).toString(),
						 Qt::ISODate);
	query.bindValue(":datetime", timestamp);
	// 2017-09-05T11:50:39
	query.bindValue(":datetime", timestamp.toString("yyyy-MM-dd hh:mm:ss"));
	// same as above but without the T
	*/
}

int traverse( const char* start_folder )
{
	QDateTime dt_scan = QDateTime::currentDateTime();

	if (cfg_debug) {
		qDebug() << "start_folder = " << start_folder;
	}

	QStringList paths;
	paths << start_folder;

	// get db for sqlite
	db.setDatabaseName(cfg_fn_database);
	if (!db.open()) {
		qDebug() << "! FAILURE db.open()" << db.lastError().text();
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
			qDebug() << "levelrec =" << levelrec <<
					 "\tcurrent_dir =" << current_dir;
		}
		if (levelrec>1) {
			qDebug() << "paths.size() =" << paths.size();
			break;
		}

		paths.removeFirst();

		QDir Dir_current = current_dir;
		//~ qDebug() << "Dir_current = " << Dir_current.absolutePath();
		Dir_current.setFilter( QDir::AllDirs|QDir::AllEntries|QDir::Hidden|
							   QDir::System|QDir::NoDotAndDotDot );
		QFileInfoList entries = Dir_current.entryInfoList();

		if (entries.isEmpty()) continue;

		int folder_id = get_folder_id(&current_dir);
		if (-1==folder_id) {
			qDebug() << "! ERROR int folder_id = get_folder_id(&current_dir);";
			break;
		}
		int type_id;

		QString it, filename, target, new_dir;
		for (QFileInfoList::ConstIterator entry=entries.begin();
			 entry!=entries.end(); entry++) {

			filename = entry->fileName();

			if (entry->isFile()) {
				it = "file";
				add_or_update_fsitem(folder_id, -1, &QString(""),
									 entry, &dt_scan);

			} else if (entry->isJunction()) {
				it = "";
				type_id = get_type_id(entry, &QString("junction link"));
				if (-1==type_id) {
					qDebug() << "! ERROR get_type_id(entry, "
							 "&QString(\"junction link\")) == -1";
					break;
				}
				target = NTFSreadlink(entry);
				qDebug() << "junctionlink" << filename <<
						 "->" << target;
				add_or_update_fsitem(folder_id, type_id, &target,
									 entry, &dt_scan);

			} else if (entry->isSymbolicLink()) {
				it = "";
				type_id = get_type_id(entry, &QString("symbolic link"));
				if (-1==type_id) {
					qDebug() << "! get_type_id(entry, "
							 "&QString(\"symbolic link\")) == -1";
					break;
				}
				target = entry->symLinkTarget();
				qDebug() << "symboliclink" << filename <<
						 "->" << target;
				add_or_update_fsitem(folder_id, type_id, &target,
									 entry, &dt_scan);

			} else if (entry->isDir()) {
				it = "";
				new_dir = QDir::toNativeSeparators(
							  QDir::cleanPath(
								  Dir_current.absolutePath() +
								  QDir::separator() +
								  filename
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

			if (it!="") qDebug().noquote() << it << filename;
		}
	}
	if (db.isOpen()) {
		db.commit();
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
	//~ QTextCodec *codec = QTextCodec::codecForName("UTF-8");
	//~ QTextCodec::setCodecForCStrings(codec);
	qInstallMessageHandler(myMessageOutput);

	const char* start_folder;

	if (!PyArg_ParseTuple(args, "sO", &start_folder, &obj_cfg)) {
		qDebug() << "FAILURE! "
				 "PyArg_ParseTuple(args, ""sO"", &start_folder, &obj_cfg)";
		return NULL;
	}

	int int_poit;	// PyObject_IsTrue
	PyObject* obj_pogas;	//PyObject_GetAttrString;

	// get cfg.debug flag
	obj_pogas = PyObject_GetAttrString(obj_cfg, "debug");
	int_poit = PyObject_IsTrue(obj_pogas);
	if (int_poit==0) {
		cfg_debug = false;
	} else if (int_poit==1) {
		cfg_debug = true;
	} else {
		qDebug() << "! FAILURE int_poit =" << int_poit;
	}
	if (cfg_debug) {
		qDebug() << "cfg_debug =" << cfg_debug;
	}

	// get cfg.type_use_magic
	obj_pogas = PyObject_GetAttrString(obj_cfg, "type_use_magic");
	int_poit = PyObject_IsTrue(obj_pogas);
	if (int_poit==0) {
		cfg_type_use_magic = false;
	} else if (int_poit==1) {
		cfg_type_use_magic = true;
	} else {
		qDebug() << "! FAILURE int_poit =" << int_poit;
	}
	if (cfg_debug) {
		qDebug() << "cfg_type_use_magic =" << cfg_type_use_magic;
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
		qDebug() << "traverse(" << start_folder << ") =" << r;
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
