#
# patricia26
#

py.extension_module(
  '_patricia26',
  sources: [
    '_patricia26.pyx', # Local path, no prefix!
  ],
  c_args: py_cflags,
  cython_args: cython_args,
  dependencies: [py_dep, patricia26_dep],
  include_directories: inc_dir,
  link_args: py_ldflags,
  link_with: patricia26_lib,
  subdir: 'patricia26',
  install: true,
)

# 3. Compile Integer/Object Module within local scope
py.extension_module(
  '_patricia26_int',
  sources: [
    '_patricia26_int.pyx', # Local path, no prefix!
  ],
  c_args: py_cflags,
  #cython_args: cython_args + ['--module-name', '_patricia26'],
  cython_args: cython_args,
  dependencies: [py_dep, patricia26_dep],
  include_directories: inc_dir,
  link_args: py_ldflags,
  link_with: patricia26_lib,
  subdir: 'patricia26',
  install: true,
)

