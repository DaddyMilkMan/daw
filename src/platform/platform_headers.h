#pragma once

// Consolidated platform headers for rendering targets and backends.

#if defined(_WIN32)
  #include <Windows.h>
  #include <GL/gl.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl3.h>
#else
  #include <GL/gl.h>
#endif
