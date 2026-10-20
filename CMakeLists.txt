cmake_minimum_required(VERSION 3.21)
project(libmuslib LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 他のプロジェクトから add_subdirectory したときは、ライブラリ本体だけをビルドする
option(MUSLIB_BUILD_TOOLS "Build muslib-render and the golden tests" ${PROJECT_IS_TOP_LEVEL})
option(MUSLIB_BUILD_LEGACY "Build the frozen auPiECE.kpi muslib for golden generation" ${PROJECT_IS_TOP_LEVEL})

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
# 符号付き整数のあふれはラップアラウンドとして扱う (元コードの前提)。
# MSVC (cl) には -fwrapv に当たるオプションがないので、cl でビルドしたときは
# muslib-render と golden テスト (ctest) で出力が一致することを確かめること。
if(MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
  target_compile_options(muslib PRIVATE /utf-8 /clang:-fwrapv /W3)
elseif(MSVC)
  target_compile_options(muslib PRIVATE /utf-8 /W3)
else()
  target_compile_options(muslib PRIVATE -fwrapv -Wall -Wextra)
endif()
if(NOT WIN32)
  target_link_libraries(muslib PRIVATE m)
endif()

if(EMSCRIPTEN)
  # JS グルーなしの単体 wasm (muslib.wasm) を作る
  set(MUSLIB_WASM_EXPORTS
    _muslib_block_samples _muslib_size _muslib_init _muslib_play _muslib_stop
    _muslib_render _muslib_finished _muslib_title _muslib_title2 _muslib_num_parts _malloc _free)
  list(JOIN MUSLIB_WASM_EXPORTS "," MUSLIB_WASM_EXPORTS)
  add_executable(muslib-wasm wasm/muslib-wasm.c)
  target_link_libraries(muslib-wasm PRIVATE muslib)
  set_target_properties(muslib-wasm PROPERTIES OUTPUT_NAME muslib SUFFIX ".wasm")
  target_link_options(muslib-wasm PRIVATE
    --no-entry
    -sSTANDALONE_WASM
    -sEXPORTED_FUNCTIONS=${MUSLIB_WASM_EXPORTS}
    -sMALLOC=emmalloc
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=1MB
    -sSTACK_SIZE=64KB
    -sFILESYSTEM=0
    -Oz)
  target_compile_options(muslib-wasm PRIVATE -Oz)
elseif(MUSLIB_BUILD_TOOLS)
  add_executable(muslib-render cli/muslib-render.c)
  target_link_libraries(muslib-render PRIVATE muslib)

  if(MUSLIB_BUILD_LEGACY)
    add_subdirectory(legacy)
  endif()

  # golden との照合。test/ は repo 同梱の fixture。
  # 公開テストセット (test/public) は tools/fetch-public.mjs で pmd を取得しておくと追加される。
  # 手元のテストセットは configure 時に MUSLIB_TESTSET と PMD_DIR を設定しておくと追加される。
  # wasm は別の build ディレクトリで emcmake してビルドしておくと (既定は build-wasm/)、
  # 同じテストセットを wasm でも照合する。
  enable_testing()
  find_program(NODE_EXECUTABLE node)
  set(MUSLIB_PUBLIC_PMD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../testsets/public-cache/pmd"
    CACHE PATH "tools/fetch-public.mjs で取得した公開テストセットの pmd")
  set(MUSLIB_WASM "${CMAKE_CURRENT_SOURCE_DIR}/build-wasm/muslib.wasm"
    CACHE FILEPATH "照合に使う muslib.wasm")

  if(NODE_EXECUTABLE)
    set(sets "fixtures|test|test/fixtures")
    if(EXISTS ${MUSLIB_PUBLIC_PMD_DIR})
      list(APPEND sets "public|test/public|${MUSLIB_PUBLIC_PMD_DIR}")
    endif()
    if(DEFINED ENV{MUSLIB_TESTSET} AND DEFINED ENV{PMD_DIR})
      list(APPEND sets "private|$ENV{MUSLIB_TESTSET}|$ENV{PMD_DIR}")
    endif()

    foreach(s IN LISTS sets)
      string(REPLACE "|" ";" s "${s}")
      list(GET s 0 name)
      list(GET s 1 set)
      list(GET s 2 pmd)
      add_test(NAME golden-${name}
        COMMAND ${NODE_EXECUTABLE} tools/golden.mjs check
          --set ${set} --pmd-dir ${pmd} --cli $<TARGET_FILE:muslib-render>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      if(EXISTS ${MUSLIB_WASM})
        add_test(NAME golden-${name}-wasm
          COMMAND ${NODE_EXECUTABLE} tools/golden.mjs check
            --set ${set} --pmd-dir ${pmd} --cli tools/wasm-render.mjs
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(golden-${name}-wasm PROPERTIES ENVIRONMENT "MUSLIB_WASM=${MUSLIB_WASM}")
      endif()
    endforeach()

    if(EXISTS ${MUSLIB_WASM})
      add_test(NAME wasm-api
        COMMAND ${NODE_EXECUTABLE} --test test/wasm.test.mjs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set_tests_properties(wasm-api PROPERTIES ENVIRONMENT "MUSLIB_WASM=${MUSLIB_WASM}")
    endif()
  endif()
endif()
