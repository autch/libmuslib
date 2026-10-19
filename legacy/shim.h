/*
 * legacy/shim.h - auPiECE.kpi 版 muslib を Windows 以外で無改変のままビルドするための強制インクルード
 *
 * SPDX-License-Identifier: MIT
 *
 * legacy/muslib/ 以下は auPiECE.kpi からの無改変コピーで、golden 生成専用。
 * コンパイラに -include でこのファイルを先に読ませる。
 */
#ifndef LEGACY_SHIM_H
#define LEGACY_SHIM_H

/* muslib が読むシステムヘッダは long の再定義より前にここで展開しておく
   (インクルードガードにより muslib 側の #include は空振りになる) */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 元コードは Windows (LLP64) 前提で long = 32bit。LP64 で 64bit になると
   unsigned long の位相アキュムレータ等のラップアラウンドが変わるので揃える。 */
#define long int

typedef struct tagPCEWAVEINFO {
	volatile unsigned char stat;
	unsigned char type;
	unsigned short resv;
	const void *pData;
	unsigned long len;
	struct tagPCEWAVEINFO *next;
	void (*pfEndProc)( struct tagPCEWAVEINFO *);
} PCEWAVEINFO;

#define PW_TYPE_16BITPCM   1
#define PW_TYPE_CONT       0x10

int pceWaveDataOut( int ch, PCEWAVEINFO *pwave );

#define ZeroMemory(p, n) memset((p), 0, (n))

int MulDiv( int nNumber, int nNumerator, int nDenominator );

#endif
