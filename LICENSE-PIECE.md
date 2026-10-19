# P/ECE SDK 由来のソースについて

この repo の `src/`、`legacy/muslib/` 以下のファイル (`legacy/shim.h` と `legacy/render.c` は除く) は、
P/ECE 標準音楽ライブラリ muslib のソースを元に改変したものです。
これらのファイルには MIT License (`LICENSE`) は適用されません。

```
Copyright (C)2001 AQUAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
Coded by MIO.H (OeRSTED)
```

## 再配布の条件

アクアプラスによる「付属ソースを元にしたプログラムの再配布の扱いについて」
(<https://aquaplus.jp/piece/intr.html> より引用):

> P/ECEは、カーネル、ライブラリ、アプリケーションのソースファイルを公開しています。
> P/ECE付属のセットアップCD、WEB等による配布での差分等によって得られる、ソースを元に、改造したプログラムを発表・配布・販売する事に関しての扱いをお知らせします。
>
> これらのソースに関しては、アクアプラスが著作権を所有していますが、 P/ECE用のプログラムである限り、このソースを元にプログラムを改良して自由に再配布する事が可能です。
> WEBやCD−ROM等のメディアを使って発表する際に、アクアプラスの許可を得る必要はありません。
>
> 但し、配布する際には、元にしたソースファイル名が明確にドキュメントに記述されている必要があります。
> また、その改変されたプログラムにより、使用したユーザの P/ECE並びにPCに不具合が発生した場合の責任をアクアプラスに求めないように告知されていなければなりません。
>
> また、アクアプラスは、これらの変更を加えたプログラムの内容並び、適用した結果について、一切の責任を負うものではありません。

## 元にしたソースファイル

P/ECE SDK の以下のファイルを元にしています (パスは SDK のインストール先 `/usr/piece/` からの相対)。
auPiECE.kpi (Yui N. / S. Kino. / Kobarin による改変版) を経由しています。

| P/ECE SDK | この repo |
|---|---|
| `sysdev/music/mus.c` | `src/mus.c`, `legacy/muslib/mus.c` |
| `sysdev/music/seq.c` | `src/seq.c`, `legacy/muslib/seq.c` |
| `sysdev/music/musdef.h` | `src/musdef.h`, `legacy/muslib/musdef.h` |
| `sysdev/music/muslib.h` | `legacy/muslib/muslib.h` |
| `sysdev/music/exp12.c` | `src/exp12.c`, `legacy/muslib/exp12.c` |
| `sysdev/music/instdef.c` | `src/instdef.c`, `legacy/muslib/instdef.c` |
| `sysdev/music/wavetable.c` | `src/wavetable.c`, `legacy/muslib/wavetable.c` |
| `sysdev/music/wave/i_*.c` (ドラム音色) | `src/wave/i_*.c`, `legacy/muslib/wave/i_*.c` |
| `include/piece.h` (サウンド関係の定義の一部) | `legacy/muslib/musdefwin.h` |

## 免責

このプログラムは P/ECE 標準音楽ライブラリ muslib のソースを改変したものを含みます。
これらの改変についてアクアプラスはなんら関係なく、プログラムの内容およびその実行結果について、
アクアプラスは一切の責任を負いません。このプログラムを使用したことによってユーザーの
P/ECE ならびに PC に不具合が発生しても、その責任をアクアプラスに求めないでください。
このプログラムに関する一切の事柄について、アクアプラスにサポートを求めてはいけません。
