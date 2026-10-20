/*
 * libmuslib - P/ECE 標準音楽ドライバ muslib
 *
 * SPDX-License-Identifier: MIT
 *
 * 公開 API。1 インスタンス = 1 曲の演奏状態で、グローバル変数は持たない。
 *
 *   muslib_t *m = malloc(muslib_size());
 *   muslib_init(m, 44100);
 *   muslib_play(m, pmd, pmd_len);
 *   int16_t buf[MUSLIB_BLOCK_SAMPLES];
 *   while (muslib_render(m, buf)) { ... }
 *
 * 出力は 16bit 符号付きモノラル。
 */
#ifndef MUSLIB_H
#define MUSLIB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUSLIB_BLOCK_SAMPLES 128
#define MUSLIB_MAX_RATE 96000		/* シーケンサのテンポカウンタが 16bit のための上限 */
#define MUSLIB_MAX_PMD_SIZE 0x10000
#define MUSLIB_MAX_PARTS 32		/* 演奏するパート数の上限。これより後ろのパートは鳴らさない */

typedef struct muslib muslib_t;

/* MUSLIB_BLOCK_SAMPLES と同じ値 (wasm から定数を引くため) */
int muslib_block_samples(void);

/* muslib_t に必要なバイト数 (pmd の 64KB バッファを含む) */
size_t muslib_size(void);

/* 出力サンプリング周波数を設定して初期化する。確保した muslib_t は最初にこれを呼ぶこと。
   MUSLIB_MAX_RATE を超える値は MUSLIB_MAX_RATE に丸める。実際に使うレートを返す。
   rate <= 0 のときは 0 を返し、次に正しいレートで呼ぶまで muslib_play() は失敗する。 */
int32_t muslib_init(muslib_t *m, int32_t rate);

/* pmd データを内部バッファにコピーして演奏を始める。
   成功で 0、データが空か MUSLIB_MAX_PMD_SIZE を超えるときは -1。 */
int muslib_play(muslib_t *m, const uint8_t *pmd, size_t len);

/* 演奏を止める。以降 muslib_render() は 0 を返す。 */
void muslib_stop(muslib_t *m);

/* 1 ブロック (MUSLIB_BLOCK_SAMPLES サンプル) を out に書き出して 1 を返す。
   演奏が終わっていれば何も書かずに 0 を返す。 */
int muslib_render(muslib_t *m, int16_t out[MUSLIB_BLOCK_SAMPLES]);

/* 演奏が終わっている (以降 muslib_render() が 0 を返す) なら 1 */
int muslib_finished(const muslib_t *m);

/* pmd に埋め込まれたタイトル (NUL 終端、文字コードは通常 Shift_JIS)。
   muslib_play() の後で有効。オフセットが 0 なら空文字列。 */
const char *muslib_title(const muslib_t *m);
const char *muslib_title2(const muslib_t *m);

/* pmd のヘッダに書かれたパート数。muslib_play() の後で有効。
   MUSLIB_MAX_PARTS を超えていてもそのまま返す (演奏するのは MUSLIB_MAX_PARTS パートまで)。 */
int muslib_num_parts(const muslib_t *m);

#ifdef __cplusplus
}
#endif

#endif /* MUSLIB_H */
