# generate shader header using cmake
r = run_command('cmake', '-DSHADER_DIR=../src', '-DTARGET_DIR=.', '-P', '../src/generate.cmake')
if r.returncode() != 0
  error('failed to generate headers with cmake')
endif

# configure library header
conf_data = configuration_data()
conf_data.set('version', version)
configure_file(
  input: '../include/msdfgl.h.in',
  output: 'msdfgl.h',
  configuration: conf_data,
)
