srcs = files(
    'buf.c',
    'elfload.c',
    'fd.c',
    'host.c',
    'linux.c',
    'proc.c',
    'sys.c',
    'thread.c',
    'cwalk.c',
    'kfile.c',
    'sched.c',
)

subdir('sys')

liblps_linux = static_library(
    'lps-linux',
    srcs,
    c_args: c_args_hardened,
    dependencies: [lps_core.as_link_whole()],
    install: true,
    include_directories: include_directories('include'),
)

lps_linux = declare_dependency(link_with: liblps_linux, include_directories: include_directories('include', '../core/include'))

install_headers('include/lps_linux.h')