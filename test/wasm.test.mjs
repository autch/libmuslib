// SPDX-License-Identifier: MIT
//
// wasm/muslib.mjs (と muslib.wasm の公開 API) のテスト。
//   node --test test/wasm.test.mjs
// wasm は環境変数 MUSLIB_WASM (既定は build-wasm/muslib.wasm)。
// 出力そのものの正しさは golden で見るので、ここでは API の振る舞いを確かめる。

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { Muslib } from '../wasm/muslib.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const module = await WebAssembly.compile(readFileSync(process.env.MUSLIB_WASM ?? path.join(root, 'build-wasm', 'muslib.wasm')));
const fixture = (name) => readFileSync(path.join(root, 'test', 'fixtures', `${name}.pmd`));
const sjis = new TextDecoder('shift_jis');

function renderAll(m, maxBlocks) {
    const out = [];
    const buf = new Int16Array(Muslib.BLOCK_SAMPLES);
    while (out.length < maxBlocks && m.render(buf)) out.push(buf.slice());
    return out;
}

test('タイトルを生バイト列で返す', () => {
    const m = Muslib.fromModule(module);
    m.init(48000);
    assert.ok(m.play(fixture('ys2_item')));
    assert.equal(sjis.decode(m.title), '[Ys II] 宝箱');
    assert.equal(sjis.decode(m.title2), '(c) Falcom, arr: Autch');
    assert.equal(m.numParts, 6);
    m.dispose();
});

test('演奏が終わると render が false を返し finished になる', () => {
    const m = Muslib.fromModule(module);
    m.init(16000);
    m.play(fixture('ys2_pause'));
    const blocks = renderAll(m, 100000);
    assert.ok(blocks.length > 0 && blocks.length < 100000);
    assert.ok(m.finished);
    assert.equal(m.render(new Int16Array(Muslib.BLOCK_SAMPLES)), false);
    m.dispose();
});

test('先頭の 2 ブロックは無音 (元の muslib の出力キューの挙動)', () => {
    const m = Muslib.fromModule(module);
    m.init(44100);
    m.play(fixture('ys1_op'));
    const blocks = renderAll(m, 200);
    assert.ok(blocks[0].every((v) => v === 0));
    assert.ok(blocks[1].every((v) => v === 0));
    assert.ok(blocks.some((b) => b.some((v) => v !== 0)));
    m.dispose();
});

test('stop() で止まる', () => {
    const m = Muslib.fromModule(module);
    m.init(44100);
    m.play(fixture('ys1_op'));
    renderAll(m, 10);
    assert.equal(m.finished, false);
    m.stop();
    assert.ok(m.finished);
    assert.equal(m.render(new Int16Array(Muslib.BLOCK_SAMPLES)), false);
    m.dispose();
});

test('同じインスタンスで曲を替えても前の曲の状態を持ち越さない', () => {
    const m = Muslib.fromModule(module);
    m.init(44100);
    m.play(fixture('herzog_03'));
    const first = renderAll(m, 500);
    m.play(fixture('ys3_07'));
    renderAll(m, 500);
    m.play(fixture('herzog_03'));
    assert.deepEqual(renderAll(m, 500), first);
    m.dispose();
});

test('複数のインスタンスは互いに影響しない', () => {
    const solo = (name) => {
        const m = Muslib.fromModule(module);
        m.init(48000);
        m.play(fixture(name));
        const r = renderAll(m, 300);
        m.dispose();
        return r;
    };
    const a = Muslib.fromModule(module), b = Muslib.fromModule(module);
    a.init(48000);
    b.init(48000);
    a.play(fixture('valna_08'));
    b.play(fixture('ys2_open'));
    const ra = [], rb = [];
    const buf = new Int16Array(Muslib.BLOCK_SAMPLES);
    for (let i = 0; i < 300; i++) {
        a.render(buf);
        ra.push(buf.slice());
        b.render(buf);
        rb.push(buf.slice());
    }
    assert.deepEqual(ra, solo('valna_08'));
    assert.deepEqual(rb, solo('ys2_open'));
    a.dispose();
    b.dispose();
});

test('不正な入力', () => {
    const m = Muslib.fromModule(module);
    assert.equal(m.play(fixture('ys2_item')), false, 'init 前');
    assert.equal(m.init(0), 0);
    assert.equal(m.init(192000), 96000, 'レートは 96000 で頭打ち');
    assert.equal(m.init(44100), 44100);
    assert.equal(m.play(new Uint8Array(0)), false, '空');
    assert.equal(m.play(new Uint8Array(0x10001)), false, '64KB 超');
    assert.equal(m.play(new Uint8Array(0x10000)), true, 'ちょうど 64KB');
    m.dispose();
});

test('Muslib.load() でも作れる', async () => {
    const m = await Muslib.load(module);
    m.init(44100);
    assert.ok(m.play(fixture('ys2_item')));
    m.dispose();
});
