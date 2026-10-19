# test/fixtures

[autch/piece-pmds](https://github.com/autch/piece-pmds) の MML (`*.pmml`) を
[mucc-next](https://github.com/autch/mucc-next) でコンパイルした pmd。
作者の了承を得て収録している。

| 項目 | 値 |
|---|---|
| piece-pmds | `d56f08d` |
| mucc-next | `d474be5` |

`lz3_fairies_s.pmml` は改行コードが CRLF で、mucc-next (`d474be5`) がそのままでは読めない
(全行が "Unknown line format" になる) ため、LF に変換してからコンパイルした。

これらの golden は `test/golden.json` にある (生成は `legacy/` の旧 C 版)。
