#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#~ import sys
import os
import logging
#~ import tempfile

try:
	_DEBUG
except NameError:
	_DEBUG = False

myname = "pylocate"

logger = logging.getLogger(myname)
logd = logger.debug
logi = logger.info
logw = logger.warning
loge = logger.error
logc = logger.critical
if _DEBUG:
	logger.setLevel(logging.DEBUG)
else:
	logger.setLevel(logging.INFO)


class CFG(object):
	exclude_folders = []
	_debug = _DEBUG
	fn_database = "locate.db"
	showpath_level = 3
	type_use_magic = False
	#~ delete_report_every = 10
	check_report_every = 1000
	big_items_count_in_folder = 100

	def __new__(cls):
		if not hasattr(cls, '__instance'):
			cls.__instance = super(CFG, cls).__new__(cls)
		return cls.__instance

	def __init__(self, *args, **keywords):
		super(CFG, self).__init__()		# *args, **keywords

		for key, value in keywords.items():
			setattr(self, key, value)

		#~ self.logd = self._logd

		if "HOME" in os.environ:
			env_HOME = os.environ["HOME"]
		elif "HOMEDRIVE" in os.environ and "HOMEPATH" in os.environ:
			env_HOME = os.environ["HOMEDRIVE"] + os.environ["HOMEPATH"]
		else:
			env_HOME = None

		if "XDG_DATA_HOME" in os.environ:
			self._xdg_data_home = os.environ["XDG_DATA_HOME"]
		elif "APPDATA" in os.environ:
			self._xdg_data_home = os.environ["APPDATA"]
		elif env_HOME:
			self._xdg_data_home = os.path.join(
				env_HOME,
				os.path.join(".local", "share")
			)
		logd("XDG_DATA_HOME=%r", self._xdg_data_home)

		if "XDG_CONFIG_HOME" in os.environ:
			self._xdg_config_home = os.environ["XDG_CONFIG_HOME"]
		elif "LOCALAPPDATA" in os.environ:
			self._xdg_config_home = os.environ["LOCALAPPDATA"]
		elif env_HOME:
			self._xdg_config_home = os.path.join(
				env_HOME,
				".config"
			)
		logd("XDG_CONFIG_HOME=%r", self._xdg_config_home)

		self.fn_data = os.path.join(self._xdg_data_home, myname)
		if not os.path.exists(self.fn_data):
			logd("Creating folder: %r", self.fn_data)
			os.makedirs(self.fn_data)

		self.fn_database = os.path.join(self.fn_data, myname + ".sqlite")
		self.fn_config = os.path.join(self._xdg_config_home, myname + ".json")

	def __str__(self):
		addr = "0x%016X" % id(self)
		res = "%s at %s {" % (self.__class__, addr)
		for attr in dir(self):
			if attr[0] != "_" \
				and not callable(getattr(self, attr)):
				res += "\n\t%s = %s" % (attr, getattr(self, attr))
		res += "\n%s } at %s" % (self.__class__, addr)
		return res

	def _logd(self, *args, **kwargs):
		if not self._debug:
			return

		if len(args) > 1:
			print(args[0] % args[1:], **kwargs)
		else:
			print(*args, **kwargs)

	def setup_logging(self, *args, **kwargs):
		if self._debug:
			logger.setLevel(logging.DEBUG)
			logd("setup logging")
		else:
			logd("shutdown logging")
			logger.setLevel(logging.INFO)

	@property
	def debug(self):
		return self._debug

	@debug.setter
	def debug(self, val):
		self._debug = val
		self.setup_logging()


# end CFG class ===
cfg = CFG()
#~ print(cfg)


def main():
	print(cfg)


if __name__ == '__main__':
	main()
