#!/usr/bin/env python3
# -*- coding: utf-8 -*-

_DEBUG = True
COMMIT_INTERVAL = 50

import os, sys, string, stat, subprocess, time, json, argparse, logging
import tempfile
from datetime import datetime

try:
	_DEBUG
except NameError:
	_DEBUG = False
	loglevel = logging.INFO

myname = os.path.splitext(os.path.basename(__file__))[0]

if _DEBUG:
	loglevel = logging.DEBUG

logging.basicConfig(level=loglevel, format='%(filename)s:%(lineno)d: ' + \
	'%(name)s: %(levelname)s: %(message)s')
logger = logging.getLogger(__name__)
logd=logger.debug
logi=logger.info
logw=logger.warning
loge=logger.error
logc=logger.critical

import sqlalchemy, magic

from config import cfg

from models import FSItemFolder, FSItemType, FSItem, InitDB


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


def fsitem_add_or_update(item):
	cmp_attrs = ("dtime", "size", "type_id", "description", "atime",
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
			if getattr(item, cmp_item)!=getattr(db_item, cmp_item):
				changed.append(cmp_item)

		if len(changed)>0:

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


def logd_status(itemcount, changes, elapsed):
	status = ("items = %r, speed = %.2f items/sec, " + \
		"changes = %r, speed = %.2f changes/sec")%(
		itemcount, itemcount / elapsed,
		changes, changes / elapsed
	)
	logd(status)


def get_folder_id(fp_item):
	try:
		db_item = session.query(FSItemFolder).filter_by(
			name=fp_item
		).one()
		#~ logd(db_item)
	except sqlalchemy.exc.NoResultFound:
		db_item = None

	if db_item is None:
		logd("get_folder_id:	%s %s", len(session.new), session.new)
		db_item = FSItemFolder(name=fp_item)
		session.add(db_item)
		session.commit()

	return db_item.id


def get_type_id(fp_item, s=None):

	if s is None:
		try:
			t = magic.from_file(fp_item)
		except FileNotFoundError:
			t = "cannot open %r"%fp_item

		if t.startswith("cannot open "):
			try:
				t = magic.from_buffer(open(fp_item, "rb").read(1024))
			except FileNotFoundError:
				t = "FileNotFoundError %r"%fp_item
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
		logd("get_type_id:	%s %s", len(session.new), session.new)
		db_item = FSItemType(name=t)
		session.add(db_item)
		session.commit()

	return db_item.id


def get_description(fp_item):
	...


def updatedb(startpath):
	global session

	logd("Updating database '%s'", cfg.fn_database)

	session = InitDB()

	paths = [startpath]

	dt_scan = datetime.now()	# scaning datetime

	itemcount = 0
	changes = 0

	t_start = time.perf_counter()

	while len(paths)>0:

		path = paths.pop(0)

		try:
			itemlist = os.listdir(path)
		except NotADirectoryError:
			# NotADirectoryError: [WinError 267]
			# Неверно задано имя папки: 'C:\\slair\\tmp\\bad-fff'
			# файловый симлинк на фолдер
			# todo: занести в базу кривой симлинк
			continue

		if path in cfg.exclude_folders:
			logw("Folder '%s' excluded by config", path)
			continue

		if len(itemlist)==0:
			continue

		folder_id = get_folder_id(path)

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
				stat.S_ISLNK(item_stat.st_mode):	# regular file or symlink

				if stat.S_ISLNK(item_stat.st_mode):
					target = os.readlink(fp_item)
					type_id = get_type_id(fp_item, "symbolic link")
				else:
					target = None
					type_id = get_type_id(fp_item)

				fsitem = FSItem(
					folder_id	= folder_id,
					name		= item,
					type_id		= type_id,
					inode		= item_stat.st_ino,
					nlink		= item_stat.st_nlink,
					dev			= item_stat.st_dev,
					size		= item_stat.st_size,
					target		= target,
					description	= get_description(fp_item),
					stime		= dt_scan,
					atime		= datetime.fromtimestamp(item_stat.st_atime),
					mtime		= datetime.fromtimestamp(item_stat.st_mtime),
					ctime		= datetime.fromtimestamp(item_stat.st_ctime)
				)

				changes += fsitem_add_or_update(fsitem)

				if changes!=0 and changes % COMMIT_INTERVAL == 0:
					logd("updatedb:	%s %s", len(session.new), session.new)
					session.commit()

				if itemcount!=0 and itemcount % COMMIT_INTERVAL == 0:
					elapsed = time.perf_counter() - t_start
					logd_status(itemcount, changes, elapsed)

			else:
				logd(item_stat)

	elapsed = time.perf_counter() - t_start
	logd_status(itemcount, changes, elapsed)
	session.commit()
	session.close()
	logd("Updating database '%s' finished in %.2f sec",
		cfg.fn_database, elapsed)


def parse_commandline(args):
	ap = argparse.ArgumentParser(description="Update pylocate database.")

	ap.add_argument("-d", "--debug", action="store_true", default=_DEBUG,
		dest="debug")

	ap.add_argument("-e", "--exclude-folder", action="append",
		dest="exclude_folder", help="folder to exclude from scaning"
		" %s"%cfg.exclude_folders)

	ap.add_argument("-r", "--recreate", action="store_true", default=False,
		dest="recreate", help="create new database %s"%cfg.fn_database)

	ap.add_argument("-v", "--show-config", action="store_true", default=False,
		dest="show_config", help="show config")

	ap.add_argument("-s", "--save-config", action="store_true", default=False,
		dest="save_config", help="save config changes to %s"%cfg.fn_config)

	ns = ap.parse_args(args[1:])

	#~ logd(ns)

	cfg.debug = ns.debug
	if cfg.debug:
		logger.setLevel(logging.DEBUG)

	if ns.exclude_folder is not None and len(ns.exclude_folder)>0:
		cfg.exclude_folders.extend(ns.exclude_folder)

	if ns.recreate:
		logd("Deleting database: '%s'", cfg.fn_database)
		os.unlink(cfg.fn_database)

	#~ logd(cfg)

	if ns.show_config:
		print(cfg)

	if ns.save_config:
		dconfig = dict(
			exclude_folders=cfg.exclude_folders,
		)
		save_obj(dconfig, cfg.fn_config)


def cleardb():
	global session
	logd("Cleaning database '%s'", cfg.fn_database)
	session = InitDB()
	dt_scan = datetime.now()	# cleaning datetime
	itemcount = 0
	changes = 0
	t_start = time.perf_counter()



	elapsed = time.perf_counter() - t_start
	logd_status(itemcount, changes, elapsed)
	session.commit()
	session.close()
	logd("Cleaning database '%s' finished in %.2f sec",
		cfg.fn_database, elapsed)

def main():

	if os.path.exists(cfg.fn_config):
		d = load_obj(cfg.fn_config)
		cfg.exclude_folders = d["exclude_folders"]

	parse_commandline(sys.argv)

	cleardb()
	updatedb("C:\\slair\\tmp")


if __name__=='__main__':
	main()
