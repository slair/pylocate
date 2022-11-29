#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
#~ import string
import stat
#~ import subprocess
import time
import json
import argparse
import logging
#~ import tempfile

from datetime import datetime

import sqlalchemy
import magic

from dbgtools import str_obj

from config import cfg

from models import FSItemFolder, FSItemType, FSItem, InitDB

#~ sys.path.append("build\\lib.win-amd64-3.7")
from _fudb import fast_updatedb

_DEBUG = True
COMMIT_INTERVAL = 50

win32 = sys.platform == "win32"
linux = sys.platform == "linux"
stop = sys.exit

try:
	_DEBUG
except NameError:
	_DEBUG = False
	loglevel = logging.INFO

myname = os.path.splitext(os.path.basename(__file__))[0]

if _DEBUG:
	loglevel = logging.DEBUG

logging.basicConfig(level=loglevel, format='%(filename)12s:%(lineno)03d: '
	+ '%(levelname)8s %(message)s')
	#~ '%(name)s: %(levelname)s: %(message)s')
logger = logging.getLogger(__name__)
logd = logger.debug
logi = logger.info
logw = logger.warning
loge = logger.error
logc = logger.critical


def save_obj(obj, fn):
	with open(fn, 'w') as handle:
		logd("Jsoning '%s'", fn)
		#~ json.dump(obj, handle, cls=APPEncoder, indent=2)
		json.dump(obj, handle, indent=2)


def load_obj(fn):
	if not os.path.exists(fn):
		return dict()

	with open(fn, 'r') as handle:
		logd("Unjsoning '%s'", fn)
		#~ return json.load(handle, object_hook=APP_decode)
		return json.load(handle)


def fsitem_add_or_update(session, item):
	cmp_attrs = ("size", "type_id", "description", "atime",
		"mtime", "ctime", "inode", "dev", "nlink")

	try:
		db_item = session.query(FSItem).filter_by(
			folder_id=item.folder_id,
			name=item.name
		).one()
	except sqlalchemy.exc.NoResultFound:
		db_item = None

	if db_item:

		changed = []

		for cmp_item in cmp_attrs:
			if getattr(item, cmp_item) != getattr(db_item, cmp_item):
				changed.append(cmp_item)

		if len(changed) > 0:

			for changed_item in changed:
				setattr(db_item, changed_item, getattr(item, changed_item))
			db_item.stime = item.stime

			#~ logd("update")
			#~ session.commit()

			return 1

		else:
			return 0

	else:
		session.add(item)
		return 1


def logd_status(itemcount, changes, elapsed, last_commit_len):
	status = ("(%s) items = %r, speed = %.2f items/sec, "
		+ "changes = %r, speed = %.2f changes/sec") % (
		last_commit_len, itemcount, itemcount / elapsed
		, changes, changes / elapsed
	)
	logd(status)
	#~ if changes>100:
		#~ stop()


def get_folder_id(session, fp_item):
	try:
		db_item = session.query(FSItemFolder).filter_by(
			name=fp_item
		).one()
		#~ logd(db_item)
	except sqlalchemy.exc.NoResultFound:
		db_item = None

	desc = get_description(fp_item)

	if db_item is None:
		if len(session.new) > 0:
			logd("get_folder_id:	%s %s", len(session.new), session.new)
		db_item = FSItemFolder(name=fp_item, description=desc)
		session.add(db_item)
		session.commit()
	else:
		if db_item.description != desc:
			db_item.description = desc

	return db_item.id


def get_type_id(session, fp_item, s=None):

	if s is None:
		t = "no magic"
		if cfg.type_use_magic:
			try:
				t = magic.from_file(fp_item)

			except magic.magic.MagicException:
				#~ loge(str_obj(e))
				t = "Error: magic exception"

			except FileNotFoundError:
				t = "cannot open %r" % fp_item

			except PermissionError:
				t = "Error: permission denied"

			if t.startswith("cannot open "):
				try:
					t = magic.from_buffer(open(fp_item, "rb").read(1024))
				except FileNotFoundError:
					#~ t = "FileNotFoundError %r"%fp_item
					t = "Error: file not found"
	else:
		t = s

	#~ logd(t)
	try:
		db_item = session.query(FSItemType).filter_by(
			name=t
		).one()
		#~ logd(db_item)
	except sqlalchemy.exc.NoResultFound:
		db_item = None

	if db_item is None:
		#~ if len(session.new)>0:
			#~ logd("get_type_id:	%s %s", len(session.new), session.new)
		db_item = FSItemType(name=t)
		session.add(db_item)
		session.commit()

	return db_item.id


def get_description(fp_item):
	folder = os.path.dirname(fp_item)
	name = os.path.basename(fp_item)
	fn_descfile = os.path.join(folder, "Descript.ion")

	if not os.path.exists(fn_descfile):
		return None

	with open(fn_descfile, "r", encoding="cp866") as f:
		for line in f:
			line = line.strip()
			#~ logd(line)
			if line[0] == "\"":
				bq = 1
				eq = line.find("\"", bq)
				#~ logd("bq=%r, eq=%r", bq, eq)
				fn = line[bq:eq]
				d = line[eq + 2:]
			else:
				#~ logd(line)
				eq = line.find(" ")
				fn = line[:eq]
				d = line[eq + 1:]
				#~ logd("fn=%s, d=%s", fn, d)

			if fn == name:
				#~ logd("Description for '%s' found '%s'", name, d)
				return d

	return None

# win32 code begin ============================================================

from ctypes import WinDLL, WinError # noqa
from ctypes.wintypes import DWORD, LPCWSTR, HANDLE, LPVOID, BOOL, LPDWORD \
	, USHORT, ULONG, WCHAR, c_buffer, c_ubyte, Structure, Union, addressof \
	, byref # noqa

kernel32 = WinDLL('kernel32')

GetFileAttributesW = kernel32.GetFileAttributesW
GetFileAttributesW.restype = DWORD
GetFileAttributesW.argtypes = (LPCWSTR,)		# lpFileName In

INVALID_FILE_ATTRIBUTES = 0xFFFFFFFF
FILE_ATTRIBUTE_REPARSE_POINT = 0x00400

CreateFileW = kernel32.CreateFileW
CreateFileW.restype = HANDLE
CreateFileW.argtypes = (LPCWSTR,		# lpFileName In
						DWORD,			# dwDesiredAccess In
						DWORD,			# dwShareMode In
						LPVOID,			# lpSecurityAttributes In_opt
						DWORD,			# dwCreationDisposition In
						DWORD,			# dwFlagsAndAttributes In
						HANDLE)			# hTemplateFile In_opt

OPEN_EXISTING = 3
FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000
FILE_FLAG_BACKUP_SEMANTICS = 0x02000000
INVALID_HANDLE_VALUE = HANDLE(-1).value
MAXIMUM_REPARSE_DATA_BUFFER_SIZE = 0x4000

DeviceIoControl = kernel32.DeviceIoControl
DeviceIoControl.restype = BOOL
DeviceIoControl.argtypes = (HANDLE,			# hDevice In
							DWORD,			# dwIoControlCode In
							LPVOID,			# lpInBuffer In_opt
							DWORD,			# nInBufferSize In
							LPVOID,			# lpOutBuffer Out_opt
							DWORD,			# nOutBufferSize In
							LPDWORD,		# lpBytesReturned Out_opt
							LPVOID)			# lpOverlapped Inout_opt

FSCTL_GET_REPARSE_POINT = 0x000900A8

UCHAR = c_ubyte


class GENERIC_REPARSE_BUFFER(Structure):
	_fields_ = (('DataBuffer', UCHAR * 1),)


class SYMBOLIC_LINK_REPARSE_BUFFER(Structure):
	_fields_ = (('SubstituteNameOffset', USHORT),
				('SubstituteNameLength', USHORT),
				('PrintNameOffset', USHORT),
				('PrintNameLength', USHORT),
				('Flags', ULONG),
				('PathBuffer', WCHAR * 1))

	@property
	def PrintName(self):
		arrayt = WCHAR * (self.PrintNameLength // 2)
		offset = type(self).PathBuffer.offset + self.PrintNameOffset
		return arrayt.from_address(addressof(self) + offset).value


class MOUNT_POINT_REPARSE_BUFFER(Structure):
	_fields_ = (('SubstituteNameOffset', USHORT),
				('SubstituteNameLength', USHORT),
				('PrintNameOffset', USHORT),
				('PrintNameLength', USHORT),
				('PathBuffer', WCHAR * 1))

	@property
	def PrintName(self):
		arrayt = WCHAR * (self.PrintNameLength // 2)
		offset = type(self).PathBuffer.offset + self.PrintNameOffset
		return arrayt.from_address(addressof(self) + offset).value


class REPARSE_DATA_BUFFER(Structure):
	class REPARSE_BUFFER(Union):
		_fields_ = (('SymbolicLinkReparseBuffer',
						SYMBOLIC_LINK_REPARSE_BUFFER),
					('MountPointReparseBuffer',
						MOUNT_POINT_REPARSE_BUFFER),
					('GenericReparseBuffer',
						GENERIC_REPARSE_BUFFER))
	_fields_ = (('ReparseTag', ULONG),
				('ReparseDataLength', USHORT),
				('Reserved', USHORT),
				('ReparseBuffer', REPARSE_BUFFER))
	_anonymous_ = ('ReparseBuffer',)


CloseHandle = kernel32.CloseHandle
CloseHandle.restype = BOOL
CloseHandle.argtypes = (HANDLE,)		# hObject In


def isNTFSlink(path):
	result = GetFileAttributesW(path)
	if result == INVALID_FILE_ATTRIBUTES:
		raise WinError()
	return bool(result & FILE_ATTRIBUTE_REPARSE_POINT)


IO_REPARSE_TAG_SYMLINK = 0xA000000C
IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003


def NTFSreadlink(path):
	reparse_point_handle = CreateFileW(
		path,
		0,
		0,
		None,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
		None)

	if reparse_point_handle == INVALID_HANDLE_VALUE:
		raise WinError()

	target_buffer = c_buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
	n_bytes_returned = DWORD()
	io_result = DeviceIoControl(
		reparse_point_handle,
		FSCTL_GET_REPARSE_POINT,
		None, 0,
		target_buffer, len(target_buffer),
		byref(n_bytes_returned),
		None)
	CloseHandle(reparse_point_handle)
	if not io_result:
		raise WinError()
	rdb = REPARSE_DATA_BUFFER.from_buffer(target_buffer)
	if rdb.ReparseTag == IO_REPARSE_TAG_SYMLINK:
		return rdb.SymbolicLinkReparseBuffer.PrintName
	elif rdb.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT:
		return rdb.MountPointReparseBuffer.PrintName
	raise ValueError("not a link")

# win32 code end ==============================================================


def updatedb(startpath):

	logd("Updating database '%s'", cfg.fn_database)

	session = InitDB()

	paths = [startpath]

	dt_scan = datetime.now()		# scaning datetime

	itemcount = 0
	changes = 0
	last_commit_len = 0

	t_start = time.perf_counter()

	while len(paths) > 0:

		path = paths.pop(0)

		#~ logd("os.path.ismount(path)=%s", os.path.ismount(path))
		#~ logd("os.path.islink(path)=%s", os.path.islink(path))

		target = None
		try:
			target = os.readlink(path)
		except ValueError:
			pass

		except OSError as e:
			if e.errno == 13:
				type_id = get_type_id(session, path, "Error: permission denied")
				folder_id = get_folder_id(session, os.path.dirname(path))

				item_stat = os.stat(path, follow_symlinks=False)

				fsitem = FSItem(
					folder_id=folder_id,
					name=os.path.basename(path),
					type_id=type_id,
					inode=item_stat.st_ino,
					nlink=item_stat.st_nlink,
					dev=item_stat.st_dev,
					size=item_stat.st_size,
					target=target,
					description=get_description(path),
					stime=dt_scan,
					atime=datetime.fromtimestamp(item_stat.st_atime),
					mtime=datetime.fromtimestamp(item_stat.st_mtime),
					ctime=datetime.fromtimestamp(item_stat.st_ctime)
				)

				#~ logd(fsitem)
				changes += fsitem_add_or_update(session, fsitem)
				continue

			elif e.errno not in (22,):
				loge(path)
				loge(str_obj(e))
				stop()

		if target is None:
			jp = isNTFSlink(path)
			if jp:
				target = NTFSreadlink(path)
				type_id = get_type_id(session, path, "junction link")
		else:
			loge("Link found: %s -> %s", path, target)
			stop(0)

		if target:

			folder_id = get_folder_id(session, os.path.dirname(path))

			item_stat = os.stat(path, follow_symlinks=False)

			fsitem = FSItem(
				folder_id=folder_id,
				name=os.path.basename(path),
				type_id=type_id,
				inode=item_stat.st_ino,
				nlink=item_stat.st_nlink,
				dev=item_stat.st_dev,
				size=item_stat.st_size,
				target=target,
				description=get_description(path),
				stime=dt_scan,
				atime=datetime.fromtimestamp(item_stat.st_atime),
				mtime=datetime.fromtimestamp(item_stat.st_mtime),
				ctime=datetime.fromtimestamp(item_stat.st_ctime)
			)

			#~ logd(fsitem)
			changes += fsitem_add_or_update(session, fsitem)
			continue

		try:
			itemlist = os.listdir(path)

		except PermissionError as e:
			if e.errno == 13:
				logw("Permission denied: '%s'", path)
				type_id = get_type_id(session, path
					, "Error: permission denied")
				folder_id = get_folder_id(session, os.path.dirname(path))

				item_stat = os.stat(path, follow_symlinks=False)

				fsitem = FSItem(
					folder_id=folder_id,
					name=os.path.basename(path),
					type_id=type_id,
					inode=item_stat.st_ino,
					nlink=item_stat.st_nlink,
					dev=item_stat.st_dev,
					size=item_stat.st_size,
					target=target,
					description=get_description(path),
					stime=dt_scan,
					atime=datetime.fromtimestamp(item_stat.st_atime),
					mtime=datetime.fromtimestamp(item_stat.st_mtime),
					ctime=datetime.fromtimestamp(item_stat.st_ctime)
				)

				#~ logd(fsitem)
				changes += fsitem_add_or_update(session, fsitem)
				continue
			else:
				loge(path)
				loge(str_obj(e))
				stop()

		except NotADirectoryError as e:
			loge(path)
			loge(str_obj(e))
			# NotADirectoryError: [WinError 267]
			# Неверно задано имя папки: 'C:\\slair\\tmp\\bad-fff'
			# файловый симлинк на фолдер
			# todo: занести в базу кривой симлинк
			continue

		if path in cfg.exclude_folders:
			logw("Folder '%s' excluded by config", path)
			continue

		if len(itemlist) == 0:
			continue

		folder_id = None		# get_folder_id(path)

		sep_count = path.count(os.sep)

		if sep_count <= cfg.showpath_level:
			if len(itemlist) > cfg.big_items_count_in_folder:
				logi("%7d %s ...", len(itemlist), path)

		for item in itemlist:

			fp_item = os.path.join(path, item)
			itemcount += 1

			try:
				item_stat = os.stat(fp_item, follow_symlinks=False)
			except FileNotFoundError:
				# FileNotFoundError: [WinError 2]
				# Не удается найти указанный файл: 'C:\\slair\\tmp\\bad-1.txt'
				# "битый" симлинк
				# todo: занести в базу кривой симлинк
				continue

			if stat.S_ISDIR(item_stat.st_mode):		# folder
				if item in cfg.exclude_folders:
					logw("Folder '%s' excluded by config", item)
					continue

				paths.append(fp_item)

			elif stat.S_ISREG(item_stat.st_mode) or \
				stat.S_ISLNK(item_stat.st_mode):  # regular file or symlink

				if stat.S_ISLNK(item_stat.st_mode):
					target = os.readlink(fp_item)
					type_id = get_type_id(session, fp_item, "symbolic link")
				else:
					target = None
					type_id = get_type_id(session, fp_item)

				if folder_id is None:
					folder_id = get_folder_id(session, path)

				fsitem = FSItem(
					folder_id=folder_id,
					name=item,
					type_id=type_id,
					inode=item_stat.st_ino,
					nlink=item_stat.st_nlink,
					dev=item_stat.st_dev,
					size=item_stat.st_size,
					target=target,
					description=get_description(fp_item),
					stime=dt_scan,
					atime=datetime.fromtimestamp(item_stat.st_atime),
					mtime=datetime.fromtimestamp(item_stat.st_mtime),
					ctime=datetime.fromtimestamp(item_stat.st_ctime)
				)

				changes += fsitem_add_or_update(session, fsitem)

				if changes != 0 and changes % COMMIT_INTERVAL == 0:
					if len(session.new) > 0:
						#~ logd("updatedb:	%s %s",
							#~ len(session.new), session.new)
						last_commit_len = len(session.new)
						session.commit()

				if itemcount != 0 and itemcount % COMMIT_INTERVAL == 0:
					elapsed = time.perf_counter() - t_start
					logd_status(itemcount, changes, elapsed, last_commit_len)

			else:
				logd(item_stat)
				stop()

	elapsed = time.perf_counter() - t_start
	last_commit_len = len(session.new)
	logd_status(itemcount, changes, elapsed, last_commit_len)
	session.commit()
	logd("Finished updating database '%s' in %.2f sec",
		cfg.fn_database, elapsed)

	#~ clear_empty_folders(time.perf_counter(), session)
	session.close()


def parse_commandline(args):
	ap = argparse.ArgumentParser(description="Update pylocate database.")

	ap.add_argument("-d", "--debug", action="store_true", default=_DEBUG,
		dest="debug")

	ap.add_argument("-e", "--exclude-folder", action="append",
		dest="exclude_folder", help="folder to exclude from scaning"
		" %s" % cfg.exclude_folders)

	ap.add_argument("-r", "--recreate", action="store_true", default=False,
		dest="recreate", help="create new database %s" % cfg.fn_database)

	ap.add_argument("-v", "--show-config", action="store_true", default=False,
		dest="show_config", help="show config")

	ap.add_argument("-m", "--type-use-magic", action="store_true",
		default=False, dest="type_use_magic",
		help="use magic python library to detect file type")

	ap.add_argument("-s", "--save-config", action="store_true", default=False,
		dest="save_config", help="save config changes to %s" % cfg.fn_config)

	ns = ap.parse_args(args[1:])

	#~ logd(ns)

	need_exit = False

	cfg.debug = ns.debug
	if cfg.debug:
		logger.setLevel(logging.DEBUG)

	cfg.type_use_magic = ns.type_use_magic

	if ns.exclude_folder is not None and len(ns.exclude_folder) > 0:
		cfg.exclude_folders.extend(ns.exclude_folder)
		#~ for f in ns.exclude_folder:
			#~ cfg.exclude_folders.append(envvar2path(f))

	if ns.recreate:
		logd("Deleting database: '%s'", cfg.fn_database)
		os.unlink(cfg.fn_database)
		print("Database '%s' deleted!" % cfg.fn_database)
		need_exit = True

	if ns.show_config:
		print(cfg)
		need_exit = True

	if ns.save_config:
		dconfig = dict(
			exclude_folders=cfg.exclude_folders,
		)
		save_obj(dconfig, cfg.fn_config)

	#~ cfg.exclude_folders = list(map(envvar2path, cfg.exclude_folders))
	#~ print(cfg)

	if need_exit:
		sys.exit(0)

	cfg.exclude_folders = list(map(envvar2path, cfg.exclude_folders))
	#~ print(cfg)


def logd_checked_deleted(checked, deleted, elapsed):
	logd("Checked %d, deleted %s items in %.2f sec" % (
		checked, deleted, elapsed))


def clear_nonexist_folders(t_start, session):
	session.begin()
	logd("Quering folders...")
	folders = session.query(FSItemFolder).all()
	deleted = 0
	checked = 0
	logd("Checking %d folders...", len(folders))
	for f in folders:
		if not os.path.exists(f.name):
			logd("Delete %s", f.name)
			session.delete(f)
			deleted += 1
		checked += 1

		if checked % cfg.check_report_every == 0:
			logd_checked_deleted(checked, deleted,
				time.perf_counter() - t_start)

	session.commit()
	logd_checked_deleted(checked, deleted, time.perf_counter() - t_start)
	return deleted


def clear_nonexist_files(t_start, session):
	session.begin()
	logd("Quering files...")
	files = session.query(FSItem).all()
	deleted = 0
	checked = 0
	logd("Checking %d files...", len(files))
	for f in files:
		folder = f.folder.name
		name = f.name
		fullpath = os.path.join(folder, name)
		if not os.path.exists(fullpath) and not os.path.islink(fullpath):
			logd("Delete %s", fullpath)
			session.delete(f)
			deleted += 1
		checked += 1

		if checked % cfg.check_report_every == 0:
			logd_checked_deleted(checked, deleted,
				time.perf_counter() - t_start)

	session.commit()
	logd_checked_deleted(checked, deleted, time.perf_counter() - t_start)
	return deleted


def clear_types(t_start, session):
	session.begin()
	logd("Cleaning types...")
	#~ dataset = session.query(FSItemType).all()
	checked = 0
	deleted = session.query(FSItemType).delete()
	#~ for item in dataset:
		#~ logd("%d %d Delete %s", len(dataset)-deleted, len(item.fsitems),
			#~ item.name)
		#~ session.delete(item)
		#~ deleted += 1
		#~ checked += 1
	session.commit()
	logd_checked_deleted(checked, deleted, time.perf_counter() - t_start)
	return deleted


def clear_empty_folders(t_start, session):
	session.begin()
	logd("Quering empty folders...")
	folders = session.query(FSItemFolder).all()
	deleted = 0
	checked = 0
	logd("Checking empty %d folders...", len(folders))
	for f in folders:
		if len(f.fsitems) == 0:
			logd("Delete %s", f.name)
			session.delete(f)
			deleted += 1
		checked += 1

		if checked % cfg.check_report_every == 0:
			logd_checked_deleted(checked, deleted,
				time.perf_counter() - t_start)

	session.commit()
	logd_checked_deleted(checked, deleted, time.perf_counter() - t_start)
	return deleted


def cleardb():
	logd("Cleaning database '%s'", cfg.fn_database)
	t_start = time.perf_counter()

	session = InitDB()

	clear_nonexist_folders(time.perf_counter(), session)

	deleted = clear_nonexist_files(time.perf_counter(), session)

	if deleted > 0:
		clear_empty_folders(time.perf_counter(), session)

	session.close()

	elapsed = time.perf_counter() - t_start
	logd("Finished cleaning database '%s' in %.2f sec",
		cfg.fn_database, elapsed)


def envvar2path(ev):
	if ev[0] == "%":
		# win32
		if ev[-1] == "%":
			varname = ev[1:-1]
		else:
			varname = ev[1:]
		if varname in os.environ:
			return os.environ[varname]

		else:
			return ev

	elif ev[0] == "$":
		# linux
		varname = ev[1:]
		if varname in os.environ:
			return os.environ[varname]

		else:
			return ev

	else:
		return ev


def main():

	if os.path.exists(cfg.fn_config):
		d = load_obj(cfg.fn_config)
		cfg.exclude_folders = d["exclude_folders"]

	parse_commandline(sys.argv)

	session = InitDB()
	#~ clear_types(time.perf_counter(), session)
	session.close()

	#~ cleardb()
	t_start = time.perf_counter()

	#~ updatedb("C:\\")
	#~ print(os.stat("c:", follow_symlinks=False))
	#~ print(os.stat("c:\\", follow_symlinks=False))
	#~ sys.stdout.flush()

	#~ cfg.debug = False
	fast_updatedb("C:\\", cfg)

	logd("elapsed %.9f", time.perf_counter() - t_start)


if __name__ == '__main__':
	main()
	#~ for item in ("%SystemRoot%", "$SystemRoot", "$HOME", "$HOME111", "%home%",
				#~ "%USERPROFILE%", "$USERPROFILE", "%USERPROFILE",
				#~ "$USERPROFILE", "$USER$PROFILE", "%%USERPROFILE"):
		#~ print("'%s' = '%s'"%(item, envvar2path(item)))
