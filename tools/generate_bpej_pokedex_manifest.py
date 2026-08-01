#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


SUPPORTED_REPRESENTATIONS = {
    "none": "raw",
    "gba_lz77": "gba_lz77_stream",
}
OUTPUT_ROOT = Path("build/extracted_pokedex")
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the BPEJ Pokedex extraction manifest from the canonical JSONL DB."
    )
    parser.add_argument("--symbols", required=True, help="Canonical Pokedex symbols JSONL")
    parser.add_argument("--output", required=True, help="Generated manifest path")
    return parser.parse_args()


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


def parse_int(value: int | str) -> int:
    return value if isinstance(value, int) else int(value, 0)


def validate_output_path(value: str, symbol_id: str) -> None:
    path = Path(value)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError(f"{symbol_id}: unsafe output path {value}")
    try:
        path.relative_to(OUTPUT_ROOT)
    except ValueError as error:
        raise ValueError(f"{symbol_id}: output must be under {OUTPUT_ROOT}") from error


def make_asset(entry: dict) -> dict:
    symbol_id = entry["id"]
    location = entry.get("location", {})
    verification = entry.get("verification", {})
    attrs = entry.get("attrs", {})
    extraction = attrs.get("extraction", {})

    if entry.get("status") != "verified" or verification.get("status") != "verified":
        raise ValueError(f"{symbol_id}: only verified symbols may be extracted")
    if verification.get("byte_match") is not True:
        raise ValueError(f"{symbol_id}: Expansion byte match is not verified")
    if location.get("size_status") != "known":
        raise ValueError(f"{symbol_id}: ROM size is not known")

    offset = location.get("rom_offset")
    size = location.get("size")
    compression = attrs.get("compression")
    if offset is None or size is None:
        raise ValueError(f"{symbol_id}: ROM offset or size is missing")
    if compression not in SUPPORTED_REPRESENTATIONS:
        raise ValueError(f"{symbol_id}: unsupported compression {compression!r}")

    representation = extraction.get("output_representation")
    expected_representation = SUPPORTED_REPRESENTATIONS[compression]
    if representation != expected_representation:
        raise ValueError(
            f"{symbol_id}: {compression} requires output_representation={expected_representation}"
        )

    required = ("label", "output_path", "consumer", "fallback_file")
    missing = [key for key in required if not extraction.get(key)]
    if missing:
        raise ValueError(f"{symbol_id}: extraction fields missing: {', '.join(missing)}")
    validate_output_path(extraction["output_path"], symbol_id)
    if not Path(extraction["fallback_file"]).is_file():
        raise ValueError(f"{symbol_id}: fallback asset is missing: {extraction['fallback_file']}")

    expected_hash = verification.get("sha1")
    if not isinstance(expected_hash, str) or not SHA1_RE.fullmatch(expected_hash):
        raise ValueError(f"{symbol_id}: verified SHA1 is missing or invalid")

    return {
        "symbol_id": symbol_id,
        "label": extraction["label"],
        "rom_offset": offset,
        "size": parse_int(size),
        "compression": compression,
        "output_representation": representation,
        "output_path": extraction["output_path"],
        "expected_hash": expected_hash,
        "status": entry["status"],
        "relations": entry.get("relations", []),
        "consumer": extraction["consumer"],
        "fallback_file": extraction["fallback_file"],
        "verification_method": verification.get("method"),
    }


def validate_assets(assets: list[dict]) -> None:
    labels: set[str] = set()
    outputs: set[str] = set()
    ranges: list[tuple[int, int, str]] = []
    for asset in assets:
        if asset["label"] in labels:
            raise ValueError(f"duplicate extraction label {asset['label']}")
        if asset["output_path"] in outputs:
            raise ValueError(f"duplicate extraction output {asset['output_path']}")
        labels.add(asset["label"])
        outputs.add(asset["output_path"])
        start = parse_int(asset["rom_offset"])
        end = start + asset["size"]
        ranges.append((start, end, asset["symbol_id"]))

    ranges.sort()
    for previous, current in zip(ranges, ranges[1:]):
        if current[0] < previous[1]:
            raise ValueError(f"overlapping ROM ranges: {previous[2]} and {current[2]}")


def main() -> int:
    args = parse_args()
    symbols_path = Path(args.symbols)
    output_path = Path(args.output)
    rows = load_jsonl(symbols_path)

    baserom_rows = [entry for entry in rows if entry.get("type") == "pokedex.baserom"]
    if len(baserom_rows) != 1:
        raise ValueError("canonical DB must contain exactly one pokedex.baserom row")
    baserom_attrs = baserom_rows[0].get("attrs", {})

    selected = [
        entry for entry in rows if entry.get("attrs", {}).get("extraction", {}).get("enabled", True)
        and "extraction" in entry.get("attrs", {})
    ]
    assets = [make_asset(entry) for entry in selected]
    assets.sort(key=lambda asset: parse_int(asset["rom_offset"]))
    validate_assets(assets)
    if not assets:
        raise ValueError("canonical DB contains no enabled extraction entries")

    source_hash = hashlib.sha1(symbols_path.read_bytes()).hexdigest()
    manifest = {
        "format_version": 1,
        "baserom": {
            "file": "baserom.gba",
            "gameCode": baserom_attrs["gameCode"],
            "size": baserom_attrs["size"],
            "sha1": baserom_attrs["sha1"],
            "crc32": baserom_attrs["crc32"],
        },
        "source": {
            "path": symbols_path.as_posix(),
            "sha1": source_hash,
        },
        "asset_count": len(assets),
        "assets": assets,
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary_path = output_path.with_suffix(output_path.suffix + ".tmp")
    temporary_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    temporary_path.replace(output_path)
    print(f"Generated {output_path} with {len(assets)} verified Pokedex assets.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
