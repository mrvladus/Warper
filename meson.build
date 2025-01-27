project(
  'warper',
  'c',
  version: '1.0',
)

install_subdir('data' / 'applications', install_dir: get_option('datadir'))
install_subdir('data' / 'icons', install_dir: get_option('datadir'))
install_subdir('data' / 'metainfo', install_dir: get_option('datadir'))

bin = executable(
  meson.project_name(),
  sources: files('src' / 'main.c'),
  dependencies: [dependency('libadwaita-1'), dependency('libportal')],
  install_dir: get_option('bindir'),
  install: true,
)

run_target('run', command: bin)
