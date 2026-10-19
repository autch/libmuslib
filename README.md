# libmuslib

P/ECE 標準音楽ドライバ muslib を、ネイティブと WebAssembly の両方で使える C ライブラリとして整理し直すプロジェクト。

## 状態

1. [x] 旧 C 版 (auPiECE.kpi) を Linux で動かし、golden PCM を生成する — `legacy/`, `test/golden.json`
2. [ ] グローバル変数を構造体にまとめ、`muldiv` を導入する。ネイティブ版で golden と一致させる
3. [ ] wasm ビルド
4. [ ] (別途) auPiECE.kpi をこのライブラリに移行する

## ビルド

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## golden

条件: 16000 / 44100 / 48000 Hz、最大 10000 ブロック (1 ブロック = 128 サンプル)。
比較対象は int16le モノラルのストリーム。
`test/golden.json` には pmd ファイルそのものは入れず、各 pmd の SHA-256、出力全体の SHA-256、
500 ブロックごとのハッシュ、ブロック数、演奏が終了したかどうかを記録している。
pmd は再配布できないものを含むので repo には置かず、ディレクトリを指定して使う。

```sh
# 生成 (通常は不要。legacy/ を変えたときだけ)
node tools/golden.mjs make  --cli build/legacy/muslib-legacy-render --pmd-dir $PMD_DIR
# 照合。--ref を付けると、ずれ始めたブロックとサンプルの位置まで表示する
node tools/golden.mjs check --cli <renderer> --pmd-dir $PMD_DIR --ref build/legacy/muslib-legacy-render
```

壊れていて基準にできない pmd は `test/excluded.json` に理由を添えて列挙する。

## ライセンス

この repo には、ライセンスが異なる 2 種類のファイルがある。

- **このプロジェクトで書いたファイル** (`SPDX-License-Identifier: MIT` と書いてあるもの): MIT License (`LICENSE`)
- **P/ECE muslib に由来するファイル** (`legacy/muslib/` 以下と、今後それを元に作るエンジン本体):
  auPiECE.kpi と同じ扱いとする。

> このプログラムは，P/ECE 標準音楽ライブラリ muslib のソースにパッチを当てたものが使用されています。
> オリジナルの muslib ライブラリの著作権表示は以下のとおりです。
>
>     Copyright (C)2001 AQUAPLUS Co., Ltd. / OeRSTED, Inc.
>     all rights reserved.
>     Coded by MIO.H (OeRSTED)
>
> これらの改変についてアクアプラスはなんら関係なく，プログラムの内容およびそれの実行結果について，
> アクアプラスは一切の責任を負いません。このプログラムに関する一切の事柄についてアクアプラスに
> サポートを求めてはいけません。
>
> 私およびアクアプラスは，このプログラムを使用したこと，または使用しなかった，使用できなかった事に
> よって発生する，または発生する恐れのある一切の損害について，それらが発生することをあらかじめ
> 知っていたか，あるいは知ることができたとしても，私およびアクアプラスは何らの法的および道義的
> 責任を負いません。
>
> これら以外については，P/ECE デフォルトのフリーソフトの扱いに準じます。
