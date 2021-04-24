src = [
  'demo.c'
]
inc = [
  '../include'
]
msdfgldemo_exe = executable('msdfgldemo', sources: src, include_directories: inc, dependencies: [
  meson.get_compiler('c').find_library('m', required: true),

  dependency('glfw3'),
  dependency('opengl'),
  dependency('glew'),
  dependency('freetype2'),
  msdfgl_dep,
  ])
