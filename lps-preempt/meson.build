executable(
  'lps-preempt',
  sources: ['main.c'],
  dependencies: [lps_linux, argp],
  link_args: ['-T', meson.current_source_dir() / 'ld.ld'],
  install: true,
)
