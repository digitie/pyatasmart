#!/usr/bin/python2

import distutils
from distutils.core import setup
from distutils.extension import Extension
import glob
import os

smart_ext = Extension(name = '_atasmart',
sources = glob.glob(os.path.join('src', '*.c')),
                libraries = ['atasmart'])

setup(
    name = 'pyatasmart',
    version = '0.0.1',
    ext_modules = [smart_ext],
    license='GPLv2+',
    packages=['atasmart'],
    package_dir={'atasmart': 'atasmart'},
    scripts = glob.glob(os.path.join('scripts', '*'))
 )