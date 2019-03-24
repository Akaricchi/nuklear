
#
# For the meaning of these, see src/HEADER
#

cfg = configuration_data()
cfg.set('NK_INCLUDE_COMMAND_USERDATA', true)
cfg.set('NK_INCLUDE_COMMAND_USERDATA', true)
cfg.set('NK_INCLUDE_DEFAULT_ALLOCATOR', true)
cfg.set('NK_INCLUDE_FIXED_TYPES', true)
cfg.set('NK_INCLUDE_STANDARD_IO', true)
cfg.set('NK_INCLUDE_STANDARD_VARARGS', true)
cfg.set('NK_INCLUDE_VERTEX_BUFFER_OUTPUT', true)
cfg.set('NK_BUTTON_TRIGGER_ON_RELEASE', true)  # Personal preference; could be made an option
cfg.set('NK_SIN', 'sinf')
cfg.set('NK_COS', 'cosf')
cfg.set('NK_MEMSET', 'memset')
cfg.set('NK_MEMCPY', 'memcpy')
cfg.set('NK_SQRT', 'sqrt')
cfg.set('NK_STRTOD', 'strtod')
cfg.set('NK_VSNPRINTF', 'vsnprintf')

nuklear_config = configure_file(
    configuration : cfg,
    output : 'nuklear_config_auto.h',
)

nuklear_incdir = include_directories('.')
