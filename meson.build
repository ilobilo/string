project('string', 'cpp', license : 'GPL-3.0')

string_dep = declare_dependency(include_directories : include_directories('include'))

if meson.version().version_compare('>=0.54.0')
    meson.override_dependency('string', string_dep)
endif