#!/usr/bin/env python3
# -*- coding: utf-8 -*-

print("< Начинаем сборку >")

from distutils.core import setup, Extension

module_fastupdatedb = Extension(
	'_fudb',
	define_macros = [("MAJOR_VERSION", "1"), ("MINOR_VERSION", "0")],
	include_dirs = [
		"C:\\apps\\qt\\include",
		"C:\\apps\\qt\\include\\QtCore",
		"C:\\apps\\qt\\include\\QtSql"
	],
	libraries = ["Qt5Core", "Qt5Sql"],
	library_dirs = ["C:\\apps\\qt\\lib"],
	sources = ["fast_updatedb.cpp"])

setup (name = "FastUpdateDB",
	   version = "1.0",
	   description = "Update DB with C++ and Qt5",
	   ext_modules = [module_fastupdatedb])

