from distutils.core import setup, Extension
from Cython.Build import cythonize

ext_modules = [
  Extension("dg_hash_wrapper",
    sources = ["dg_hash_wrapper.pyx"],
    extra_objects=['dg_hash.a']
  )
]

setup(
  name = 'wrapper for dg_hash',
  ext_modules = cythonize(ext_modules),
)
