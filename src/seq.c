
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
// シーケンサー
//
//  v1.00 2001.11.09 MIO.H
//  v1.01 2003.02.23 S.Kino.	可変周波数に対応
//  v1.02 2003.05.03 Yui N.		デチューンとヴィブラートが初期化されない仕様を改良
//  v2.00 2026.10.19 Autch		libmuslib: グローバル変数を muslib_t にまとめ、
//														シーケンスデータをオフセットで扱い範囲外を読まないようにした
//


#include "musdef.h"

#define TATT_DRUMS 1
#define TATT_NOOFF 2

#define TSTA_PORTASW 1
#define TSTA_INPORTA 2
#define TSTA_LEG1    4
#define TSTA_LEG2    8

#define MAXSEQ MAXCH

// シーケンスデータの読み出し。オフセットは 16bit で折り返すので、
// data[] (64KB + 1) の外は読まない。
static uint8_t rd( const struct muslib *m, uint16_t off )
{
	return m->data[off];
}

static uint16_t getadrs( const struct muslib *m, uint16_t off )
{
	return rd(m, off) + (rd(m, (uint16_t)(off + 1)) << 8);
}

static uint8_t next( const struct muslib *m, SEQWK *psw )
{
	return rd(m, psw->pseq++);
}

typedef void (*SEQCMD)( struct muslib *m, SEQWK *psw );

static void SeqNop( struct muslib *m, SEQWK *psw )
{
	(void)m; (void)psw;
}

static void PortaNop( SEQWK *psw )
{
	(void)psw;
}

void StartSeq( struct muslib *m )
{
	int i, n, s;
	uint16_t seqptr = 0;

	m->tempo = (120<<4);
	m->tempowk = 0;
	m->runf = 1;

	n = rd(m, seqptr);
	if ( !n ) n = rd(m, ++seqptr);
	if ( n > MAXSEQ ) n = MAXSEQ;

	s = 1;
	for ( i = 0; i < n; i++,s+=2 ) {
		SEQWK *psw = m->seqwk + i;
		psw->pseq = getadrs( m, seqptr + s );
		psw->cnt  = 1;
		psw->len  = 1;
		psw->ch   = i;
		psw->np   = 0;
		psw->trs  = 0;
		psw->run  = 1;
		psw->tatt  = 0;
		psw->tsta = 0;
		psw->gate = 1*41; // gate=99
		psw->portament = PortaNop;
		SetupCh( m, &psw->mdwk, i );
		SetInst( m, &psw->mdwk, 0 );
		SetVol( m, &psw->mdwk, 127 - i*16 );
		// yui: ビブラートを初期化
		SetVib( m, &psw->mdwk, 0, 0, 0, 0);
		psw->dtn = 0;			// yui: デチューンも初期化
	}
	for ( ; i < MAXSEQ; i++ ) {
		SEQWK *psw = m->seqwk + i;
		SetupCh( m, 0, i );
		psw->run  = 0;
	}

	// ここは MAXSEQ で丸める前のパート数を使う (元コードどおり)
	s = 1 + rd(m, seqptr) * 2;
	m->drumsbase = getadrs( m, seqptr + s );
	m->title = getadrs( m, seqptr + s + 2 );
	m->title2 = getadrs( m, seqptr + s + 4 );
}

static void SeqPortaExecP( SEQWK *psw )
{
	if ( (psw->mdwk.ptwk1 += psw->porvv) >= 0 ) {
		psw->mdwk.ptwk1 = 0;
		psw->tsta &= ~TSTA_INPORTA;
	}
}

static void SeqPortaExecN( SEQWK *psw )
{
	if ( (psw->mdwk.ptwk1 -= psw->porvv) <= 0 ) {
		psw->mdwk.ptwk1 = 0;
		psw->tsta &= ~TSTA_INPORTA;
	}
}

static void SeqNote( struct muslib *m, SEQWK *psw, int note )
{
	int ntw = ((note + psw->trs)<<8) + psw->dtn;

	psw->cnt = psw->len;

	if ( (psw->tsta & TSTA_LEG2) && psw->run==2 ) {
		NotePitch( &psw->mdwk, ntw );
	}
	else {
		NoteOn( m, &psw->mdwk, ntw );
		psw->run = 2;
	}

	if ( (psw->tsta & (TSTA_LEG1|TSTA_LEG2)) == TSTA_LEG2 ) {
		psw->tsta &= ~TSTA_LEG2;
	}

	if ( psw->tsta & TSTA_PORTASW ) {
		int a = psw->porpd;
		if ( !a ) a = psw->lnote - note;
		psw->mdwk.ptwk1 = a * (1<<(XSF1+8));
		if ( a < 0 ) {
			psw->portament = SeqPortaExecP;
		}
		else {
			psw->portament = SeqPortaExecN;
		}
		psw->tsta |= TSTA_INPORTA;
	}

	psw->lnote = (signed char)note;

	{
		int len = psw->len;
		int a = 0;
		if ( !(psw->tatt & TATT_NOOFF) ) {
			a = ( ( len * psw->gate ) >> 12 );
			if ( a < 1 ) a = 1;
			else if ( a >= len ) a = len-1;
		}
		psw->stop = a;
	}
}

static void SeqNote2( struct muslib *m, SEQWK *psw )
{
	int note = next(m, psw);
	if ( note >= 0x80 ) note -= 0x80-12;
	SeqNote( m, psw, note );
}

// ネストのスタックに 3 要素積む。あふれる場合は積まずに 0 を返す
// (元コードは SEQWK の後続メンバを壊していた。正しいデータでは起きない)。
static int Push( SEQWK *psw, unsigned short cnt, unsigned short ret, unsigned short top )
{
	int np = psw->np;
	if ( np + 3 > MAXNESTD ) return 0;
	psw->nestd[np++] = cnt;
	psw->nestd[np++] = ret;
	psw->nestd[np++] = top;
	psw->np = np;
	return 1;
}

static void SeqDrums( struct muslib *m, SEQWK *psw, int note )
{
	uint16_t ad = getadrs( m, m->drumsbase + (note<<1) );
	if ( Push( psw, 1, psw->pseq, ad ) ) psw->pseq = ad;
}

static void SeqRest( struct muslib *m, SEQWK *psw )
{
	(void)m;
	if ( psw->run == 2 ) {
		NoteOff( &psw->mdwk );
		psw->run = 1;
	}
	psw->cnt = psw->len;
}

static void SeqInst( struct muslib *m, SEQWK *psw )
{
	SetInst( m, &psw->mdwk, next(m, psw) );
}

static void SeqTempo( struct muslib *m, SEQWK *psw )
{
	m->tempo = next(m, psw) << 4;
}

static void SeqEnd( struct muslib *m, SEQWK *psw )
{
	int np = psw->np;
	(void)m;
	if ( np < 3 ) {
		psw->run = 0;
		psw->cnt = 1;
		return;
	}

	np -= 3;
	if ( --psw->nestd[np] ) {
		psw->pseq = psw->nestd[np+2];
	}
	else {
		psw->pseq = psw->nestd[np+1];
		psw->np = np;
	}
}

static void SeqGate( struct muslib *m, SEQWK *psw )
{
	psw->gate = ( 100 - next(m, psw) ) * 41;
}

static void SeqJump( struct muslib *m, SEQWK *psw )
{
	psw->pseq = getadrs( m, psw->pseq );
}

static void SeqCall( struct muslib *m, SEQWK *psw )
{
	uint16_t ad = getadrs( m, psw->pseq );
	uint8_t cnt = rd( m, psw->pseq + 2 );
	psw->pseq += 3;
	if ( Push( psw, cnt, psw->pseq, ad ) ) psw->pseq = ad;
}

static void SeqRept( struct muslib *m, SEQWK *psw )
{
	int np = psw->np;
	uint8_t cnt = next(m, psw);
	// 3 要素目 (ブレーク先) は最初の SeqNext() で書かれるのでここでは触らない
	if ( np + 3 > MAXNESTD ) return;
	psw->nestd[np] = cnt;
	psw->nestd[np+1] = psw->pseq;
	psw->np = np + 3;
}

static void SeqNext( struct muslib *m, SEQWK *psw )
{
	int np = psw->np - 3;
	(void)m;
	if ( np < 0 ) return;
	if ( --psw->nestd[np] ) {
		psw->nestd[np+2] = psw->pseq;
		psw->pseq = psw->nestd[np+1];
	}
	else {
		psw->np = np;
	}
}

static void SeqBreak( struct muslib *m, SEQWK *psw )
{
	int np = psw->np - 3;
	(void)m;
	if ( np < 0 ) return;
	if ( psw->nestd[np] == 1 ) {
		psw->pseq = psw->nestd[np+2];
		psw->np = np;
	}
}

static void SeqTrs( struct muslib *m, SEQWK *psw )
{
	psw->trs = (signed char)next(m, psw);
}

static void SeqEnv( struct muslib *m, SEQWK *psw )
{
	int ar = next(m, psw);
	int dr = next(m, psw);
	int sl = next(m, psw);
	int sr = next(m, psw);
	SetEnv( m, &psw->mdwk, ar, dr, sl, sr );
}

static void SeqVol( struct muslib *m, SEQWK *psw )
{
	SetVol( m, &psw->mdwk, next(m, psw) );
}

static void SeqDtn( struct muslib *m, SEQWK *psw )
{
	psw->dtn = (signed char)next(m, psw);
}

static void SeqPortaPara( struct muslib *m, SEQWK *psw )
{
	psw->porpd = (signed char)next(m, psw);
	psw->porvv = (int32_t)(gexp(next(m, psw) * 192 + 5*12*256) << XSF1);
	// porvv * tempo は速い設定 (テンポ 120 なら速度 162 以上) で 32bit を
	// あふれて負値になり、ポルタメントの進行方向が逆転する
	// (SeqPortaExecP が加算しても 0 に届かず音程が下がり続ける)。
	// muldiv は中間値を 64bit で持つのであふれない。
	psw->porvv = muldiv(psw->porvv, m->tempo, m->MCOUNT);
}

static void SeqPortaOn( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->tsta |= TSTA_PORTASW;
}

static void SeqPortaOff( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->tsta &= ~(TSTA_PORTASW|TSTA_INPORTA);
	psw->mdwk.ptwk1 = 0;
}

static void SeqTatt( struct muslib *m, SEQWK *psw )
{
	psw->tatt = next(m, psw);
}

static void SeqVibrato( struct muslib *m, SEQWK *psw )
{
	int depth = next(m, psw);
	int spd = next(m, psw);
	int t1 = next(m, psw);
	int t2 = next(m, psw);
	SetVib( m, &psw->mdwk, depth, spd, t1, t2 );
}

// 以下 4 つは未実装 (元からパラメータを読み飛ばすだけ)
static void SeqPartFade( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->pseq += 2;
}

static void SeqMasterFade( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->pseq += 2;
}

static void SeqMasterVol( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->pseq++;
}

static void SeqBend( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->pseq += 2;
}

static void SeqLegOn( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->tsta |= (TSTA_LEG1|TSTA_LEG2);
}

static void SeqLegOff( struct muslib *m, SEQWK *psw )
{
	(void)m;
	psw->tsta &= ~TSTA_LEG1;
}

static void SeqExp( struct muslib *m, SEQWK *psw )
{
	SetExp( m, &psw->mdwk, next(m, psw) );
}

static void SeqExpRel( struct muslib *m, SEQWK *psw )
{
	SetExpRel( m, &psw->mdwk, (signed char)next(m, psw) );
}

static const SEQCMD SeqCmd[32] = {
	SeqRest,		// 0xe0
	SeqGate,		// 0xe1
	SeqJump,		// 0xe2
	SeqCall,		// 0xe3
	SeqRept,		// 0xe4
	SeqNext,		// 0xe5
	SeqTrs,			// 0xe6
	SeqTempo,		// 0xe7
	SeqInst,		// 0xe8
	SeqVol,			// 0xe9
	SeqEnv,			// 0xea
	SeqDtn,			// 0xeb
	SeqNote2,		// 0xec
	SeqPortaPara,	// 0xed
	SeqPortaOn,		// 0xee
	SeqPortaOff,	// 0xef
	SeqTatt,		// 0xf0
	SeqVibrato,		// 0xf1
	SeqMasterVol,	// 0xf2
	SeqMasterFade,	// 0xf3
	SeqPartFade,	// 0xf4
	SeqBend,		// 0xf5
	SeqBreak,		// 0xf6
	SeqNop,			//
	SeqLegOn,		// 0xf8
	SeqLegOff,		// 0xf9
	SeqExp,			// 0xfa
	SeqExpRel,		// 0xfb
	SeqNop,
	SeqNop,
	SeqNop,
	SeqNop,
};

int ProcSeq( struct muslib *m )
{
	SEQWK *psw;
	int i;

	for ( i = 0,psw = m->seqwk; i < MAXSEQ; i++,psw++ ) {
		if ( psw->run ) {
			if ( psw->tsta & TSTA_INPORTA ) psw->portament( psw );
		}
	}

	if ( ( m->tempowk -= m->tempo ) >= 0 ) return m->runf;
	m->tempowk += (int16_t)m->MCOUNT;

	m->runf = 0;
	for ( i = 0,psw = m->seqwk; i < MAXSEQ; i++,psw++ ) {
		m->runf += psw->run;
		if ( psw->run ) {
			if ( !--psw->cnt ) {
				do {
					int a = next(m, psw);
					if ( a >= 0xe0 ) {
						(SeqCmd[a-0xe0])( m, psw );
					}
					else if ( a >= 0x80 ) {
						if ( psw->tatt & TATT_DRUMS ) {
							SeqDrums( m, psw, a-0x80 );
						}
						else {
							SeqNote( m, psw, a-0x80+12 );
						}
					}
					else if ( a ) {
						if ( a == 127 ) {
							a = next(m, psw);
							if ( a < 127 ) a += 256;
						}
						psw->len = a;
					}
					else {
						SeqEnd( m, psw );
					}
				} while ( !psw->cnt );
			}
			else {
				if (psw->cnt==psw->stop && psw->run==2 && !(psw->tsta & TSTA_LEG1)) {
					NoteOff( &psw->mdwk );
					psw->run = 1;
				}
			}
		}
	}

	return m->runf;
}
