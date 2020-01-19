add_library(glad STATIC include/glad/glad.h include/KHR/khrplatform.h src/glad.c)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

target_include_directories(glad PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(glad PUBLIC ${OPENGL_gl_LIBRARY})
if(APPLE)
  # We make it the library user's responsibility to link against some OpenGL.
  set_target_properties(glad PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
