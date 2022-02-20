// -*- coding: utf-8 -*-

//~ // cppcheck-suppress missingInclude

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>

#ifdef MS_WINDOWS
#include <windows.h>
#endif

//~ #include <QCoreApplication>
#include <QDebug>
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
size_t		cfg_big_items_count_in_folder;
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
		qCritical() << query;
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
			res = get_type_id_from_db(&QString("no magic allowed"));
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
		//~ qWarning() << "INVALID_HANDLE_VALUE " << *item;
		CloseHandle(hFile);
		return false;
	} else {
		if (!GetFileInformationByHandle(hFile, info)) {
			qCritical() << "GetFileInformationByHandle("
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
	//~ qDebug() << "int get_folder_id(" << *folder << ") = " << res;
	return res;
}


#ifdef MS_WINDOWS
typedef struct _REPARSE_DATA_BUFFER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR  DataBuffer[1];
		} GenericReparseBuffer;
	} DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER;


QString NTFSreadlink(QFileInfoList::ConstIterator entry)
{
	QString path = QDir::toNativeSeparators(entry->filePath());
	//~ qDebug() << path;
	QString res;

	//~ DWORD result = (DWORD)E_FAIL;
	//~ TCHAR TargetPath[MAX_PATH];
	//~ LPTSTR TargetPath;
	//~ size_t TargetSize;

	HANDLE reparse_point_handle = CreateFileW((const wchar_t*) path.utf16(),
								  0, 0, NULL,
								  OPEN_EXISTING,
								  FILE_FLAG_OPEN_REPARSE_POINT
								  |FILE_FLAG_BACKUP_SEMANTICS, NULL);

	BOOL io_result = false;
	char target_buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = {0};
	REPARSE_DATA_BUFFER& reparseData = *(REPARSE_DATA_BUFFER*)target_buffer;
	DWORD n_bytes_returned;

	if (reparse_point_handle == INVALID_HANDLE_VALUE) {
		qWarning() << "INVALID_HANDLE_VALUE" << path;
		CloseHandle(reparse_point_handle);
	} else {
		io_result = DeviceIoControl(reparse_point_handle,
									FSCTL_GET_REPARSE_POINT,
									NULL, 0,
									&reparseData,
									sizeof(target_buffer),
									&n_bytes_returned,
									NULL);
		CloseHandle(reparse_point_handle);
	}
	if (!io_result) {
		qWarning() << "io_result =" << io_result;
	} else {
		if (reparseData.ReparseTag==IO_REPARSE_TAG_MOUNT_POINT) {
			res = QString::fromUtf16((const char16_t *)
									 (reparseData
									  .MountPointReparseBuffer
									  .PathBuffer + reparseData
									  .MountPointReparseBuffer
									  .PrintNameOffset/sizeof(WCHAR)),
									 reparseData
									 .MountPointReparseBuffer
									 .PrintNameLength/sizeof(WCHAR)
									);
		}
	}
	return res;
}

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

bool add_or_update_fsitem(const int folder_id,
						  const int type_id,	// -1 сами придумаем via magic
						  const QString* target,
						  QFileInfoList::ConstIterator entry,
						  const QDateTime* dt_scan
						 )
{
	int my_type_id;
	if (-1==type_id) {
		my_type_id = get_type_id(entry, &QString(""));
		if (-1==my_type_id) {
			qCritical("get_type_id(entry, &QString("")) = -1");
			return false;
		}
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
		// cppcheck-suppress unreadVariable
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
		if (!get_BY_HANDLE_FILE_INFORMATION(&entry->filePath(), &fileInfo)) {
			//~ qWarning() << "get_BY_HANDLE_FILE_INFORMATION(" <<
			//~ entry->filePath() << ", &fileInfo))";
			//~ return false;
			qWarning() << "unaccessible"
					   << QDir::toNativeSeparators(entry->filePath());
			my_type_id = get_type_id_from_db(&QString("unaccessible"));
			q.bindValue(":inode", 0);
			q.bindValue(":nlink", 0);
			q.bindValue(":dev", 0);
			q.bindValue(":size", 0);
			q.bindValue(":atime", 0);
			q.bindValue(":mtime", 0);
			q.bindValue(":ctime", 0);
		} else {
			q.bindValue(":inode", (((uint64_t)fileInfo.nFileIndexHigh) << 32)
						+ fileInfo.nFileIndexLow);
			q.bindValue(":nlink", (uint64_t)fileInfo.nNumberOfLinks);
			q.bindValue(":dev", (uint64_t)fileInfo.dwVolumeSerialNumber);
			q.bindValue(":size", (((__int64)fileInfo.nFileSizeHigh)<<32)
						+ fileInfo.nFileSizeLow);
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
			//~ qDebug() << atime << " -> " << ts;
			q.bindValue(":atime", ts);
			ts.setTime_t(mtime);
			q.bindValue(":mtime", ts);
			ts.setTime_t(ctime);
			q.bindValue(":ctime", ts);
		}
		q.bindValue(":stime", *dt_scan);
		q.bindValue(":folder_id", folder_id);
		q.bindValue(":name", entry->fileName());
		q.bindValue(":type_id", my_type_id);
		q.bindValue(":target", *target);
		q.bindValue(":description", "");	// todo: get description

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
	return true;
}

int traverse( const char* start_folder )
{
	int err = 0;

	QDateTime dt_scan = QDateTime::currentDateTime();

	qDebug() << "start_folder = " << start_folder;

	QStringList paths;
	paths << start_folder;

	// get db for sqlite
	db.setDatabaseName(cfg_fn_database);
	if (!db.open()) {
		qCritical() << "db.open()" << db.lastError().text();
		err = -1;
		return err;
	} else {
		qDebug() << "db opened";
	}

	while (!paths.isEmpty()) {
		int levelrec;

		QString current_dir = paths.constFirst();

		levelrec = current_dir.count(QDir::separator());
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

		if (cfg_big_items_count_in_folder < entries.size()) {
			qDebug() << "levelrec =" << levelrec
					 << "\t" << current_dir
					 << "\t" << entries.size();
		}

		int folder_id = get_folder_id(&current_dir);
		if (-1==folder_id) {
			qCritical() << "int folder_id = get_folder_id(&current_dir);";
			err = -2;
			break;
		}
		int type_id;

		QString it, filename, target, new_dir;
		for (QFileInfoList::ConstIterator entry=entries.begin();
			 entry!=entries.end(); entry++) {

			filename = entry->fileName();

			if (entry->isFile()) {
				it = "";
				if (!add_or_update_fsitem(folder_id, -1, &QString(""),
										  entry, &dt_scan)) {
					qCritical() << "add_or_update_fsitem(folder_id, "
								"-1, &QString(\"\"), entry, &dt_scan)";
					err = -3;
					break;
				}

			} else if (entry->isJunction()) {
				it = "";
				type_id = get_type_id(entry, &QString("junction link"));
				if (-1==type_id) {
					qDebug() << "! ERROR get_type_id(entry, "
							 "&QString(\"junction link\")) == -1";
					break;
				}
				target = NTFSreadlink(entry);
				//~ qDebug() << "junctionlink" << filename <<
				//~ "->" << target;
				if (!add_or_update_fsitem(folder_id, type_id, &target,
										  entry, &dt_scan)) {
					qCritical() << "add_or_update_fsitem(folder_id, "
								"type_id, &target, entry, &dt_scan)";
					err = -3;
					break;
				}

			} else if (entry->isSymbolicLink()) {
				it = "";
				type_id = get_type_id(entry, &QString("symbolic link"));
				if (-1==type_id) {
					qDebug() << "! get_type_id(entry, "
							 "&QString(\"symbolic link\")) == -1";
					err = -4;
					break;
				}
				target = QDir::toNativeSeparators(entry->symLinkTarget());
				//~ qDebug() << "symboliclink" << filename <<
				//~ "->" << target;
				if (!add_or_update_fsitem(folder_id, type_id, &target,
										  entry, &dt_scan)) {
					qCritical() << "add_or_update_fsitem(folder_id,"
								" type_id, &target, entry, &dt_scan)";
					err = -3;
					break;
				}

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
					qWarning() << "folder" <<
							   new_dir <<
							   "excluded by config";
				}
			} else {
				it = "unknown";
			}

			if (it!="") qDebug().noquote() << it << filename;
		}
		if (err!=0) {
			break;
		}
	}
	if (db.isOpen()) {
		db.commit();
		db.close();
		qDebug() << "db closed";
	}
	return err;
}

//~ int argc = 0;
//~ char* argv[1];
//~ QCoreApplication app(argc, argv);


void verboseMessageHandler(QtMsgType type, const QMessageLogContext &context,
						   const QString &msg)
{
	static const char* typeStr[] = {"   DEBUG", " WARNING",
									"CRITICAL", "   FATAL"
								   };

	if(type <= QtFatalMsg) {

#ifdef MS_WINDOWS
		// Установка кодека для нормальной работы консоли
		QTextCodec::setCodecForLocale(QTextCodec::codecForName("CP 866"));
#endif

		QByteArray localMsg = msg.toLocal8Bit();

		//~ QString contextString(QStringLiteral("%1:%2: %3")
		//~ .arg(context.file)
		//~ .arg(context.line)
		//~ .arg(context.function));
		QString contextString(QStringLiteral("%1:%2: ")
							  .arg(context.file)
							  .arg(context.line));

		QString timeStr(QDateTime::currentDateTime()
						.toString("dd-MM-yy HH:mm:ss:zzz"));

		//~ std::cerr << timeStr.toLocal8Bit().constData() << " - "
		if (type>0 || cfg_debug) {
			std::cout << contextString.toLocal8Bit().constData()
					  << typeStr[type] << " "
					  //~ << timeStr.toLocal8Bit().constData() << " "
					  << localMsg.constData() << std::endl;
			//~ std::cout.flush();
		}

#ifdef MS_WINDOWS
		// Установка кодека для нормальной работы локали
		QTextCodec::setCodecForLocale(QTextCodec::codecForName("CP 1251"));
#endif
		if(type == QtFatalMsg) {
			abort();
		}
	}
}
size_t get_size_t_attr_from_PyObject(PyObject* obj, const char* attr)
{
	size_t res;
	PyObject* obj_attr = PyObject_GetAttrString(obj, attr);
	res = PyLong_AsSize_t(obj_attr);
	return res;
}

bool get_bool_attr_from_PyObject(PyObject* obj, const char* attr)
{
	bool res;
	int int_poit;
	PyObject* obj_attr = PyObject_GetAttrString(obj, attr);
	int_poit = PyObject_IsTrue(obj_attr);
	if (int_poit==0) {
		res = false;
	} else if (int_poit==1) {
		res = true;
	} else {
		qCritical() << "int_poit =" << int_poit;
	}
	return res;
}

QString	get_QString_attr_from_PyObject(PyObject* obj, const char* attr)
{
	QString res;
	PyObject* obj_attr = PyObject_GetAttrString(obj, attr);
	res = PyUnicode_AsUTF8(obj_attr);
	return res;
}


QStringList get_QStringList_attr_from_PyObject(PyObject* obj,
		const char* attr)
{
	QStringList res;
	PyObject* obj_attr = PyObject_GetAttrString(obj, attr);
	Py_ssize_t ef_size = PyList_Size(obj_attr);
	for(Py_ssize_t i=0; i<ef_size; i++) {
		PyObject* ef_pystr = PyList_GetItem(obj_attr, i);
		const char *c_str = PyUnicode_AsUTF8(ef_pystr);
		QString out = QString(c_str);
		res << QDir::toNativeSeparators(QDir::cleanPath(out));
	}
	return res;
}


static PyObject* updatedb_impl(PyObject* self, PyObject* args)
{
	qInstallMessageHandler(verboseMessageHandler);

	const char* start_folder;

	if (!PyArg_ParseTuple(args, "sO", &start_folder, &obj_cfg)) {
		qCritical() << "!PyArg_ParseTuple(args, ""sO"", "
					"&start_folder, &obj_cfg)";
		return NULL;
	}

	// get debug flag from cfg
	cfg_debug = get_bool_attr_from_PyObject(obj_cfg, "debug");
	qDebug() << "cfg_debug =" << cfg_debug;

	// get type_use_magic from cfg
	cfg_type_use_magic = get_bool_attr_from_PyObject(obj_cfg, "type_use_magic");
	qDebug() << "cfg_type_use_magic =" << cfg_type_use_magic;

	// get fn_database from cfg
	cfg_fn_database = get_QString_attr_from_PyObject(obj_cfg, "fn_database");
	qDebug() << "cfg_fn_database =" << cfg_fn_database;

	// get exclude_folders from cfg
	cfg_exclude_folders = get_QStringList_attr_from_PyObject(obj_cfg,
						  "exclude_folders");
	qDebug() << "cfg_exclude_folders =" << cfg_exclude_folders;

	// get cfg_big_items_count_in_folder from cfg
	cfg_big_items_count_in_folder = get_size_t_attr_from_PyObject(obj_cfg,
									"big_items_count_in_folder");
	qDebug() << "cfg_big_items_count_in_folder ="
			 << cfg_big_items_count_in_folder;

	int r = traverse(start_folder);

	qDebug() << "traverse(" << start_folder << ") =" << r;

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

// cppcheck-suppress [unusedFunction]
PyMODINIT_FUNC PyInit__fudb(void)
{
	return PyModule_Create(&cppmodule);
}
