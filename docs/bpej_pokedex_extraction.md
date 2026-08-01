# BPEJ図鑑UIアセット抽出

この仕組みは、日本版ポケットモンスター エメラルド（BPEJ）のROMから、検証済みの図鑑UIアセットだけをビルド時に抽出します。ROMと抽出物はGit管理しません。図鑑アセットDBの正本は `symbols/bpej/pokedex/symbols.jsonl` です。

## 初回PoCの対象

初回PoCでは、BPEJ ROMと既存Expansionアセットのbyte一致を確認できた次の4件だけを抽出します。

| symbol id | ROM範囲 | 出力 |
| --- | --- | --- |
| `bpej:pokedex:regional_dex:palette:bg_hoenn_loaded` | `0x00537422` + `190` bytes | `build/extracted_pokedex/palettes/bg_hoenn_loaded.gbapal` |
| `bpej:pokedex:national_dex:palette:bg_national_loaded` | `0x005375A2` + `190` bytes | `build/extracted_pokedex/palettes/bg_national_loaded.gbapal` |
| `bpej:pokedex:list:gfx:caught_ball` | `0x00539C0E` + `64` bytes | `build/extracted_pokedex/sprites/caught_ball.4bpp` |
| `bpej:pokedex:detail:palette:size_silhouette` | `0x005430F0` + `32` bytes | `build/extracted_pokedex/palettes/size_silhouette.gbapal` |

Hoenn／全国図鑑paletteは、透明色を除いて実際にロードされる190 bytesがDB上の検証範囲です。既存の完全なpaletteファイルは比較・フォールバック用として残します。

一覧・詳細画面の共通tilesやtilemapは、現時点のDBでは `partial` またはbyte不一致です。search、sort、分布図、ポケモンスプライト、鳴き声、Expansion追加UIとともにPoC対象外です。不明な圧縮形式や値を推測して抽出対象へ加えてはいけません。

## 生成と抽出

プロジェクトルートへ正規のBPEJ ROMを `baserom.gba` として配置し、通常どおり`make`を実行します。ビルドは既存のBPEJ ROM検証を通した後、次を自動生成します。

- `build/generated/bpej_pokedex_manifest.json`
- `build/extracted_pokedex/` 以下の抽出物
- `build/generated/bpej_pokedex_extracted.ok`

manifestを手で編集せず、JSONLの検証情報と `attrs.extraction` から生成してください。

```sh
python3 tools/generate_bpej_pokedex_manifest.py \
  --symbols symbols/bpej/pokedex/symbols.jsonl \
  --output build/generated/bpej_pokedex_manifest.json

python3 tools/extract_bpej_pokedex.py \
  --baserom baserom.gba \
  --manifest build/generated/bpej_pokedex_manifest.json \
  --stamp build/generated/bpej_pokedex_extracted.ok
```

出力予定、書き込みなしの検証、抽出済みファイルの検証には、それぞれ `--print-outputs`、`--dry-run`、`--check` を使います。`make clean`は生成manifest、stamp、`build/extracted_pokedex/`を削除しますが、`graphics/pokedex/`の既存アセットは削除しません。

## ビルド経路

PoCはvanilla図鑑の `src/pokedex.c` だけを対象にします。`POKEDEX_PLUS_HGSS` のExpansion追加図鑑は従来アセットを使い、graphics全体のinclude pathは変更しません。対象4件以外も従来経路のままです。

新しい対象を追加する場合は、JSONLで `verified`、ROM offset、size、compression、出力先、SHA1、Expansion側とのbyte一致がすべて確定した項目だけに `attrs.extraction` を追加します。
