/*
 * legacy/render.c - auPiECE.kpi 版 muslib で pmd を raw PCM に書き出す (golden 生成用)
 *
 * SPDX-License-Identifier: MIT
 *
 * auPiECE.kpi の winlayer.cpp (AuPIECEKPI::Open / Render, pceWaveDataOut) を
 * 1 ブロック (128 サンプル) 単位の Render 呼び出しとして再現する。
 *
 * usage: muslib-legacy-render -r RATE -n MAXBLOCKS file.pmd > out.raw
 *   stdout: int16 little-endian mono
 *   stderr: "blocks=N finished=0|1"
 */
#include "muslib/musdef.h"
#include "muslib/muslib.h"

#define BLKS 128
#define MAX_FS 96000
#define PMD_BUFSIZE 0x10000

/* winlayer.cpp */
long FS, FS2, MCOUNT, FOFS;
/* mus.c */
extern long VOLS, ENVSTART, ENVSTOP, ENVRR;
extern unsigned char finish;

static PCEWAVEINFO *pWave;
static const signed short *pBuffer;

int pceWaveDataOut( int ch, PCEWAVEINFO *pwave )
{
	(void)ch;
	pBuffer = pwave->pData;
	pWave = pwave;
	return 0;
}

/* Windows の MulDiv 互換: 64bit 中間値、0.5 は 0 から遠い側へ丸め、あふれ / 0 除算は -1 */
int MulDiv( int a, int b, int c )
{
	int64_t r;

	if ( c == 0 ) return -1;
	if ( c < 0 ) { a = -a; c = -c; }
	if ( (a < 0) == (b < 0) )
		r = ((int64_t)a * b + c / 2) / c;
	else
		r = ((int64_t)a * b - c / 2) / c;
	if ( r > 2147483647 || r < -2147483647 ) return -1;
	return (int)r;
}

/* AuPIECEKPI::Open() のレート依存定数の設定部分 */
static void setup_rate( int rate )
{
	FS2 = rate;
	if ( FS2 > MAX_FS ) FS2 = MAX_FS;
	FS = FS2 << 1;
	MCOUNT = (FS2 * 40 + 64) >> 7;
	FOFS = (long)(3072.0 * log((double)FS2) / log(2.0) + 0.5);
	VOLS = 6;
	ENVSTART = ((-4 * 12 * 256) << VOLS);
	ENVSTOP = ((-10 * 12 * 256) << VOLS);
	ENVRR = MulDiv(4000 << VOLS, 16000, FS2);
}

static void usage( void )
{
	fprintf(stderr, "usage: muslib-legacy-render -r RATE -n MAXBLOCKS file.pmd > out.raw\n");
	exit(2);
}

int main( int argc, char **argv )
{
	static unsigned char seq[PMD_BUFSIZE + 1];
	int rate = 44100, maxblocks = 10000, i, n;
	const char *path = NULL;
	FILE *fp;

	for ( i = 1; i < argc; i++ ) {
		if ( !strcmp(argv[i], "-r") && i + 1 < argc ) rate = atoi(argv[++i]);
		else if ( !strcmp(argv[i], "-n") && i + 1 < argc ) maxblocks = atoi(argv[++i]);
		else if ( argv[i][0] != '-' && !path ) path = argv[i];
		else usage();
	}
	if ( !path || rate <= 0 || maxblocks < 0 ) usage();

	if ( !(fp = fopen(path, "rb")) ) { perror(path); return 1; }
	/* kpi は fsize+1 を未初期化で確保していたが、ここでは 64KB をゼロ埋めして決定的にする */
	fread(seq, 1, PMD_BUFSIZE, fp);
	fclose(fp);

	setup_rate(rate);
	InitMusic();
	PlayMusic(seq);

	/* AuPIECEKPI::Render() を dwSize = 1 ブロックで呼び続けるのと同じ */
	for ( n = 0; n < maxblocks && !finish; n++ ) {
		fwrite(pBuffer, sizeof(signed short), BLKS, stdout);
		pWave->pfEndProc(pWave);
	}

	fprintf(stderr, "blocks=%d finished=%d\n", n, finish ? 1 : 0);
	return 0;
}
