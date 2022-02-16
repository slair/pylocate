#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

from io866 import print866

print866("<        Начинаем сборку        >")

from distutils.core import setup, Extension

module_fastupdatedb = Extension(
	'_fudb',
	define_macros = [
		("MAJOR_VERSION", "1"),
		("MINOR_VERSION", "0"),
		("HAVE_QT5", "1"),
	],
	include_dirs = [
		"C:\\apps\\qt\\include",
		"C:\\apps\\qt\\include\\QtCore",
		"C:\\apps\\qt\\include\\QtSql",
		"3rdparty\\sqlite",
		"3rdparty\\sqlitex",
		"3rdparty\\qupzilla",
	],
	libraries = ["Qt5Core", "Qt5Sql"],
	library_dirs = ["C:\\apps\\qt\\lib"],
	sources = [
		"fast_updatedb.cpp",
		"3rdparty\\sqlite\\sqlite3.c",
		"3rdparty\\sqlitex\\sqlitedriver.cpp",
		"3rdparty\\sqlitex\\moc_sqlitedriver.cpp",
		"3rdparty\\sqlitex\\sqliteextension.cpp",
		"3rdparty\\sqlitex\\sqlcachedresult.cpp",
		"3rdparty\\qupzilla\\qzregexp.cpp",
	],
	extra_compile_args = ["/Yu", "/Yc"],
	extra_link_args = [],
	)

setup (name = "FastUpdateDB",
	   version = "1.0",
	   description = "Update DB with C++ and Qt5",
	   ext_modules = [module_fastupdatedb])
