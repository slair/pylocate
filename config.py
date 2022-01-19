#!/usr/bin/env python3
# -*- coding: utf-8 -*-

myname = "pylocate"

try:
	_DEBUG
except NameError:
	_DEBUG = False

import sys, os, logging, tempfile

logger = logging.getLogger(__name__)
#~ logger.setLevel(logging.DEBUG)
logd=logger.debug
logi=logger.info
logw=logger.warning
loge=logger.error
logc=logger.critical

class CFG(object):
	exclude_folders = []
	_debug = _DEBUG
	fn_database = "locate.db"

	def __new__(cls):
		if not hasattr(cls, 'instance'):
			cls.instance = super(CFG, cls).__new__(cls)
		return cls.instance

	def __init__(self, *args, **keywords):
		super(CFG, self).__init__()	# *args, **keywords

		for key,value in keywords.items():
			setattr(self, key, value)

		#~ self.logd = self._logd

		if "HOME" in os.environ:
			env_HOME = os.environ["HOME"]
		elif "HOMEDRIVE" in os.environ and "HOMEPATH" in os.environ:
			env_HOME = os.environ["HOMEDRIVE"] + os.environ["HOMEPATH"]

		if "XDG_DATA_HOME" in os.environ:
			self._xdg_data_home = os.environ["XDG_DATA_HOME"]
		elif "APPDATA" in os.environ:
			self._xdg_data_home = os.environ["APPDATA"]
		elif env_HOME:
			self._xdg_data_home = os.path.join(
				env_HOME,
				os.path.join(".local", "share")
			)
		#~ logd("XDG_DATA_HOME=%r", self._xdg_data_home)

		if "XDG_CONFIG_HOME" in os.environ:
			self._xdg_config_home = os.environ["XDG_CONFIG_HOME"]
		elif "LOCALAPPDATA" in os.environ:
			self._xdg_config_home = os.environ["LOCALAPPDATA"]
		elif env_HOME:
			self._xdg_config_home = os.path.join(
				env_HOME,
				".config"
			)

		self.fn_data = os.path.join(self._xdg_data_home, myname)
		if not os.path.exists(self.fn_data):
			logd("Creating folder: %r", self.fn_data)
			os.makedirs(self.fn_data)

		self.fn_database = os.path.join(self.fn_data, myname+".sqlite")
		self.fn_config = os.path.join(self._xdg_config_home, myname+".json")


	def __str__(self):
		addr = "0x%016X"%id(self)
		res="%s at %s {"%(self.__class__, addr)
		for attr in dir(self):
			if attr!="instance" and attr[0]!="_" \
				and not callable(getattr(self, attr)):
				res += "\n\t%s = %s"%(attr, getattr(self, attr))
		res += "\n%s } at %s"%(self.__class__, addr)
		return res


	def _logd(self, *args, **kwargs):
		if not self._debug:
			return
		if len(args)>1:
			print(args[0]%args[1:], **kwargs)
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
	...

if __name__=='__main__':
	main()
