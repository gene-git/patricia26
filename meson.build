#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Copyright (c) 2026 Your Name <arch@sapience.com>
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

cpu_level = get_option('cpu_level')
if cpu_level == 'x86-64-v3'
  cpu_cflags = ['-march=x86-64-v3', '-mtune=generic']
else
  cpu_cflags = ['-march=x86-64', '-mtune=generic']
endif

if get_option('hardened_export')
  visibility_cflags = ['-fvisibility=hidden']
else
  visibility_cflags = ['-fvisibility=default']
endif

#
# cython generates code with lot reused variables, dodgy conversions etc.
# To keep the compile logs down we keep the aggresive build flags for the
# base code, but then when focus on python turn them off
#  -Wshadow
#  -Wconversion
#  -Wpedantic
#
my_cflags = [
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
  ## Aggressive Diagnostic Hardening
  '-Wshadow',                          # Warn if local variable/decl shadows another 
  '-Wconversion',                       # Warn about type conversions that may alter value
  '-Wformat=2',                         # Warn about format/args mismatch printf, scanf etc
  '-Werror=format-security',            # Also warn about insecure printf format strings
] + cpu_cflags + visibility_cflags

opt_level = get_option('optimization')
if opt_level not in ['0', 'g', 'default']
  my_cflags += ['-Wp,-D_FORTIFY_SOURCE=3']
endif

if get_option('analyzer')
  my_cflags += ['-fanalyzer']
endif

# link-time hardening
my_ldflags = [
  '-Wl,--as-needed',                    # Limit shared to 'as needed'
  '-Wl,-z,relro',                       # Mark relocation tables read-only
  '-Wl,-z,now',                         # Force immediate binding (Full RELRO)
  '-Wl,-z,noexecstack',                 # Strictly enforce non-executable stack
  '-Wl,-z,pack-relative-relocs',        # Optimize layout with DT_RELR packing
  '-Wl,-z,defs',                        # Catch unresolved symbols at link time, not at runtime
]
#add_project_arguments(my_cflags, language: 'c')
#add_project_link_arguments(my_ldflags, language: 'c')
#
#-
# Python module
# - compile / linker options
#
py_cflags = [
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
  # disable these for cython code
  '-Wno-shadow',                        # cython has lots of variable names reused
  '-Wno-conversion',                    # its loosy with type conversions - leave on?
  '-Wno-pedantic',                      # its loosy with type conversions - leave on?
  '-Wno-unused-function',
  ## Aggressive Diagnostic Hardening
  '-Wformat=2',                         # Warn about format/args mismatch printf, scanf etc
  '-Werror=format-security',            # Also warn about insecure printf format strings
] + cpu_cflags

opt_level = get_option('optimization')
if opt_level not in ['0', 'g', 'default']
  py_cflags += ['-Wp,-D_FORTIFY_SOURCE=3']
endif

py_ldflags = [
  '-Wl,--as-needed',                    # Limit shared to 'as needed'
  '-Wl,-z,relro',                       # Mark relocation tables read-only
  '-Wl,-z,now',                         # Force immediate binding (Full RELRO)
  '-Wl,-z,noexecstack',                 # Strictly enforce non-executable stack
  '-Wl,-z,pack-relative-relocs',        # Optimize layout with DT_RELR packing
]


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



#py.extension_module(
#  '_patricia26',
#  sources: [
#    'src/patricia26/patricia26.pyx',
#  ],
#  c_args: py_cflags,
#  cython_args: cython_args,
#  dependencies: [py_dep, patricia26_dep],
#  include_directories: inc_dir,
#  module_name: 'patricia26._patricia26',
#  link_args: py_ldflags,
#  link_with: patricia26_lib,
#  subdir: 'patricia26',
#  install: true,
#)

#py.extension_module(
#  '_patricia26_int',
#  sources: [
#    'src/patricia26/patricia26_int.pyx',
#  ],
#  c_args: py_cflags,
#  cython_args: cython_args,
#  dependencies: [py_dep, patricia26_dep],
#  include_directories: inc_dir,
#  module_name: 'patricia26._patricia26_int',
#  link_args: py_ldflags,
#  link_with: patricia26_lib,
##  subdir: 'patricia26',
#  install: true,
#)



#
# Install
#
s_share = 'src/share'
s_docs = 'Docs'

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
py.install_sources(
  [
    'src/stubs/_patricia26.pyi',
    'src/stubs/_patricia26_int.pyi',
    'src/python/py.typed',
    'src/python/__init__.py',
  ],
  subdir: 'patricia26',
)
