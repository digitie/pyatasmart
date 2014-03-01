#!/usr/bin/python

import distutils
from distutils.core import setup
from distutils.extension import Extension


smart_ext = Extension(name = 'smart',
				sources = ['smart.c'],
                libraries = ['atasmart'])

setup(name = 'pyatasmart', version = '0.0.1', ext_modules = [smart_ext], scripts = ['smartdump'])
