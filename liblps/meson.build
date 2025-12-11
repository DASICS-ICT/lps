srcs = files(
    'linux.c',
    'proc.c',
    'fd.c',
    'elfload.c',
    'buf.c',
    'thread.c',
    'sys.c',
)

subdir('sys')

liblps_linux = static_library(
    'lps-linux',
    srcs,
    dependencies: [lps_core.as_link_whole()],
    install: true,
    include_directories: include_directories('include'),
)

lps_linux = declare_dependency(link_with: liblps_linux, include_directories: include_directories('include'))

install_headers('include/lps_linux.h')