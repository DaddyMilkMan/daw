# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-src"
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-build"
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix"
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix/tmp"
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp"
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix/src"
  "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/micah/Desktop/zenith/daw/_deps/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
