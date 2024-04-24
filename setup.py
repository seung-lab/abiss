from setuptools import setup, find_packages
from setuptools.extension import Extension
# from setuptools.command.build_ext import build_ext as _build_ext
# from Cython.Build import cythonize
import os
# import builtins
import numpy

# VERSION = '1.0'

source_dir = os.path.dirname(os.path.abspath(__file__))
include_dirs = [
    os.path.join(source_dir, 'src'),
    # os.path.dirname(get_python_inc()),
    numpy.get_include(),
]

extensions = [
    Extension(
        'abiss',
        sources=[
            'python/abiss.pyx',
            'python/abiss_wrapper.cpp',
        ],
        include_dirs=include_dirs,
        language='c++',
        extra_link_args=['-lboost_iostreams', '-fopenmp', '-ltbb'],
        extra_compile_args=['-std=c++20', '-w'],
        # undef_macros=["NDEBUG"],
    )
]


setup(
    name='abiss',
    setup_requires=['numpy'],
    install_requires=['numpy', 'cython'],
    ext_modules=extensions,
)
