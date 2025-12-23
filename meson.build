project(
  'lps',
  ['c'],
  default_options: ['warning_level=1', 'buildtype=release'],
  version: run_command('git', 'rev-parse', '--short', 'HEAD', check: false).stdout().strip()
)

# add_project_arguments('-D_LARGEFILE64_SOURCE', '-D_FILE_OFFSET_BITS=64', '-Werror=implicit-function-declaration', language : 'c')
compiler = meson.get_compiler('c')

# Enable various hardening flags.
c_args_hardened = [
  '-fno-strict-aliasing',
  '-fno-delete-null-pointer-checks',
  '-fno-strict-overflow',
  '-Werror=implicit',
  '-Werror=incompatible-pointer-types',
  '-Werror=int-conversion',
]

c_args_warnings = [
  '-Wno-unused-parameter',
  # '-Wno-gnu-zero-variadic-macro-arguments',
]

has_auto_var_init = compiler.has_argument('-ftrivial-auto-var-init=zero')
if has_auto_var_init
  c_args_hardened += '-ftrivial-auto-var-init=zero'
endif

argp = subproject('argp').get_variable('argp_dep')

cpu = target_machine.cpu_family()
if cpu == 'riscv64'
  add_project_arguments('-march=rv64gc_zba', language: 'c')
endif
add_project_link_arguments('-static', language: 'c')


defines = []
if compiler.has_function('getdents64', prefix : '''#define _GNU_SOURCE
#include <dirent.h>''')
  defines += '-DHAVE_GETDENTS64'
endif

subdir('core')
subdir('liblps')

subdir('lps-go')
subdir('lps-pipe')
subdir('lps-preempt')

subdir('utimer-test')