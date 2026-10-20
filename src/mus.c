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
// 音源ドライバー
//
//  v1.00 2001.11.09 MIO.H
//  v1.01 2003.02.23 S.Kino.	可変周波数に対応
//  v1.02 2003   ?   Yui N.		演奏終了を検出できるようにした
//  v1.03 2003.05.07 Yui N.		はらやんさんのプチノイズ対策を実装
//  v1.04 2003.08.03 Yui N., Kobarin
//														Kobarin さんの msvcrt レス化パッチを取り込むも、
//														コンパイラを変更したので無用に（ぉ
//  v1.05 2003.08.07 Yui N.   エンベロープとビブラートの調整が大嘘だったので修正;;
//  v2.00 2026.10.19 Autch		libmuslib: グローバル変数を muslib_t にまとめ、
//														P/ECE カーネルのバッファリング (pceWaveDataOut) を
//														muslib_render() に置き換えた
//

#include <math.h>
#include <string.h>

#include "musdef.h"

// --------------------------------
// 波形生成

void MakeWaveLP( MDEVICE *mp, int16_t *p, int32_t vv, int cnt )
{
	const signed char *tbl = mp->pData;
	uint32_t cc = mp->freqwk;
	do {
		uint32_t o = ( cc >> 14 );
		uint32_t x = ( cc & 0x3fff );
		int32_t d1 = tbl[o];
		int32_t d2 = tbl[o+1];
		if ( (cc += mp->freq) >= mp->loop_end ) {
			cc -= mp->loop_w;
			// 1 サンプルでループ 1 周以上進む (出力レートに対して音程が高すぎる) と
			// 1 回の減算ではループ内に戻らず、次で表の外を読む。元コードにはない対策で、
			// 通常の音域では起きないので出力は変わらない。
			if ( cc >= mp->loop_end ) {
				uint32_t top = mp->loop_end - mp->loop_w;
				cc = top + ( cc - top ) % mp->loop_w;
			}
		}
		d2 -= d1;
		d2 *= x;
		d2 >>= 14;
		d1 += d2;
		d1 *= vv;
		*p++ += (int16_t)(d1>>8);
	} while ( --cnt );
	mp->freqwk = cc;
}

void MakeWaveNL( MDEVICE *mp, int16_t *p, int32_t vv, int cnt )
{
	const signed char *tbl = mp->pData;
	uint32_t cc = mp->freqwk;
	do {
		uint32_t o = ( cc >> 14 );
		uint32_t x = ( cc & 0x3fff );
		int32_t d1 = tbl[o];
		int32_t d2 = tbl[o+1];
		if ( (cc += mp->freq) >= mp->loop_end ) {
			mp->freq = 0;
			return;
		}
		d2 -= d1;
		d2 *= x;
		d2 >>= 14;
		d1 += d2;
		d1 *= vv;
		*p++ += (int16_t)(d1>>8);
	} while ( --cnt );
	mp->freqwk = cc;
}

// 矩形波に最適化
//
void MakeWaveSQR( MDEVICE *mp, int16_t *p, int32_t vv, int cnt )
{
	int32_t cc = mp->freqwk;
	vv >>= 1;
	do {
		*p++ += (int16_t)((cc & 0x8000) ? vv : -vv);
		cc += mp->freq;
	} while ( --cnt );
	mp->freqwk = cc;
}

// のこぎり波に最適化
//
void MakeWaveSAW( MDEVICE *mp, int16_t *p, int32_t vv, int cnt )
{
	int32_t cc = mp->freqwk;
	do {
		int16_t aa = (int16_t)cc;
		*p++ += (int16_t)((aa * vv) >>16);
		cc += mp->freq;
	} while ( --cnt );
	mp->freqwk = cc;
}

// 三角波に最適化
//
void MakeWaveTRI( MDEVICE *mp, int16_t *p, int32_t vv, int cnt )
{
	int16_t cc = (int16_t)mp->freqwk;
	vv >>= 1;
	do {
		int32_t dd = cc * vv;
		if ( cc < 0 ) dd = -dd;
		dd -= (vv<<14);
		*p++ += (int16_t)(dd >> 14);
		cc += (int16_t)mp->freq;
	} while ( --cnt );
	mp->freqwk = cc;
}

// --------------------------------
// エンベロープ・ジェネレーター

// SK : 可変周波数対応 エンベロープスピード生成関数
static int32_t SetEnvSpeed2( const struct muslib *m, int n )
{
	// 2003.08.07 Yui N.: これはレート値なので Freq. の調整が必要
	return muldiv((int32_t)(gexp( n*192 + 4*12*256 )<<m->VOLS), 16000, m->FS2);
}

static void GenEnvR( const struct muslib *m, MDEVICE *mp )
{
	if ( ( mp->envwk -= m->ENVRR ) < m->ENVSTOP ) {
		mp->envwk = m->ENVSTOP;
		mp->freq = 0;
	}
}

static void GenEnvS( const struct muslib *m, MDEVICE *mp )
{
	if ( ( mp->envwk -= mp->envSR ) < m->ENVSTOP ) {
		mp->envwk = m->ENVSTOP;
		mp->freq = 0;
	}
}

static void GenEnvD( const struct muslib *m, MDEVICE *mp )
{
	(void)m;
	if ( ( mp->envwk -= mp->envDR ) < mp->envSL ) {
		mp->genenv = GenEnvS;
	}
}

static void GenEnvA( const struct muslib *m, MDEVICE *mp )
{
	(void)m;
	if ( ( mp->envwk += mp->envAR ) > 0 ) {
		mp->envwk = 0;
		mp->genenv = GenEnvD;
	}
}

static void GenEnvInit( const struct muslib *m, MDEVICE *mp )
{
	mp->envwk = mp->envAR ? m->ENVSTART : 0;
	mp->genenv = GenEnvA;
}


void SetEnv( const struct muslib *m, MDEVICE *mp, int ar, int dr, int sl, int sr )
{
	if ( sl > 127 ) sl = 127;
	mp->envSL = (sl-127) * (192*(1<<m->VOLS));
	mp->envAR = SetEnvSpeed2( m, ar );
	mp->envDR = SetEnvSpeed2( m, dr );
	mp->envSR = SetEnvSpeed2( m, sr );
}

// --------------------------------
// ビブラート・ジェネレーター

#define T1CONST 0x4000

// vv (リニア音量) の上限。gexp() は元々テーブルの都合で 65535 止まりで、
// 波形生成側の d1 * vv >> 8 と int16_t への格納がその範囲で釣り合って
// いたが、gexp() のテーブルあふれ修正でこの頭打ちが外れた。呼び出し側で
// 明示的に抑える。
#define VVMAX 65535

// SK : 可変周波数対応 ビブラートスピード生成関数
// spd 用。vibwk は int16_t で 2^16 を法として位相を持つので、
// int16_t への切り捨て (= 2^16 の剰余) はそのままで正しい。
static int16_t SetVibSpeed2( const struct muslib *m, int n )
{
	// 2003.08.07 Yui N.: これはレート値なので Freq. の調整が必要
	return (int16_t)muldiv((int32_t)gexp( n * 192 + 8*12*256 ), 16000, m->FS2);
}

// t1/t2 用。こちらは T1CONST / vibdpe との >= 比較に使うので、切り捨てで
// 負値になると比較が成立せずビブラートが進行しない (深さが 0 のまま、
// あるいはかかりっぱなしになる)。大きい値の意味は「最速」なので上限で
// 飽和させる。
static int16_t SetVibRate2( const struct muslib *m, int n )
{
	int32_t r = muldiv((int32_t)gexp( n * 192 + 8*12*256 ), 16000, m->FS2);

	return (int16_t)( r > 32767 ? 32767 : r );
}

static void GenVibS2( MDEVICE *mp )
{
	int a = ( mp->vibwk += mp->vibvv );
	if ( a < 0 ) a = -a;
	a = 0x4000 - a;
	mp->ptwk2 = ((a * mp->vibdpwk)>>3);
}

static void GenVibS1( MDEVICE *mp )
{
	if ( ( mp->vibdpwk += mp->vibv2 ) >= mp->vibdpe ) {
		mp->genvib = GenVibS2;
		mp->vibdpwk = mp->vibdpe;
	}
	GenVibS2( mp );
}

static void GenVibS0( MDEVICE *mp )
{
	if ( ( mp->vibdpwk += mp->vibv1 ) >= T1CONST ) {
		mp->genvib = GenVibS1;
		mp->vibwk = 0;
		mp->vibdpwk = 0;
	}
}

static void GenVibInit( MDEVICE *mp )
{
	mp->genvib = GenVibS0;
	mp->vibdpwk = 0;
}

void SetVib( const struct muslib *m, MDEVICE *mp, int depth, int spd, int t1, int t2 )
{
	mp->vibdpe = depth * 12 * 8;
	mp->vibvv  = SetVibSpeed2( m, spd );
	mp->vibv1  = SetVibRate2( m, t1 );
	mp->vibv2  = SetVibRate2( m, t2 );
}


// 存在しない音色番号は無視して直前の音色のままにする
// (元コードは表の外を読んでクラッシュしていた)。
void SetInst( const struct muslib *m, MDEVICE *mp, int no )
{
	const INST *ip;

	if ( (unsigned int)no >= NUM_INST ) return;
	ip = inst[no];

	mp->pData = ip->pData;
	mp->loop_end = ip->loop_end;
	mp->loop_w = ip->loop_end - ip->loop_top;
	mp->freq = 0;
	mp->ipitch = ip->pitch_fs - ip->pitch_org + ( IBASE0 - m->FOFS );
	mp->envAR = 0;

	if ( ip->wtype & IT_FAST ) {
		switch ( ip->param ) {
			case 0  : mp->genwave = MakeWaveSQR; break;
			case 1  : mp->genwave = MakeWaveSAW; break;
			case 2  : mp->genwave = MakeWaveTRI; break;
		}
	}
	else if ( ip->wtype & IT_LOOP ) {
		mp->genwave = MakeWaveLP;
	}
	else {
		mp->genwave = MakeWaveNL;
	}
}

void SetVol( const struct muslib *m, MDEVICE *mp, int vol )
{
	mp->pvol = ((vol * 192 + 11*12*256)<<m->VOLS);
	mp->pexp = 0;
}

void SetExp( const struct muslib *m, MDEVICE *mp, int exp )
{
	exp -= 127;
	mp->pexp = ((exp * 192)<<m->VOLS);
}

void SetExpRel( const struct muslib *m, MDEVICE *mp, int expr )
{
	mp->pexp += ((expr * 192)<<m->VOLS);
}

void NoteOn( const struct muslib *m, MDEVICE *mp, int pitch )
{
	mp->freqwk = 0;
	mp->ptwk1 = 0;
	mp->ptwk2 = 0;
	mp->cpitch = mp->ipitch + pitch;
	mp->freq = 1;
	GenEnvInit( m, mp );
	GenVibInit( mp );
}


void NotePitch( MDEVICE *mp, int pitch )
{
	mp->cpitch = mp->ipitch + pitch;
}


void NoteOff( MDEVICE *mp )
{
	mp->genenv = GenEnvR;
}


void SetupCh( struct muslib *m, MDEVICE *mp, int ch )
{
	m->music[ch] = mp;
	if ( mp ) mp->ch = ch;
}


// 1 ブロック分の波形を合成する (元の BuffFree() の合成部分)
static void Synth( struct muslib *m, int16_t *p0 )
{
	int ch;

	memset(p0, 0, sizeof(int16_t) * BLKS);
	for ( ch = 0; ch < MAXCH; ch++ ) {
		MDEVICE *mp = m->music[ch];
		if ( mp && mp->freq ) {
			int32_t vv;

			mp->genvib( mp );
			mp->freq = gexp((int)(mp->cpitch + (mp->ptwk1>>XSF1) + (mp->ptwk2>>XSF2)));

			mp->genenv( m, mp );
			vv = ((mp->pvol + mp->pexp + mp->envwk) >> m->VOLS);
			if ( vv <= 11 ) vv = 0; else vv = gexp( vv );
			if ( vv > VVMAX ) vv = VVMAX;
			mp->genwave( mp, p0, vv, BLKS );
		}
	}
}

// --------------------------------
// 公開 API

int muslib_block_samples( void )
{
	return MUSLIB_BLOCK_SAMPLES;
}

size_t muslib_size( void )
{
	return sizeof(struct muslib);
}

int32_t muslib_init( muslib_t *m, int32_t rate )
{
	memset(m, 0, sizeof(*m));
	if ( rate <= 0 ) return 0;

	// 以下は auPiECE.kpi の winlayer.cpp (AuPIECEKPI::Open) から
	m->FS2 = rate > MUSLIB_MAX_RATE ? MUSLIB_MAX_RATE : rate;

	// (long)(FS2/128.0*2.5*16.0+0.5) == (FS2*40+64)/128
	m->MCOUNT = (m->FS2 * 40 + 64) >> 7;

	m->FOFS = (int32_t)(3072.0 * log((double)m->FS2) / log(2.0) + 0.5);

	// VOLS はエンベロープ値の固定小数点の小数部ビット数。
	// エンベロープ速度は 128 サンプルごとの加算値なので FS2 に反比例して
	// 小さくなり、VOLS = 1 のままだと整数化の誤差が無視できなくなる
	// (44.1kHz で最大 45%、96kHz では 0 になってエンベロープが止まる)。
	// 小数ビットを増やして誤差を 1% 程度に抑える。
	// pvol/pexp/envwk の和は (82752 << VOLS) 程度なので 32bit に収まる。
	m->VOLS = 6;

	// 2003.08.07 Yui N.
	// ENVSTART と ENVSTOP はレート値ではないので Freq. による調整は不要！
	m->ENVSTART = ((-4 * 12 * 256) * (1 << m->VOLS));
	m->ENVSTOP = ((-10 * 12 * 256) * (1 << m->VOLS));

	// ENVRR はその名のとおりレート値なので調整が必要
	// Freq. が上がったら増やすのではなくて減らす
	m->ENVRR = muldiv(4000 << m->VOLS, 16000, m->FS2);

	m->finish = 1;
	return m->FS2;
}

int muslib_play( muslib_t *m, const uint8_t *pmd, size_t len )
{
	if ( m->FS2 == 0 || len == 0 || len > MUSLIB_MAX_PMD_SIZE ) return -1;

	// 前の曲の状態を持ち越さない (kpi は 1 プロセスで最初の曲はゼロ初期化された
	// グローバル変数から始まっていたので、それと同じ状態にする)
	m->finish = 1;
	memset(m->music, 0, sizeof(m->music));
	memset(m->seqwk, 0, sizeof(m->seqwk));
	memset(m->data, 0, sizeof(m->data));
	memcpy(m->data, pmd, len);

	StartSeq( m );

	// 元の PlayMusic() はゼロクリアしたバッファを 1 つ出力キューに積んでから
	// 演奏を始めていた (はらやんさんのプチノイズ対策) ので、最初の 1 ブロックは
	// 無音になる。以降は「出力済みの次のブロック」を 1 つ先に合成しておく。
	// 元の BuffFree() は初回だけ 3 ブロック先まで合成していたが、その時点では
	// どのチャンネルも freq == 0 で何も起きないので 1 ブロック分と等価。
	memset(m->pending, 0, sizeof(m->pending));
	m->finish = 0;
	return 0;
}

void muslib_stop( muslib_t *m )
{
	if ( !m->finish ) m->finish = 1;
}

int muslib_render( muslib_t *m, int16_t out[MUSLIB_BLOCK_SAMPLES] )
{
	if ( m->finish ) return 0;

	memcpy(out, m->pending, sizeof(m->pending));
	Synth( m, m->pending );
	if ( !ProcSeq( m ) ) m->finish = 2;
	return 1;
}

int muslib_finished( const muslib_t *m )
{
	return m->finish != 0;
}

const char *muslib_title( const muslib_t *m )
{
	return m->title ? (const char *)m->data + m->title : "";
}

const char *muslib_title2( const muslib_t *m )
{
	return m->title2 ? (const char *)m->data + m->title2 : "";
}

int muslib_num_parts( const muslib_t *m )
{
	return m->parts;
}
