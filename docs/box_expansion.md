# ボックス拡張（28箱）に関する注意

このフォークではPCボックスを14箱（420匹）から28箱（840匹）に拡張している。その都合でセーブデータの形式がバニラ／上流と異なるため、以下の注意点を前提として扱うこと。

## セーブ形式の概要

- 進捗（SaveBlock2 + SaveBlock1）は従来通り2スロット交互保存。物理セクタ 0〜4（スロット1）と 14〜18（スロット2）を使う。
- ボックス（PokemonStorage）は、両スロットのストレージ領域である物理セクタ 5〜13 と 19〜27 を、1つの共有領域として使う（18セクタ）。
- 論理セクタIDは 0〜22（進捗 0〜4、ストレージ 5〜22）。進捗セクタはスロット内で回転するが、ストレージセクタは固定対応。物理セクタへの変換は `src/save.c` の `GetPhysicalSector()` と `sStoragePhysicalSectors` を参照。

## 注意点

1. **旧セーブと互換性がない。** 14箱時代のセーブデータは読み込めず、「セーブデータが壊れています」扱いになる（中途半端に読み込まれることはない）。逆に、28箱形式でセーブしたデータを14箱形式のビルドで読むこともできない。
2. **ボックスにバックアップがない。** 進捗には予備スロットがあるが、ボックスは共有の1か所だけである。ストレージ書き込み中の電源断ちなどでボックス領域が壊れると復旧できず、セーブ全体が破損扱いになる。
3. **進捗とボックスの時点がずれることがある。** セーブ失敗で進捗が旧スロットに巻き戻った場合も、ボックスは新しい時点のままになる。その間にボックスへ出し入れしたポケモンが増殖・消失し得る。SaveBlock3 の一部（図鑑ナビの検索レベル等）もストレージ側のセクタに乗るため、同様に新しい時点のままになる。
4. **セーブに時間がかかる。** 1回のセーブで書くセクタが14個から23個に増えている。
5. **EWRAMにほぼ空きがない。** ボックス拡張分（約34KB）で256KBがほぼ満杯（空き約72バイト）であり、EWRAMに静的データを追加するとリンクエラー（region `EWRAM` overflowed）になる。空きを作るため `SAVEBLOCK_MOVE_RANGE` を 128 から 64 に下げてある（`include/load_save.h` と `src/load_save.c` の両方で定義。128に戻すと溢れる）。
6. **レイアウト表のオフセットは32ビット幅。** `src/save.c` の `sSaveSlotLayout` の `offset` は `u32`（ストレージ後半のオフセットが64KiBを超えるため）。`u16` に戻すとオーバーフローでビルドエラーになる。

## 実装箇所

- `include/pokemon_storage_system.h`: `TOTAL_BOXES_COUNT` = 28
- `include/save.h`: `SECTOR_ID_PKMN_STORAGE_END` = 22、`NUM_PROGRESS_SECTORS` / `NUM_STORAGE_SECTORS` / `NUM_LOGICAL_SECTORS`
- `src/save.c`: `sStoragePhysicalSectors`、`GetPhysicalSector()`、`CheckSectorsValid()`、`CopySaveSlotData` / `GetSaveValidStatus` の「進捗スロット＋共有ストレージ」の2段構成、LinkFullSave のセクタ数
- `test/save.c`: `T_POKEMONSTORAGE_SIZE` = 67884

## 元の形式に戻す場合

コミット dd9bb1742（本体）と、ビルド修正の 218e04a54 / b7a768fcb を revert すれば14箱形式に戻る。
