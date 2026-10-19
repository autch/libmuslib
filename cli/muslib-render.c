/*
 * muslib-render - pmd を raw PCM に書き出す
 *
 * SPDX-License-Identifier: MIT
 *
 * usage: muslib-render [-r RATE] [-n MAXBLOCKS] file.pmd > out.raw
 *   stdout: int16 little-endian mono
 *   stderr: "blocks=N finished=0|1"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "muslib.h"

static void usage( void )
{
	fprintf(stderr, "usage: muslib-render [-r RATE] [-n MAXBLOCKS] file.pmd > out.raw\n");
	exit(2);
}

static void put_le16( const int16_t *buf, size_t n, FILE *fp )
{
	unsigned char b[MUSLIB_BLOCK_SAMPLES * 2];
	size_t i;

	for ( i = 0; i < n; i++ ) {
		b[i * 2] = (unsigned char)(buf[i] & 0xff);
		b[i * 2 + 1] = (unsigned char)((uint16_t)buf[i] >> 8);
	}
	fwrite(b, 2, n, fp);
}

int main( int argc, char **argv )
{
	static uint8_t pmd[MUSLIB_MAX_PMD_SIZE + 1];
	int16_t buf[MUSLIB_BLOCK_SAMPLES];
	int rate = 44100, maxblocks = 10000, i, n;
	const char *path = NULL;
	muslib_t *m;
	size_t len;
	FILE *fp;

	for ( i = 1; i < argc; i++ ) {
		if ( !strcmp(argv[i], "-r") && i + 1 < argc ) rate = atoi(argv[++i]);
		else if ( !strcmp(argv[i], "-n") && i + 1 < argc ) maxblocks = atoi(argv[++i]);
		else if ( argv[i][0] != '-' && !path ) path = argv[i];
		else usage();
	}
	if ( !path || rate <= 0 || maxblocks < 0 ) usage();

	if ( !(fp = fopen(path, "rb")) ) { perror(path); return 1; }
	len = fread(pmd, 1, sizeof(pmd), fp);
	fclose(fp);
	if ( len > MUSLIB_MAX_PMD_SIZE ) {
		fprintf(stderr, "%s: too large (max %d bytes)\n", path, MUSLIB_MAX_PMD_SIZE);
		return 1;
	}

	if ( !(m = malloc(muslib_size())) ) { perror("malloc"); return 1; }
	muslib_init(m, rate);
	if ( muslib_play(m, pmd, len) < 0 ) {
		fprintf(stderr, "%s: cannot play\n", path);
		return 1;
	}

	for ( n = 0; n < maxblocks && muslib_render(m, buf); n++ )
		put_le16(buf, MUSLIB_BLOCK_SAMPLES, stdout);

	fprintf(stderr, "blocks=%d finished=%d\n", n, muslib_finished(m));
	free(m);
	return 0;
}
