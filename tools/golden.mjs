#!/usr/bin/env node
// SPDX-License-Identifier: MIT
//
// golden PCM の生成と照合。
//
//   node tools/golden.mjs make  --cli <renderer> --pmd-dir <dir> [--out test/golden.json] [--exclude test/excluded.json]
//   node tools/golden.mjs check --cli <renderer> --pmd-dir <dir> [--golden test/golden.json] [--ref <renderer>]
//
// <renderer> は `<renderer> -r RATE -n MAXBLOCKS file.pmd` で int16le raw を stdout に、
// "blocks=N finished=0|1" を stderr に出すコマンド。
// pmd ファイルは再配布できないものを含むので repo には置かず、ハッシュだけを記録する。
// 壊れていて実機でも正常に鳴らない pmd は excluded.json に { "ファイル名": "理由" } で
// 列挙して golden から外す (C 版の出力が未定義動作に依存するため基準にできない)。
// check で --ref (通常は muslib-legacy-render) を渡すと、不一致のときに最初にずれた
// ブロック番号とサンプル位置まで絞り込む。

import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { readFile, readdir, writeFile } from 'node:fs/promises';
import { availableParallelism } from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

export const RATES = [16000, 44100, 48000];
export const MAX_BLOCKS = 10000;
export const BLOCK_SAMPLES = 128;
export const SEGMENT_BLOCKS = 500;

const BLOCK_BYTES = BLOCK_SAMPLES * 2;

export function render(cli, file, rate, maxBlocks = MAX_BLOCKS) {
    return new Promise((resolve, reject) => {
        const p = spawn(cli, ['-r', String(rate), '-n', String(maxBlocks), file]);
        const out = [];
        let err = '';
        p.stdout.on('data', (b) => out.push(b));
        p.stderr.on('data', (b) => { err += b; });
        p.on('error', reject);
        p.on('close', (code) => {
            const m = /blocks=(\d+) finished=([01])/.exec(err);
            if (code !== 0 || !m) {
                reject(new Error(`${cli} ${file} @${rate}: exit ${code}: ${err.trim()}`));
                return;
            }
            const pcm = Buffer.concat(out);
            const blocks = Number(m[1]);
            if (pcm.length !== blocks * BLOCK_BYTES) {
                reject(new Error(`${file} @${rate}: ${pcm.length} bytes for ${blocks} blocks`));
                return;
            }
            resolve({ pcm, blocks, finished: m[2] === '1' });
        });
    });
}

const sha256 = (buf) => createHash('sha256').update(buf).digest('hex');

export function digest({ pcm, blocks, finished }) {
    const segments = [];
    for (let off = 0; off < pcm.length; off += SEGMENT_BLOCKS * BLOCK_BYTES) {
        segments.push(sha256(pcm.subarray(off, off + SEGMENT_BLOCKS * BLOCK_BYTES)).slice(0, 16));
    }
    return { blocks, finished, sha256: sha256(pcm), segments };
}

async function listPmd(dir) {
    const names = (await readdir(dir)).filter((f) => /\.pmd$/i.test(f)).sort();
    return Promise.all(names.map(async (name) => {
        const file = path.join(dir, name);
        return { name, file, pmdSha256: sha256(await readFile(file)) };
    }));
}

async function pool(items, fn) {
    const results = new Array(items.length);
    let next = 0;
    const worker = async () => {
        while (next < items.length) {
            const i = next++;
            results[i] = await fn(items[i], i);
        }
    };
    await Promise.all(Array.from({ length: availableParallelism() }, worker));
    return results;
}

async function make({ cli, pmdDir, out, exclude }) {
    const excluded = JSON.parse(await readFile(exclude, 'utf8'));
    const pmds = (await listPmd(pmdDir)).filter((p) => !(p.name in excluded));
    const jobs = pmds.flatMap((p) => RATES.map((rate) => ({ ...p, rate })));
    const entries = await pool(jobs, async (j) => ({
        file: j.name,
        pmdSha256: j.pmdSha256,
        rate: j.rate,
        ...digest(await render(cli, j.file, j.rate)),
    }));
    const golden = {
        generator: path.basename(cli),
        maxBlocks: MAX_BLOCKS,
        blockSamples: BLOCK_SAMPLES,
        segmentBlocks: SEGMENT_BLOCKS,
        format: 'int16le mono',
        entries,
    };
    await writeFile(out, JSON.stringify(golden, null, 1) + '\n');
    const finished = entries.filter((e) => e.finished).length;
    console.log(`${entries.length} entries (${pmds.length} files x ${RATES.length} rates, ${finished} finished) -> ${out}`);
    console.log(`excluded: ${Object.keys(excluded).join(', ') || '(none)'}`);
}

function firstDiff(a, b) {
    const n = Math.min(a.length, b.length) >> 1;
    for (let i = 0; i < n; i++) {
        if (a.readInt16LE(i * 2) !== b.readInt16LE(i * 2)) return i;
    }
    return a.length === b.length ? -1 : n;
}

async function check({ cli, pmdDir, golden: goldenPath, ref }) {
    const golden = JSON.parse(await readFile(goldenPath, 'utf8'));
    const pmds = new Map((await listPmd(pmdDir)).map((p) => [p.name, p]));
    let pass = 0, fail = 0, skip = 0;

    await pool(golden.entries, async (e) => {
        const p = pmds.get(e.file);
        if (!p || p.pmdSha256 !== e.pmdSha256) {
            skip++;
            return;
        }
        const r = await render(cli, p.file, e.rate, golden.maxBlocks);
        const d = digest(r);
        if (d.sha256 === e.sha256 && d.blocks === e.blocks && d.finished === e.finished) {
            pass++;
            return;
        }
        fail++;
        let msg = `FAIL ${e.file} @${e.rate}: blocks ${d.blocks}/${e.blocks} finished ${d.finished}/${e.finished}`;
        const seg = e.segments.findIndex((h, i) => d.segments[i] !== h);
        if (seg >= 0) msg += `, first bad segment ${seg} (blocks ${seg * golden.segmentBlocks}-)`;
        if (ref) {
            const rr = await render(ref, p.file, e.rate, golden.maxBlocks);
            const s = firstDiff(r.pcm, rr.pcm);
            if (s >= 0) msg += `, first diff at block ${Math.floor(s / BLOCK_SAMPLES)} sample ${s % BLOCK_SAMPLES}`;
        }
        console.log(msg);
    });

    console.log(`pass ${pass}, fail ${fail}, skip ${skip} (missing or different pmd)`);
    if (fail || !pass) process.exitCode = 1;
}

async function main() {
    const { positionals, values } = parseArgs({
        allowPositionals: true,
        options: {
            cli: { type: 'string' },
            'pmd-dir': { type: 'string', default: process.env.PMD_DIR },
            out: { type: 'string', default: 'test/golden.json' },
            golden: { type: 'string', default: 'test/golden.json' },
            exclude: { type: 'string', default: 'test/excluded.json' },
            ref: { type: 'string' },
        },
    });
    const opts = { ...values, pmdDir: values['pmd-dir'] };

    if (!opts.cli || !opts.pmdDir || !['make', 'check'].includes(positionals[0])) {
        console.error('usage: golden.mjs make|check --cli <renderer> --pmd-dir <dir> (or PMD_DIR)');
        process.exit(2);
    }
    await (positionals[0] === 'make' ? make(opts) : check(opts));
}

if (process.argv[1] === fileURLToPath(import.meta.url)) await main();
