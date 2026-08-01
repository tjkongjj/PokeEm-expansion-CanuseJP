#!/usr/bin/env python3
"""Generate the BPEJ battle-background extraction manifest from JSONL."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


OUTPUT_ROOT = Path("build/extracted_battle_backgrounds")
CUSTOM_ROOT = Path("graphics/custom/bpej/battle_backgrounds")
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
TRANSFORMS = {
    "copy": "gba_lz77_stream",
    "gba_lz77_decompress": "raw",
}


def parse_int(value: int | str) -> int:
    return value if isinstance(value, int) else int(value, 0)


def load_jsonl(path: Path) -> list[dict]:
    rows: list[dict] = []
    ids: set[str] = set()
    with path.open("r", encoding="utf-8") as source:
        for line_number, line in enumerate(source, 1):
            if not line.strip():
                continue
            entry = json.loads(line)
            symbol_id = entry.get("id")
            if not symbol_id:
                raise ValueError(f"{path}:{line_number}: symbol id is missing")
            if symbol_id in ids:
                raise ValueError(f"{path}:{line_number}: duplicate symbol id {symbol_id}")
            ids.add(symbol_id)
            rows.append(entry)
    return rows


def validate_relative_path(value: str, root: Path, symbol_id: str) -> None:
    path = Path(value)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError(f"{symbol_id}: unsafe path {value}")
    try:
        path.relative_to(root)
    except ValueError as error:
        raise ValueError(f"{symbol_id}: {value} must be under {root.as_posix()}") from error


def make_asset(entry: dict) -> dict:
    symbol_id = entry["id"]
    location = entry.get("location", {})
    verification = entry.get("verification", {})
    attrs = entry.get("attrs", {})
    extraction = attrs.get("extraction", {})

    if entry.get("status") != "verified" or verification.get("status") != "verified":
        raise ValueError(f"{symbol_id}: only verified symbols may be extracted")
    if verification.get("byte_match") is not True:
        raise ValueError(f"{symbol_id}: decompressed Expansion byte match is not verified")
    if location.get("size_status") != "known":
        raise ValueError(f"{symbol_id}: ROM size is not known")
    if attrs.get("compression") != "gba_lz77":
        raise ValueError(f"{symbol_id}: only verified GBA LZ77 assets are supported")

    required = (
        "set",
        "label",
        "consumer",
        "output_path",
        "custom_path",
        "fallback_file",
        "transform",
        "output_representation",
        "expected_output_sha1",
    )
    missing = [key for key in required if not extraction.get(key)]
    if missing:
        raise ValueError(f"{symbol_id}: extraction fields missing: {', '.join(missing)}")
    validate_relative_path(extraction["output_path"], OUTPUT_ROOT, symbol_id)
    validate_relative_path(extraction["custom_path"], CUSTOM_ROOT, symbol_id)
    if not Path(extraction["fallback_file"]).is_file():
        raise ValueError(f"{symbol_id}: retained fallback asset is missing: {extraction['fallback_file']}")

    transform = extraction["transform"]
    if transform not in TRANSFORMS:
        raise ValueError(f"{symbol_id}: unsupported transform {transform!r}")
    if extraction["output_representation"] != TRANSFORMS[transform]:
        raise ValueError(f"{symbol_id}: transform/representation mismatch")
    source_hash = verification.get("sha1")
    output_hash = extraction["expected_output_sha1"]
    if not isinstance(source_hash, str) or not SHA1_RE.fullmatch(source_hash):
        raise ValueError(f"{symbol_id}: source SHA1 is missing or invalid")
    if not isinstance(output_hash, str) or not SHA1_RE.fullmatch(output_hash):
        raise ValueError(f"{symbol_id}: output SHA1 is missing or invalid")

    return {
        "symbol_id": symbol_id,
        "set": extraction["set"],
        "label": extraction["label"],
        "rom_offset": location["rom_offset"],
        "size": parse_int(location["size"]),
        "compression": attrs["compression"],
        "transform": transform,
        "output_representation": extraction["output_representation"],
        "output_path": extraction["output_path"],
        "custom_path": extraction["custom_path"],
        "expected_source_hash": source_hash,
        "expected_hash": output_hash,
        "decompressed_size": attrs["decompressed_size"],
        "status": entry["status"],
        "relations": entry.get("relations", []),
        "consumer": extraction["consumer"],
        "fallback_file": extraction["fallback_file"],
        "verification_method": verification.get("method"),
    }


def validate_assets(assets: list[dict]) -> None:
    for field in ("label", "output_path", "consumer"):
        values = [asset[field] for asset in assets]
        if len(values) != len(set(values)):
            raise ValueError(f"duplicate extraction {field}")
    ranges = sorted(
        (parse_int(asset["rom_offset"]), parse_int(asset["rom_offset"]) + asset["size"], asset["symbol_id"])
        for asset in assets
    )
    for previous, current in zip(ranges, ranges[1:]):
        if current[0] < previous[1]:
            raise ValueError(f"overlapping extraction ranges: {previous[2]} and {current[2]}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--symbols", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    symbols_path = Path(args.symbols)
    rows = load_jsonl(symbols_path)
    baserom_rows = [entry for entry in rows if entry.get("type") == "battle_backgrounds.baserom"]
    if len(baserom_rows) != 1:
        raise ValueError("canonical DB must contain exactly one battle_backgrounds.baserom row")
    baserom = baserom_rows[0]["attrs"]
    selected = [
        entry
        for entry in rows
        if entry.get("attrs", {}).get("extraction", {}).get("enabled") is True
    ]
    assets = sorted((make_asset(entry) for entry in selected), key=lambda asset: parse_int(asset["rom_offset"]))
    validate_assets(assets)
    if len(assets) != 9 or len({asset["set"] for asset in assets}) != 3:
        raise ValueError("the initial PoC must remain exactly 9 assets across 3 background sets")

    manifest = {
        "format_version": 1,
        "baserom": {
            "file": "baserom.gba",
            "gameCode": baserom["gameCode"],
            "size": baserom["size"],
            "sha1": baserom["sha1"],
            "crc32": baserom["crc32"],
        },
        "source": {
            "path": symbols_path.as_posix(),
            "sha1": hashlib.sha1(symbols_path.read_bytes()).hexdigest(),
        },
        "asset_count": len(assets),
        "set_count": len({asset["set"] for asset in assets}),
        "assets": assets,
    }
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    temporary.replace(output)
    print(f"Generated {output} with {len(assets)} assets across {manifest['set_count']} sets.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
