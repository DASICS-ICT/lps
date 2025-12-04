srcs = files(
    'core.c',
    'arch_asm.c',
    'boxmap.c',
    'mmap.c',
    'runtime.S',
    'ctx.c',
    'box.c',
    'bound.c',
)

liblps_core = static_library(
    'lps-core',
    srcs,
    c_args: c_args_warnings,
    install: true,
    include_directories: include_directories('include'),
)

lps_core = declare_dependency(link_with: liblps_core, include_directories: include_directories('include'))

install_headers('include/lps_arch.h')
install_headers('include/lps_core.h')

subdir('test')
