
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
//  音源(音色)定義ファイルのサンプル
//
//  v1.00 2001.11.09 MIO.H
//  v2.00 2026.10.19 Autch		libmuslib: const 化、未使用の i_d0xx 定義を削除
//



#include "musdef.h"

extern const INST i_square0;
extern const INST i_saw0;
extern const INST i_triangle0;
extern const INST i_square;
extern const INST i_saw;
extern const INST i_triangle;
extern const INST i_BD909;
extern const INST i_CYMBD;
extern const INST i_HANDCLAP;
extern const INST i_HC909;
extern const INST i_HO909;
extern const INST i_SD909;
extern const INST i_SDGATE;
extern const INST i_TOMH1;
extern const INST i_TOML1;
extern const INST i_TOMM1;

const INST *const inst[NUM_INST] = {
	&i_square0,		// 0
	&i_saw0,		// 1
	&i_triangle0,	// 2
	&i_square,		// 3
	&i_saw,			// 4
	&i_triangle,	// 5
	&i_BD909, // 6  bdr
	&i_SDGATE, // 7  sdr
	&i_SD909, // 8  rim
	&i_HO909, // 9  ohh
	&i_HC909, // 10 chh
	&i_CYMBD, // 11 ccy
	&i_CYMBD, // 12 rcy
	&i_TOMH1, // 13 htm
	&i_TOMM1, // 14 mtm
	&i_TOML1, // 15 ltm
	&i_HANDCLAP, // 16 hcp
};
