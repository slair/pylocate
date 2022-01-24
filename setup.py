#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from distutils.core import setup, Extension

module1 = Extension('cpp',
                    sources = ['fast_updatedb.cpp'])

setup (name = 'FastUpdateDB',
       version = '1.0',
       description = 'Update DB with C++',
       ext_modules = [module1])
