#!/usr/bin/env node
// SPDX-License-Identifier: MIT
//
// muslib.wasm で pmd を raw PCM に書き出す。cli/muslib-render と同じインターフェイスで、
// golden.mjs の --cli にそのまま渡せる。
//
// usage: wasm-render.mjs [-r RATE] [-n MAXBLOCKS] file.pmd > out.raw
//   wasm は環境変数 MUSLIB_WASM (既定は build-wasm/muslib.wasm)

import { readFileSync, writeSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { Muslib } from '../wasm/muslib.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

const { values, positionals } = parseArgs({
    allowPositionals: true,
    options: {
        r: { type: 'string', default: '44100' },
        n: { type: 'string', default: '10000' },
    },
});
if (positionals.length !== 1) {
    console.error('usage: wasm-render.mjs [-r RATE] [-n MAXBLOCKS] file.pmd > out.raw');
    process.exit(2);
}

const wasm = readFileSync(process.env.MUSLIB_WASM ?? path.join(root, 'build-wasm', 'muslib.wasm'));
const m = await Muslib.load(wasm);
m.init(Number(values.r));
if (!m.play(readFileSync(positionals[0]))) {
    console.error(`${positionals[0]}: cannot play`);
    process.exit(1);
}

const maxBlocks = Number(values.n);
const buf = new Int16Array(Muslib.BLOCK_SAMPLES);
const out = Buffer.alloc(maxBlocks * Muslib.BLOCK_SAMPLES * 2);
let n = 0;
for (; n < maxBlocks && m.render(buf); n++) {
    for (let i = 0; i < buf.length; i++) out.writeInt16LE(buf[i], (n * buf.length + i) * 2);
}

// 大きな出力を確実に書き切るため同期で書く
const data = out.subarray(0, n * Muslib.BLOCK_SAMPLES * 2);
for (let off = 0; off < data.length;) off += writeSync(1, data, off);
console.error(`blocks=${n} finished=${m.finished ? 1 : 0}`);
