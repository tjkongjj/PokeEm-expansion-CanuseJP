# BPEJ Battle Background Asset Research

## Scope

This inventory covers the main battle background selected after battle setup: background tiles, palette, tilemap, the terrain selector, special battle overrides, and the RAM terrain selector used to index the ROM table. It does not register Pokemon or trainer sprites, move animations, the battle UI, or start-transition payloads.

HP bars, status icons, party-ball indicators, text windows, and other battle UI remain external references owned by `symbols/bpej/ui/symbols.jsonl`. The ROM Atlas and broad graphics envelope remain owned by `symbols/bpej/rom_map/regions.jsonl` and `symbols/bpej/graphics_inventory/regions.jsonl`.

## Result

`symbols/bpej/battle_backgrounds/symbols.jsonl` contains 48 records:

| Status | Count | Meaning in this DB |
| --- | ---: | --- |
| verified | 46 | 45 physical graphics assets plus the selector table |
| partial | 0 | None |
| candidate | 0 | None |
| metadata | 2 | BPEJ ROM identity and the EWRAM terrain selector |

All 45 physical assets have a direct BPEJ pointer, a valid GBA LZ77 stream, a known aligned storage size, and an exact decompressed-byte match against the corresponding current Expansion asset. The selector table is independently verified from its code-indexed 10 x 20-byte structure. No later-generation or Expansion-added background is registered as BPEJ data.

## Selection Model

`gUnknown_82EBA00` at ROM offset `0x002EBA00` is the BPEJ `gBattleTerrainTable` equivalent. It has ten records of five pointers:

1. main background tiles
2. main background tilemap
3. entry-animation tiles
4. entry-animation tilemap
5. main background palette

`DrawMainBattleBackground` and `LoadChosenBattleElement` in `asm/battle_bg.s` index each 20-byte record with the EWRAM byte at `0x02022C94`. `BattleSetup_GetTerrainId` in `asm/battle_setup.s` derives that terrain from metatile behavior, map type, surfing state, Route 113, and weather. `GetCurrentMapBattleScene` and battle flags can override the normal terrain choice.

The two entry-animation pointers are preserved in the selector record's `attrs.entries`. Their payloads are intentionally not split into symbols or extracted because start transitions are outside this task.

## Normal Terrains

| ID | Terrain | Main asset offsets (tiles / palette / tilemap) | Selection summary |
| ---: | --- | --- | --- |
| 0 | tall_grass | `0x00D77E0C` / `0x00D783BC` / `0x00D783F4` | Tall-grass metatile |
| 1 | long_grass | `0x00D786A4` / `0x00D78D1C` / `0x00D78D5C` | Long-grass metatile |
| 2 | sand | `0x00D7900C` / `0x00D79604` / `0x00D7964C` | Sand/deep sand, Route 113, or ash weather |
| 3 | underwater | `0x00D798FC` / `0x00D79EB4` / `0x00D79EFC` | Underwater map type |
| 4 | water | `0x00D7A1AC` / `0x00D7A780` / `0x00D7A7C4` | Deep/ocean water or bridge-water surfing |
| 5 | pond_water | `0x00D7AA74` / `0x00D7B01C` / `0x00D7B05C` | Other surfable water |
| 6 | rock | `0x00D7B30C` / `0x00D7B8CC` / `0x00D7B908` | Mountain metatile |
| 7 | cave | `0x00D7BBB8` / `0x00D7C1B0` / `0x00D7C1F8` | Underground map without building/pond override |
| 8 | building | `0x00D7C4E4` / `0x00D7DF58` / `0x00D7CACC` | Indoor, secret base, or underground building |
| 9 | plain | shared Building tiles / `0x00D7C4A8` / shared Building tilemap | Normal fallback |

The tile payloads expand to `0x2000` bytes, tilemaps to `0x1000` bytes, and palettes to `0x60` bytes. JSONL `location.size` is the ROM storage size, including at most three zero alignment bytes; `attrs.compressed_payload_size` records the parsed LZ77 payload length.

## Special Backgrounds

| Family | Physical assets | Conditions and sharing |
| --- | --- | --- |
| Frontier/link | palette `0x00D7CA90` | Shares Building tiles/tilemap; selected for Frontier, link, e-Reader trainer, recorded/special link, and Frontier map scene paths |
| Gym | palette `0x00D7E02C` | Shares Building tiles/tilemap; `MAP_BATTLE_SCENE_GYM` |
| Leader | palette `0x00D7E068` | Shares Building tiles/tilemap; leader trainer class |
| Stadium | tiles `0x00D7CD7C`, tilemap `0x00D7D384` | Shared by Aqua, Magma, Elite Four, and Champion palette variants |
| Aqua / Magma | palettes `0x00D7E0A4`, `0x00D7E104` | Map battle scene overrides |
| Sidney / Phoebe / Glacia / Drake | palettes `0x00D7E170`, `0x00D7E1C0`, `0x00D7E208`, `0x00D7E24C` | Elite Four map battle scenes |
| Champion Wallace | palette `0x00D7E29C` | Shares Stadium tiles/tilemap; champion trainer class |
| Kyogre | palette `0x00D7DF98` | Shares Water tiles/tilemap; Kyogre battle flag |
| Groudon | palette `0x00D7DFD4` | Shares Cave tiles/tilemap; Groudon battle flag |
| Rayquaza | tiles `0x00D7D634`, tilemap `0x00D7DCA8`, palette `0x00D7E2D8` | Dedicated set; Rayquaza battle flag |

Physical sharing is represented by relations rather than duplicate asset rows. In particular, Building and Plain share graphics, the Frontier/Gym/Leader variants share Building graphics, Stadium palettes share one tiles/tilemap pair, and Kyogre/Groudon reuse Water/Cave graphics.

## Verification Evidence

The research tool validates the BPEJ SHA1 `d7cf8f156ba9c455d164e1ea780a6bf1945465c2` and CRC32 `4881f3f8`, parses all LZ77 streams, and compares decompressed bytes with `pokeemerald-expansion/graphics/battle_environment` build products. The result is written locally to `build/bpej_battle_backgrounds/research_report.json`.

Primary code evidence:

* `asm/battle_bg.s`: direct tiles, tilemap, palette, selector-table, VRAM destination, and palette-size references
* `asm/battle_setup.s`: `BattleSetup_GetTerrainId`
* `asm/overworld.s`: `GetCurrentMapBattleScene`
* `data/data.s`: selector and broad physical storage boundaries
* Expansion `src/battle_bg.c`, `src/data/battle_environment.h`, and `src/data/graphics/battle_environment.h`: established names and use conditions

The current Expansion files are comparison references only. Expansion-added environments such as Soaring, Sky Pillar, Burial Ground, Puddle, Marsh, Swamp, Snow, Ice, Volcano, Distortion World, Space, Ultra Space, and custom environments are excluded.

## Extraction PoC

The PoC is deliberately fixed at three verified sets and nine outputs:

| Set | Reason | Outputs |
| --- | --- | --- |
| tall_grass | Normal outdoor battle | tiles, palette, tilemap |
| cave | Indoor/underground battle | tiles, palette, tilemap |
| champion_wallace | Special trainer battle | shared Stadium tiles/tilemap and Wallace palette |

Tiles and tilemaps are copied as original BPEJ GBA LZ77 streams. Palettes are decompressed to raw `.gbapal`, because the current Expansion consumer calls `LoadPalette` rather than `LoadCompressedPalette`.

The manifest is generated from JSONL by `tools/generate_bpej_battle_background_manifest.py`; it is not maintained by hand. `tools/extract_bpej_battle_backgrounds.py` supports `--dry-run`, `--print-outputs`, and `--check`. Generated files live under `build/extracted_battle_backgrounds/`, and the stamp is `build/generated/bpej_battle_backgrounds_extracted.ok`.

Optional binary overrides under `graphics/custom/bpej/battle_backgrounds/<set>/` take precedence over ROM extraction. They must use the same delivery format as the generated output: `.4bpp.lz`, `.bin.lz`, and raw `.gbapal`. `make clean` removes the resolved build outputs and stamp, but not files under `graphics/custom/`.

The retained PNG/PAL/BIN sources under `graphics/battle_environment/` are not deleted. Only the nine PoC symbol definitions are redirected to generated outputs; all other backgrounds continue to use the existing asset path.

## Audit

The deterministic report currently records:

* duplicate IDs: none
* duplicate names: none
* duplicate ROM offsets: none
* duplicate GBA addresses: none
* range overlaps: none
* missing relation targets: none
* `.sym` address-bearing rows: 46
* extraction outputs: 9

The selector table is not split into address-bearing child records, so its first child does not duplicate the parent's address. Shared graphics are referenced rather than registered a second time.

## Build Validation

Validation on 2026-07-17 used the WSL `CodexUbuntu` ARM toolchain from a `make clean` state:

* manifest generation: 9 assets across 3 sets
* `--dry-run`: success
* `--print-outputs`: 9 paths
* extraction and stamp generation: success, 0 custom overrides
* `--check`: success
* `make clean`: removed `build/extracted_battle_backgrounds/` and `build/generated/bpej_battle_backgrounds_extracted.ok`
* `make -j4 --output-sync=target`: success from the clean state in 377.9 seconds
* generated ROM: `pokeemerald.gba`, 32 MiB
* incremental make: success; unchanged background outputs did not update `graphics.o`

The successful clean build is the clone-like baserom-only pipeline check: no extracted background output or stamp existed before the build. Existing source assets remained in place. No emulator battle was launched during this research pass, so the requested wild/cave/Champion runtime display check remains explicitly unverified; the PoC has exact decompressed graphics, tilemap, and palette bytes and a successful ROM link, but those facts do not substitute for visual runtime observation.

## Remaining Work

* Start-transition/entry-animation payloads are named only by terrain-table position and remain outside scope.
* Runtime display should still be checked in an emulator for a wild tall-grass battle, a cave battle, and the Champion Wallace battle. Byte and palette equality establish the asset content but do not replace runtime observation.
* The next low-risk extraction candidates are the remaining seven normal terrain sets, followed by Building/Plain and the palette-only Gym/Leader/Frontier variants.
