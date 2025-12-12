executable(
  'lps-go',
  sources: ['main.c'],
  dependencies: [lps_linux],
  link_args: ['-T', meson.current_source_dir() / 'ld.ld'],
  install: true,
)