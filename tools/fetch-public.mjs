#!/usr/bin/env node
// SPDX-License-Identifier: MIT
//
// 公開テストセット (test/public) の pmd を入手元から取得する。
//
//   node tools/fetch-public.mjs [--out <cache>]
//
// <cache> (既定は環境変数 MUSLIB_PUBLIC_CACHE、なければ ../testsets/public-cache) に
//   archives/  ダウンロードしたアーカイブ (取得済みでハッシュが合えば再取得しない)
//   pmd/       取り出した pmd (golden.mjs の --pmd-dir に渡す)
// を作る。pmd は再配布できないので repo には置かない。
//
// 展開には 7z を使う (zip / lzh / 自己解凍 exe をまとめて扱える)。
// 自己解凍形式のアーカイブは決して実行しない。取り出した pmd は
// sources.json の SHA-256 と照合し、合わないものは置かない。

import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { copyFile, mkdir, mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const sha256 = (buf) => createHash('sha256').update(buf).digest('hex');

async function readIfExists(file) {
    try {
        return await readFile(file);
    } catch {
        return null;
    }
}

function run(cmd, args) {
    return new Promise((resolve, reject) => {
        const p = spawn(cmd, args, { stdio: ['ignore', 'ignore', 'pipe'] });
        let err = '';
        p.stderr.on('data', (b) => { err += b; });
        p.on('error', reject);
        p.on('close', (code) => code === 0 ? resolve() : reject(new Error(`${cmd} exit ${code}: ${err.trim()}`)));
    });
}

async function download(src, file) {
    const cached = await readIfExists(file);
    if (cached && sha256(cached) === src.archiveSha256) return;

    const res = await fetch(src.url);
    if (!res.ok) throw new Error(`${src.url}: HTTP ${res.status}`);
    const buf = Buffer.from(await res.arrayBuffer());
    if (sha256(buf) !== src.archiveSha256) {
        // 入手元で差し替えられた可能性がある。中の pmd のハッシュで判断するので続行する
        console.warn(`warning: ${src.url}: アーカイブのハッシュが sources.json と違う`);
    }
    await writeFile(file, buf);
}

async function main() {
    const { values } = parseArgs({
        options: {
            out: { type: 'string', default: process.env.MUSLIB_PUBLIC_CACHE ?? path.join(root, '..', 'testsets', 'public-cache') },
        },
    });
    const cache = path.resolve(values.out);
    const manifest = JSON.parse(await readFile(path.join(root, 'test', 'public', 'sources.json'), 'utf8'));

    await mkdir(path.join(cache, 'archives'), { recursive: true });
    await mkdir(path.join(cache, 'pmd'), { recursive: true });

    let ok = 0, bad = 0;
    for (const src of manifest.sources) {
        const archive = path.join(cache, 'archives', `${src.source}-${path.basename(new URL(src.url).pathname)}`);
        try {
            await download(src, archive);
        } catch (e) {
            console.error(`error: ${e.message}`);
            bad += src.files.length;
            continue;
        }

        // アーカイブごとに空のディレクトリへ展開し、必要な pmd だけを取り出す
        const tmp = await mkdtemp(path.join(cache, 'extract-'));
        try {
            await run('7z', ['x', '-y', `-o${tmp}`, archive]);
            for (const f of src.files) {
                const from = path.resolve(tmp, f.path);
                const to = path.join(cache, 'pmd', path.basename(f.path));
                const buf = from.startsWith(tmp + path.sep) ? await readIfExists(from) : null;
                if (!buf || sha256(buf) !== f.sha256) {
                    console.error(`error: ${src.url}: ${f.path} が見つからないかハッシュが違う`);
                    bad++;
                    continue;
                }
                await copyFile(from, to);
                ok++;
            }
        } catch (e) {
            console.error(`error: ${archive}: ${e.message}`);
            bad += src.files.length;
        } finally {
            await rm(tmp, { recursive: true, force: true });
        }
    }

    console.log(`${ok} pmd -> ${path.join(cache, 'pmd')}${bad ? `, ${bad} failed` : ''}`);
    if (bad) process.exitCode = 1;
}

await main();
