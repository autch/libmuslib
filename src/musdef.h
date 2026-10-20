/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE LIBRARY : muslib : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by MIO.H (OeRSTED)
//
// Comments:
//
// PIECE 標準 音楽ドライバー
//
// ドライバ内定義ファイル
//
//  v1.00 2001.11.09 MIO.H
//  v1.01 2003.02.23 S.Kino.	可変周波数に対応させるために，いくつかの
//														マクロをグローバル変数化
//  v2.00 2026.10.19 Autch		libmuslib: グローバル変数を muslib_t にまとめ、
//														型を固定幅整数にした
//

#ifndef MUSDEF_H
#define MUSDEF_H

#include <stdint.h>

#include "muslib.h"

// 元コードは Windows (LLP64) 前提で long = 32bit。ここでは型を幅で明示する。
//   long           -> int32_t
//   unsigned long  -> uint32_t
//   short          -> int16_t

#define MAXNESTD 24
#define MAXCH MUSLIB_MAX_PARTS	// auPiECE.kpi 1.x は 26。mucc-next は 32 パートまで出力する

#define BLKS MUSLIB_BLOCK_SAMPLES

typedef struct tagINST {
	unsigned char wtype;
	unsigned char param;
	unsigned char resv1;
	unsigned char resv2;
	unsigned short pitch_fs;
	unsigned short pitch_org;
	const signed char *pData;
	uint32_t loop_top;
	uint32_t loop_end;
} INST;

#define IT_LOOP  0x10
#define IT_FAST  0x80

struct muslib;

typedef struct tagMDEVICE {
	const signed char *pData;
	uint32_t freqwk;
	uint32_t freq;
	uint32_t loop_end;
	uint32_t loop_w;
	uint32_t ipitch;
	uint32_t cpitch;
	int32_t ptwk1;
	int32_t ptwk2;
	int32_t envwk;
	int32_t envAR;
	int32_t envDR;
	int32_t envSL;
	int32_t envSR;
	int32_t pvol;
	int32_t pexp;
	int16_t vibwk;
	int16_t vibvv;
	int16_t vibv1;
	int16_t vibv2;
	int16_t vibdpwk;
	int16_t vibdpe;
	unsigned char ch;
	unsigned char vsta;
	void (*genwave)( struct tagMDEVICE *mp, int16_t *p, int32_t vv, int cnt );
	void (*genenv)( const struct muslib *m, struct tagMDEVICE *mp );
	void (*genvib)( struct tagMDEVICE *mp );
} MDEVICE;


typedef struct tagSEQWK {
	MDEVICE mdwk;
	uint16_t pseq;				// 元はポインタ。データ先頭からのオフセットで持つ
	int32_t porvv;
	unsigned short cnt;
	unsigned short len;
	unsigned short stop;
	unsigned short gate;
	unsigned char ch;
	unsigned char run;
	unsigned char np;
	unsigned char tatt;
	unsigned char tsta;
	signed char trs;
	signed char dtn;
	signed char porpd;
	signed char lnote;
	void (*portament)( struct tagSEQWK *psw );
	unsigned short nestd[MAXNESTD];
} SEQWK;


// pmd のオフセットは 16bit なので、データは最大 64KB。
// 読み出しはすべて 16bit で折り返すので、どんなデータでもこの外は読まない。
#define PMD_BUFSIZE 0x10000

struct muslib {
	// レート依存の定数 (元はグローバル変数。winlayer.cpp で設定していた)
	int32_t FS2, MCOUNT, FOFS;
	int32_t VOLS, ENVSTART, ENVSTOP, ENVRR;

	// mus.c
	unsigned char finish;
	MDEVICE *music[MAXCH];
	int16_t pending[BLKS];

	// seq.c
	SEQWK seqwk[MAXCH];
	uint16_t drumsbase;
	uint16_t title;
	uint16_t title2;
	int16_t tempo;
	int16_t tempowk;
	char runf;

	uint8_t data[PMD_BUFSIZE + 1];	// 末尾 1 バイトはタイトル文字列の NUL 終端保証用
};

#define PITCH44K 47396
#define PITCH22K 44324
#define PITCH16K 42903
#define PITCH440 26976
#define PITCH_A4 PITCH440
#define PITCH_C4 (PITCH_A4-9*256)

//              16384                 32
//              -----                 ------
#define IBASE0 (43008+PITCH440-69*256+60*256)

#define WAVEADR(adr) ((const signed char *)adr)

#define NUM_INST 17
extern const INST *const inst[NUM_INST];

// mus.c
void SetInst( const struct muslib *m, MDEVICE *pmd, int no );
void SetVol( const struct muslib *m, MDEVICE *pmd, int vol );
void SetExp( const struct muslib *m, MDEVICE *pmd, int exp );
void SetExpRel( const struct muslib *m, MDEVICE *pmd, int expr );
void SetEnv( const struct muslib *m, MDEVICE *pmd, int ar, int dr, int sl, int sr );
void SetVib( const struct muslib *m, MDEVICE *pmd, int depth, int spd, int t1, int t2 );
void NoteOn( const struct muslib *m, MDEVICE *pmd, int pitch );
void NotePitch( MDEVICE *mp, int pitch );
void NoteOff( MDEVICE *pmd );
void SetupCh( struct muslib *m, MDEVICE *mp, int ch );

// seq.c
void StartSeq( struct muslib *m );
int ProcSeq( struct muslib *m );

// exp12.c
uint32_t gexp( int n );
int32_t muldiv( int32_t a, int32_t b, int32_t c );

#define XSF1 10    // 音程変化（ポルタメント）のシフト量
#define XSF2 14    // 音程変化（ビブラート）のシフト量

#endif // MUSDEF_H
