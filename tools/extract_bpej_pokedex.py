#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import zlib
from pathlib import Path


REQUIRED_MESSAGE = "日本版エメラルドROMをプロジェクトルートに baserom.gba として配置してください"
OUTPUT_PREFIX = ("build", "extracted_pokedex")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Extract verified BPEJ Pokedex UI assets.")
    parser.add_argument("--baserom", help="Path to baserom.gba")
    parser.add_argument("--manifest", required=True, help="Generated Pokedex manifest")
    parser.add_argument("--out", default="build/extracted_pokedex", help="Extraction root")
    parser.add_argument("--stamp", help="Stamp written after successful extraction")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Validate outputs and optional stamp")
    mode.add_argument("--print-outputs", action="store_true", help="Print output paths and exit")
    mode.add_argument("--dry-run", action="store_true", help="Validate ROM and assets without writing")
    return parser.parse_args()


def fail(message: str, paths: list[Path] | None = None) -> int:
    print(f"error: {message}", file=sys.stderr)
    if paths:
        for path in paths:
            print(f"  {path.as_posix()}", file=sys.stderr)
    return 1


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def load_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as source:
        manifest = json.load(source)
    if manifest.get("asset_count") != len(manifest.get("assets", [])):
        raise ValueError("manifest asset_count does not match assets")
    return manifest


def parse_int(value: int | str) -> int:
    return value if isinstance(value, int) else int(value, 0)


def resolve_output(out_dir: Path, manifest_path: str) -> Path:
    relative = Path(manifest_path)
    if relative.is_absolute() or ".." in relative.parts:
        raise ValueError(f"unsafe output path {manifest_path}")
    if relative.parts[:2] != OUTPUT_PREFIX:
        raise ValueError(f"output path must be under {'/'.join(OUTPUT_PREFIX)}: {manifest_path}")
    return out_dir / Path(*relative.parts[2:])


def expected_outputs(manifest: dict, out_dir: Path) -> list[Path]:
    return [resolve_output(out_dir, asset["output_path"]) for asset in manifest["assets"]]


def validate_baserom(data: bytes, manifest: dict, path: Path) -> None:
    expected = manifest["baserom"]
    if len(data) != parse_int(expected["size"]):
        raise ValueError(f"{path} has unexpected size 0x{len(data):X}")
    game_code = data[0xAC:0xB0].decode("ascii", errors="replace")
    actual_sha1 = sha1(data)
    actual_crc32 = f"{zlib.crc32(data) & 0xFFFFFFFF:08x}"
    if game_code != expected["gameCode"]:
        raise ValueError(f"{path} has game code {game_code!r}, expected {expected['gameCode']!r}")
    if actual_sha1 != expected["sha1"] or actual_crc32 != expected["crc32"]:
        raise ValueError(
            f"{path} is not the expected BPEJ ROM "
            f"(sha1={actual_sha1}, crc32={actual_crc32})"
        )


def slice_rom(rom: bytes, asset: dict) -> bytes:
    offset = parse_int(asset["rom_offset"])
    size = parse_int(asset["size"])
    end = offset + size
    if offset < 0 or size <= 0 or end > len(rom):
        raise ValueError(
            f"{asset['symbol_id']} range 0x{offset:X}+0x{size:X} is outside the ROM"
        )
    data = rom[offset:end]
    if sha1(data) != asset["expected_hash"]:
        raise ValueError(f"{asset['symbol_id']} hash mismatch at 0x{offset:X}")
    validate_representation(data, asset)
    return data


def gba_lz77_stream_length(data: bytes, symbol_id: str) -> int:
    if len(data) < 4 or data[0] != 0x10:
        raise ValueError(f"{symbol_id} is not a GBA LZ77 stream")
    output_size = int.from_bytes(data[1:4], "little")
    if output_size == 0:
        raise ValueError(f"{symbol_id} has an empty GBA LZ77 header")

    source_pos = 4
    output_pos = 0
    while output_pos < output_size:
        if source_pos >= len(data):
            raise ValueError(f"{symbol_id} GBA LZ77 stream is truncated")
        flags = data[source_pos]
        source_pos += 1
        for bit in range(7, -1, -1):
            if output_pos >= output_size:
                break
            if flags & (1 << bit):
                if source_pos + 2 > len(data):
                    raise ValueError(f"{symbol_id} GBA LZ77 back-reference is truncated")
                first = data[source_pos]
                second = data[source_pos + 1]
                source_pos += 2
                length = (first >> 4) + 3
                displacement = (((first & 0xF) << 8) | second) + 1
                if displacement > output_pos or output_pos + length > output_size:
                    raise ValueError(f"{symbol_id} has an invalid GBA LZ77 back-reference")
                output_pos += length
            else:
                if source_pos >= len(data):
                    raise ValueError(f"{symbol_id} GBA LZ77 literal is truncated")
                source_pos += 1
                output_pos += 1
    return source_pos


def validate_representation(data: bytes, asset: dict) -> None:
    compression = asset["compression"]
    representation = asset["output_representation"]
    if compression == "none" and representation == "raw":
        return
    if compression == "gba_lz77" and representation == "gba_lz77_stream":
        consumed = gba_lz77_stream_length(data, asset["symbol_id"])
        if consumed != len(data):
            raise ValueError(
                f"{asset['symbol_id']} GBA LZ77 size mismatch: parsed {consumed}, manifest {len(data)}"
            )
        return
    raise ValueError(
        f"{asset['symbol_id']} has unsupported compression/representation "
        f"{compression!r}/{representation!r}"
    )


def validate_output(path: Path, asset: dict) -> str | None:
    if not path.is_file():
        return "missing"
    data = path.read_bytes()
    if len(data) != parse_int(asset["size"]):
        return f"size mismatch ({len(data)} != {asset['size']})"
    if sha1(data) != asset["expected_hash"]:
        return "hash mismatch"
    try:
        validate_representation(data, asset)
    except ValueError as error:
        return str(error)
    return None


def validate_stamp(path: Path, manifest_path: Path, output_count: int) -> str | None:
    if not path.is_file():
        return "stamp is missing"
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    if values.get("verified") != "1":
        return "stamp is not verified"
    if values.get("manifest_sha1") != sha1(manifest_path.read_bytes()):
        return "stamp manifest hash mismatch"
    if values.get("file_count") != str(output_count):
        return "stamp output count mismatch"
    return None


def write_stamp(path: Path, manifest_path: Path, output_count: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    content = "\n".join(
        [
            "verified=1",
            f"manifest={manifest_path.as_posix()}",
            f"manifest_sha1={sha1(manifest_path.read_bytes())}",
            f"file_count={output_count}",
            "",
        ]
    )
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(content, encoding="utf-8", newline="\n")
    temporary.replace(path)


def main() -> int:
    args = parse_args()
    manifest_path = Path(args.manifest)
    out_dir = Path(args.out)
    try:
        manifest = load_manifest(manifest_path)
        outputs = expected_outputs(manifest, out_dir)
    except Exception as error:
        return fail(f"図鑑UI manifestの読込に失敗しました: {error}")

    if args.print_outputs:
        for path in outputs:
            print(path.as_posix())
        return 0

    if args.check:
        failures: list[str] = []
        for asset, path in zip(manifest["assets"], outputs):
            error = validate_output(path, asset)
            if error:
                failures.append(f"{path.as_posix()}: {error}")
        if args.stamp:
            stamp_error = validate_stamp(Path(args.stamp), manifest_path, len(outputs))
            if stamp_error:
                failures.append(f"{args.stamp}: {stamp_error}")
        if failures:
            print("error: 図鑑UI抽出物が不足または不正です。", file=sys.stderr)
            for failure in failures:
                print(f"  {failure}", file=sys.stderr)
            return 1
        return 0

    if not args.baserom or not Path(args.baserom).is_file():
        return fail(REQUIRED_MESSAGE, outputs)

    try:
        baserom_path = Path(args.baserom)
        rom = baserom_path.read_bytes()
        validate_baserom(rom, manifest, baserom_path)
        extracted = [slice_rom(rom, asset) for asset in manifest["assets"]]
        if args.dry_run:
            for asset, path in zip(manifest["assets"], outputs):
                print(
                    f"{asset['label']}: {asset['rom_offset']} + 0x{asset['size']:X} -> {path.as_posix()}"
                )
            return 0

        for data, path in zip(extracted, outputs):
            path.parent.mkdir(parents=True, exist_ok=True)
            temporary = path.with_suffix(path.suffix + ".tmp")
            temporary.write_bytes(data)
            temporary.replace(path)

        failures = [
            f"{path.as_posix()}: {error}"
            for asset, path in zip(manifest["assets"], outputs)
            if (error := validate_output(path, asset))
        ]
        if failures:
            raise ValueError("; ".join(failures))
        if args.stamp:
            write_stamp(Path(args.stamp), manifest_path, len(outputs))
    except Exception as error:
        return fail(f"図鑑UIアセット抽出に失敗しました: {error}")

    print(f"Extracted {len(outputs)} BPEJ Pokedex UI assets to {out_dir}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
