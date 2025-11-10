executable(
  'lps-run',
  sources: ['main.c'],
  dependencies: [liblfi_dep],
  link_args: ['-T', meson.current_source_dir() / 'ld.ld'],
  install: true,
)