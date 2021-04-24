inc = [
  '../include/',
]
src = [
  'msdfgl.c',
  'msdfgl_serializer.c',
  'msdfgl_map.c'

]
dep = [
  meson.get_compiler('c').find_library('m', required: true),

  dependency('glfw3'),
  dependency('opengl'),
  dependency('glew'),
  dependency('freetype2'),
]

msdfgl_lib = library('msdfgl', sources: src, include_directories: inc, dependencies: dep)
msdfgl_dep = declare_dependency(dependencies: dep, include_directories : include_directories('.'), link_with : msdfgl_lib)
