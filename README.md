# libmuslib

P/ECE 標準音楽ドライバ muslib を、ネイティブと WebAssembly の両方で使える C ライブラリとして整理し直すプロジェクト。

## 状態

1. [x] 旧 C 版 (auPiECE.kpi) を Linux で動かし、golden PCM を生成する — `legacy/`, `tools/golden.mjs`
2. [x] グローバル変数を構造体にまとめ、`muldiv` を導入する。ネイティブ版で golden と一致させる — `include/`, `src/`, `cli/`
3. [ ] wasm ビルド
4. [ ] (別途) auPiECE.kpi をこのライブラリに移行する

## 構成

- `include/muslib.h` — 公開 API (`muslib_init` / `muslib_play` / `muslib_render` …)。1 インスタンスが 1 曲分の状態を持ち、グローバル変数は使わない
- `src/` — エンジン本体 (muslib 由来)
- `cli/muslib-render.c` — pmd を raw PCM に書き出すツール。golden の照合にも使う
- `legacy/` — golden 生成専用の凍結版
- `tools/golden.mjs` — golden の生成と照合

### 旧 C 版からの挙動の変更 (正しいデータでは出力は変わらない)

- 存在しない音色番号は無視し、直前の音色のまま演奏を続ける (旧版はクラッシュ)
- リピートのスタックが空のときの `SeqBreak` / `SeqNext` は何もしない (旧版は直前の構造体メンバを読み書きしていた)。
  スタックがあふれる場合はそれ以上積まない
- シーケンスデータはオフセットで扱い、16bit で折り返すので 64KB のバッファの外を読まない
- ドラム波形の末尾に番兵の 0 を置く (`legacy/README.md` 参照)
- 最大パート数を 26 から 32 にした (mucc-next の上限)

## ビルド

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## golden

条件: 16000 / 44100 / 48000 Hz、最大 10000 ブロック (1 ブロック = 128 サンプル)。
比較対象は int16le モノラルのストリーム。

「テストセット」は 1 つのディレクトリで、次の 2 つのファイルを置く。

- `golden.json` — 各 pmd の SHA-256、出力全体の SHA-256、500 ブロックごとのハッシュ、ブロック数、演奏が終了したかどうか
- `excluded.json` (任意) — 壊れていて基準にできない pmd と、その理由。
  未定義動作が途中から始まるだけなら、その手前の秒数で打ち切ることもできる

テストセットは 2 種類ある。

- `test/` — repo 同梱のテストセット。pmd は `test/fixtures/` にある (作者の了承済み、入手元は同ディレクトリの README)
- `test/public/` — ネットで入手できる pmd (48 曲) のテストセット。再配布できないので pmd は含めず、
  入手元 (`sources.json`) と golden だけを置く。`tools/fetch-public.mjs` で入手元から取得する
- 手元のコレクション — 再配布できない pmd を含むので、テストセットもファイル名を含めて repo の外に置く

```sh
node tools/fetch-public.mjs                      # 公開テストセットの pmd を ../testsets/public-cache に取得 (要 7z)
cmake -S . -B build && cmake --build build
ctest --test-dir build                           # test/ と、取得済みなら test/public/ で照合
```

手元のテストセットを使う場合:

```sh
export MUSLIB_TESTSET=../testsets/private PMD_DIR=/path/to/pmd   # cmake の configure 前に設定すると ctest にも入る
# 生成 (通常は不要。legacy/ を変えたときだけ)
node tools/golden.mjs make  --cli build/legacy/muslib-legacy-render
# 照合。--ref を付けると、ずれ始めたブロックとサンプルの位置まで表示する
node tools/golden.mjs check --cli build/muslib-render --ref build/legacy/muslib-legacy-render
```

## ライセンス

この repo には、ライセンスが異なる 2 種類のファイルがある。

- **このプロジェクトで書いたファイル** (`SPDX-License-Identifier: MIT` と書いてあるもの): MIT License (`LICENSE`)
- **P/ECE muslib に由来するファイル** (`src/` と `legacy/muslib/` 以下): アクアプラスの再配布条件に従う。
  元にしたソースファイル名、再配布条件の原文、免責事項は `LICENSE-PIECE.md` を参照
