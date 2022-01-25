#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from distutils.core import setup, Extension

module1 = Extension(
	'cpp',
	define_macros = [("MAJOR_VERSION", "1"), ("MINOR_VERSION", "0")],
	include_dirs = [
		"C:\\apps\\qt\\include",
		"C:\\apps\\qt\\include\\QtCore"
	],
	libraries = ["Qt5Core"],
	library_dirs = ["C:\\apps\\qt\\lib"],
	sources = ["fast_updatedb.cpp"])

setup (name = "FastUpdateDB",
	   version = "1.0",
	   description = "Update DB with C++ and Qt5",
	   ext_modules = [module1])
