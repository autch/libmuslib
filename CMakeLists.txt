cmake_minimum_required(VERSION 3.20)
project(libmuslib LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(MUSLIB_BUILD_LEGACY "Build the frozen auPiECE.kpi muslib for golden generation" ON)

file(GLOB MUSLIB_WAVE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/wave/*.c)

add_library(muslib STATIC
  src/exp12.c
  src/instdef.c
  src/mus.c
  src/seq.c
  src/wavetable.c
  ${MUSLIB_WAVE_SOURCES}
)
target_include_directories(muslib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# 符号付き整数のあふれはラップアラウンドとして扱う (元コードの前提)
target_compile_options(muslib PRIVATE -fwrapv -Wall -Wextra)
target_link_libraries(muslib PRIVATE m)

if(NOT EMSCRIPTEN)
  add_executable(muslib-render cli/muslib-render.c)
  target_link_libraries(muslib-render PRIVATE muslib)

  if(MUSLIB_BUILD_LEGACY)
    add_subdirectory(legacy)
  endif()

  # golden との照合。test/ は repo 同梱の fixture。
  # 手元のテストセットは configure 時に MUSLIB_TESTSET と PMD_DIR を設定しておくと追加される。
  enable_testing()
  find_program(NODE_EXECUTABLE node)
  if(NODE_EXECUTABLE)
    add_test(NAME golden-fixtures
      COMMAND ${NODE_EXECUTABLE} tools/golden.mjs check
        --set test --pmd-dir test/fixtures --cli $<TARGET_FILE:muslib-render>
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # 公開テストセット。pmd は tools/fetch-public.mjs で取得しておく
    set(MUSLIB_PUBLIC_PMD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../testsets/public-cache/pmd"
      CACHE PATH "tools/fetch-public.mjs で取得した公開テストセットの pmd")
    if(EXISTS ${MUSLIB_PUBLIC_PMD_DIR})
      add_test(NAME golden-public
        COMMAND ${NODE_EXECUTABLE} tools/golden.mjs check
          --set test/public --pmd-dir ${MUSLIB_PUBLIC_PMD_DIR} --cli $<TARGET_FILE:muslib-render>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    if(DEFINED ENV{MUSLIB_TESTSET} AND DEFINED ENV{PMD_DIR})
      add_test(NAME golden-private
        COMMAND ${NODE_EXECUTABLE} tools/golden.mjs check
          --set $ENV{MUSLIB_TESTSET} --pmd-dir $ENV{PMD_DIR} --cli $<TARGET_FILE:muslib-render>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
  endif()
endif()
