srcs = files(
    'main.c',
    'uintr.c',
    'uintr_entry.S',
)


executable(
  'utimer-test',
  sources: srcs,
  link_args: ['-T', meson.current_source_dir() / 'ld.ld'],
  install: true,
)