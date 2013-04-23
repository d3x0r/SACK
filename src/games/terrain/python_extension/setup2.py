from distutils.core import setup, Extension

setup(name='foo',
      version='1.0',
      ext_modules=[Extension('pyex', ['pyex1.dll'])],
      )

