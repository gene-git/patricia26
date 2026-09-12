#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Copyright (c) 2026 Gene C <arch@sapience.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# patricia26
# Forked from https://github.com/jsommers/pytricia
#
# Creates 
# - libpatricia26.so
#   shared patricia library 
#
# - python module using the libraru
#   patricia26
#
# meson setup --reconfigure build/release --buildtype=release -Ddefault_library=shared \
#   -Dcpu_level=x86-64-v3 -Db_lto=true 
#
# meson compile -C build/release
# meson install -C build/release --destdir <xxx>
# uv build --wheel
#

project('patricia26', ['c', 'cython'],
  version: files('src/version.txt'),
  meson_version: '>=1.10.0',
  default_options: [
    'c_std=c23',
    'warning_level=3',
    'b_pie=true',                       # force PIE regardless of toolchain defaults
    'prefix=/usr',
    'mandir=share/man',
    ],
)

d_etc = get_option('sysconfdir')
d_lib = get_option('libdir')
d_bin = get_option('bindir')
d_man = get_option('mandir')
d_share = get_option('datadir')

#
# Compile / Link Options: Common
#
cpu_level = get_option('cpu_level')
if cpu_level == 'x86-64-v3'
  cpu_cflags = ['-march=x86-64-v3', '-mtune=generic']
else
  cpu_cflags = ['-march=x86-64', '-mtune=generic']
endif

optimized = false
if get_option('optimization') not in ['0', 'g']
  optimized = true
endif

cflags_common = [
  ## General
  '-D_GNU_SOURCE',
  '-pipe',                              # Speeds up compilation
  '-fno-plt',                           # Optimization for shared library calls
  '-Wmissing-prototypes',               # Warn for global function missing prototype decl
  ## Memory Protections
  '-fstack-protector-strong',           # Protect function calls with stack canaries
  '-fstack-clash-protection',           # Prevents stack-clash exploits
  '-ftrivial-auto-var-init=zero',       # Eliminate uninitialized stack memory leaks
  '-fzero-call-used-regs=used-gpr',     # Wipe general registers before returning
  ## Control Flow & Runtime Integrity
  '-mshstk',                            # Intel Shadow Stack protection
  '-fcf-protection',                    # Control-flow enforcement
  ## Diagnostic Hardening
  '-Wformat=2',                         # Warn about format/args mismatch printf, scanf etc
  '-Werror=format-security',            # Also warn about insecure printf format strings
]

cflags_common += cpu_cflags

ldflags_common = [
  '-Wl,--as-needed',                    # Limit shared to 'as needed'
  '-Wl,-z,relro',                       # Mark relocation tables read-only
  '-Wl,-z,now',                         # Force immediate binding (Full RELRO)
  '-Wl,-z,noexecstack',                 # Strictly enforce non-executable stack
  '-Wl,-z,pack-relative-relocs',        # Optimize layout with DT_RELR packing
]


#
# Compile / Link Options: libpatricia26
#
my_cflags = cflags_common + [
  '-fvisibility=hidden',                # Only explicitly exported symbols in shared lib symbol table
  '-Wshadow',                           # Warn if local variable/decl shadows another 
  '-Wconversion',                       # Warn about type conversions that may alter value
] 

if optimized
  my_cflags += ['-D_FORTIFY_SOURCE=3']
endif

if get_option('analyzer')
  my_cflags += ['-fanalyzer']
endif

my_ldflags = ldflags_common + [
  '-Wl,-z,defs',                        # Catch unresolved symbols at link time, not at runtime
]


#
# Compile / Link Options: Cython Patrici26
#
# cython generates code with lot reused variables, dodgy conversions etc.
# To keep the compile logs down we keep the aggresive build flags for the
# base code, but for c-code output by Cython ease up some.
#
py_cflags = cflags_common + [
  '-Wno-shadow',                        # cython has lots of variable names reused
  '-Wno-conversion',                    # its loosy with type conversions - leave on?
  '-Wno-pedantic',                      # its loosy with type conversions - leave on?
  '-Wno-unused-function',
]

if optimized
  py_cflags += ['-D_FORTIFY_SOURCE=3']
endif

py_ldflags = ldflags_common 

#
# Python init
#
py_mod = import('python')
py = py_mod.find_installation(pure: false)
cython_args = []
py_dep = py.dependency(embed: true)

inc_dir = []
lib_sources = []
subdir('src/patricia26')

patricia26_lib = library('patricia26',
  sources: lib_sources,
  include_directories: inc_dir,
  c_args: my_cflags,
  link_args: my_ldflags,
  dependencies: [],
  install: true,
  install_dir: d_lib,
  version: '1.0',
  soversion: '0',
  )

patricia26_dep = declare_dependency(
  link_with: patricia26_lib,
  include_directories: inc_dir,
  )

#
# Executables
#
smoke_test = executable('smoke_test',
  sources: ['src/smoke/smoke_test.c'],
  dependencies: [patricia26_dep],
  c_args: my_cflags,
  link_args: my_ldflags,
  install: false,
  install_dir: d_bin,
  )

test('smoke', smoke_test,
  args: [],
  )


#
# Install
#
s_share = 'src/share'
s_docs = 'src/docs'

install_data(
  [
    s_share / 'Readme.rst',
    s_share / 'LICENSE-LGPL.txt',
    s_share / 'LICENSE.rst',
    s_share / 'LICENSE-UMICH.txt',
    s_share / 'LICENSE-PYTRICIA.txt',
  ],
  install_dir: d_share / 'patricia26'
)

install_data(
  [
    s_share / 'LICENSE-LGPL.txt',
    s_share / 'LICENSE.rst',
    s_share / 'LICENSE-UMICH.txt',
  ],
  install_dir: d_share / 'licenses/patricia26'
)

install_data(
  [
    s_docs / 'patricia26.pdf',
  ],
  install_dir: d_share / 'patricia26'
)

subdir('src/python')

# Installs type stubs next to extension binary inside site-packages

# Update your installation tracking targets at the bottom of the file
py.install_sources(
  [
    'src/stubs/patricia26.pyi',
    'src/python/py.typed',
    'src/python/__init__.py',
  ],
  subdir: 'patricia26',
)

#
# python tests
#
pytest = find_program('/usr/bin/pytest', required: false)
if not pytest.found()
  warning('pytest not found, tests not available')
else
  proj_root = meson.project_source_root()
  build_root = meson.project_build_root()

  test_env = environment()
  test_env.prepend('PYTHONPATH', build_root / 'src/python')
  test_env.prepend('LD_LIBRARY_PATH', meson.project_build_root())

  testdir = proj_root / 'src/tests'

  test_files = [
    'test_core.py',
  ]

  foreach this_test : test_files
    test(this_test,
      pytest,
      args: ['-s', testdir / this_test],
      env: test_env,
      suite: ['unit'],
      timeout: 60,
      depends: [patricia26_lib]
    )
  endforeach
endif




