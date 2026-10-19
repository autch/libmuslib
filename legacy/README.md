# legacy/ — golden 生成用の凍結版 muslib

`muslib/` は [auPiECE.kpi](https://github.com/autch/auPiECE.kpi) の `extended-channels` ブランチ
(コミット `758d66c`、MAXCH = 26) の `muslib/` をコピーしたもの。文字コード (Shift_JIS) も含めてそのまま置いてある。
`shim.h` / `render.c` は Windows / KbMedia Player 依存部分 (`windows.h`、`MulDiv`、
`pceWaveDataOut`、`winlayer.cpp` のレート設定と `Render()`) を Linux 上で再現するためのもの。

このディレクトリは `test/golden.json` を作るためだけにあり、今後は変更しない。
変更すると golden の意味が変わるので、変える場合は golden を作り直して理由を記録すること。

## 元コードからの改変

- `muslib/wave/i_{BD909,HC909,SD909,SDGATE,HO909,TOML1,HANDCLAP}.c`:
  波形配列の末尾に `0` を 1 つ追加した。
  これらは配列長がちょうど `loop_end >> 14` で、`MakeWaveNL()` の線形補間が `tbl[o+1]` で
  配列の 1 つ先を読む。その値はリンク時の配置で決まるため、ビルドごとに出力が変わってしまう。
  どの配列も最後の値が 0 なので、0 を足すと「末尾の値を保持する」(旧 JS 版の結果) と同じになる。
- `tools/` (MML 変換ツール)、`wave/mkiw.c`、CMakeLists.txt は不要なので含めていない。

## Windows 版との違い

- `shim.h` で `long` を `int` に置き換え、LLP64 (long = 32bit) と同じ整数幅にしている。
- `-fwrapv` で符号付き整数のオーバーフローをラップアラウンドとして定義している (MSVC の実際の挙動と同じ)。
- pmd バッファは 64KB をゼロ埋めして確保している (kpi は `fsize + 1` を未初期化で確保していた)。
- `Render()` は 1 ブロック (128 サンプル) ずつ呼んでいる。kpi は 32 ブロック単位で呼ぶため、
  演奏が終わったときに最後のブロックを繰り返して出していた。この繰り返しは golden に含めない。

## 出力の性質

`PlayMusic()` の時点でゼロのブロックを 1 つ出力キューに入れ、最初の `BuffFree()` で
シーケンサを動かす前に 3 ブロック先まで書き込む。このため先頭に無音ブロックが 2 つ入り、
旧 JS 版より 1 ブロック遅れて同じ波形になる (16kHz で確認済み)。
