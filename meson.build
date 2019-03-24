project('nuklear', 'c',
    license : 'MIT',
    meson_version : '>=0.45.0',
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

subdir('include')
subdir('src')

nuklear_dep = declare_dependency(
    link_with : libnuklear,
    include_directories : nuklear_incdir,
)
