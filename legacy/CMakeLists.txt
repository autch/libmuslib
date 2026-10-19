# auPiECE.kpi 版 muslib (無改変) + Linux 用シム。golden 生成専用。
file(GLOB LEGACY_WAVE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/muslib/wave/*.c)

add_executable(muslib-legacy-render
  render.c
  muslib/exp12.c
  muslib/instdef.c
  muslib/mus.c
  muslib/seq.c
  muslib/wavetable.c
  ${LEGACY_WAVE_SOURCES}
)
target_include_directories(muslib-legacy-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/muslib)
target_compile_options(muslib-legacy-render PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/shim.h
  -w
  -fwrapv
)
target_link_libraries(muslib-legacy-render PRIVATE m)
