cmake_minimum_required(VERSION 3.20)
project(libmuslib LANGUAGES C)

option(MUSLIB_BUILD_LEGACY "Build the frozen auPiECE.kpi muslib for golden generation" ON)

if(MUSLIB_BUILD_LEGACY AND NOT EMSCRIPTEN)
  add_subdirectory(legacy)
endif()
