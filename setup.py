from setuptools import setup, find_packages
from setuptools.extension import Extension
import os
import numpy

# VERSION = '1.0'

source_dir = os.path.dirname(os.path.abspath(__file__))
include_dirs = [
    os.path.join(source_dir, 'src'),
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
        # extra_link_args=['-lboost_iostreams', '-fopenmp', '-ltbb'],  # only necessary for when agg is included
        extra_compile_args=['-std=c++17', '-w'],
        # undef_macros=["NDEBUG"],
    )
]


setup(
    name='abiss',
    setup_requires=['numpy'],
    install_requires=['numpy', 'cython'],
    ext_modules=extensions,
)
