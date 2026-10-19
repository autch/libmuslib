/*
 * wasm/muslib-wasm.c - WebAssembly ビルドのエントリ
 *
 * SPDX-License-Identifier: MIT
 *
 * wasm で export するのは include/muslib.h の公開 API と malloc / free だけ
 * (CMakeLists.txt の EXPORTED_FUNCTIONS)。JS グルーは使わず、
 * 呼び出し側が WebAssembly.instantiate() して直接呼ぶ。
 *
 * 使い方 (JS):
 *   const m = malloc(muslib_size());  muslib_init(m, 48000);
 *   const p = malloc(pmd.length);     // pmd を HEAPU8 にコピー
 *   muslib_play(m, p, pmd.length);    free(p);
 *   const out = malloc(MUSLIB_BLOCK_SAMPLES * 2);
 *   while (muslib_render(m, out)) { ... HEAP16 から読む ... }
 *
 * メモリは必要に応じて伸びるので、伸びたあとは memory.buffer のビューを作り直すこと。
 */
#include "muslib.h"
