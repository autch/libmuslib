# test/public — 公開テストセット

ネットで入手できる P/ECE 用の曲データで作ったテストセット。
曲データは再配布できないので、この repo には含めない。入手元と、各ファイルの SHA-256、golden だけを置いている。

| 入手元 | ページ | pmd 数 | アーカイブ |
|---|---|---|---|
| P/ECE 公式 フリー音楽素材 (AQUAPLUS) | <https://aquaplus.jp/piece/dl/musdata.html> | 9 | musdata.exe |
| Firecracker P/ECE Music Data collection (CHEMOOL) | <https://sound.jp/chemool/piecelib.html> | 12 | fcpmdc02.zip, fcpmdc03.zip |
| ORPHE | <http://www.orphe.net/piece.html> | 17 | 17 個の lzh |
| P.K.O. (zurachu) | <https://github.com/zurachu/pko> | 10 | commit 7b20328 の zip |

## 使い方

```sh
node tools/fetch-public.mjs     # 入手元から取得して ../testsets/public-cache/pmd に置く (要 7z)
node tools/golden.mjs check --set test/public --pmd-dir ../testsets/public-cache/pmd --cli build/muslib-render
```

- `sources.json` — 入手元の URL、アーカイブと中の pmd の SHA-256。取得した pmd はこのハッシュで照合する
- `golden.json` — `legacy/` の旧 C 版で作った golden

アクアプラスのデータは自己解凍形式 (`musdata.exe`) で配布されているが、実行はせず 7z で中身だけを取り出す。
