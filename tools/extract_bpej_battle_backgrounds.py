#!/usr/bin/env python3
"""Extract the verified BPEJ battle-background PoC assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import zlib
from pathlib import Path


OUTPUT_PREFIX = ("build", "extracted_battle_backgrounds")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baserom", help="Path to baserom.gba")
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--out", default="build/extracted_battle_backgrounds")
    parser.add_argument("--stamp")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--print-outputs", action="store_true")
    mode.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def fail(message: str) -> int:
    print(f"error: {message}", file=sys.stderr)
    return 1


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def parse_int(value: int | str) -> int:
    return value if isinstance(value, int) else int(value, 0)


def load_manifest(path: Path) -> dict:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    assets = manifest.get("assets", [])
    if manifest.get("asset_count") != len(assets) or len(assets) != 9:
        raise ValueError("manifest must contain exactly 9 PoC assets")
    if manifest.get("set_count") != 3 or len({asset["set"] for asset in assets}) != 3:
        raise ValueError("manifest must contain exactly 3 PoC sets")
    return manifest


def resolve_output(out_dir: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute() or ".." in path.parts or path.parts[:2] != OUTPUT_PREFIX:
        raise ValueError(f"unsafe output path {value}")
    return out_dir / Path(*path.parts[2:])


def validate_baserom(data: bytes, manifest: dict, path: Path) -> None:
    expected = manifest["baserom"]
    game_code = data[0xAC:0xB0].decode("ascii", errors="replace") if len(data) >= 0xB0 else ""
    actual_crc32 = f"{zlib.crc32(data) & 0xFFFFFFFF:08x}"
    if (
        len(data) != parse_int(expected["size"])
        or game_code != expected["gameCode"]
        or sha1(data) != expected["sha1"]
        or actual_crc32 != expected["crc32"]
    ):
        raise ValueError(f"{path} is not the expected BPEJ ROM")


def decompress_lz77(data: bytes, symbol_id: str) -> tuple[bytes, int]:
    if len(data) < 4 or data[0] != 0x10:
        raise ValueError(f"{symbol_id} is not a GBA LZ77 stream")
    output_size = int.from_bytes(data[1:4], "little")
    source_pos = 4
    output = bytearray()
    while len(output) < output_size:
        if source_pos >= len(data):
            raise ValueError(f"{symbol_id} GBA LZ77 stream is truncated")
        flags = data[source_pos]
        source_pos += 1
        for bit in range(7, -1, -1):
            if len(output) >= output_size:
                break
            if flags & (1 << bit):
                if source_pos + 2 > len(data):
                    raise ValueError(f"{symbol_id} GBA LZ77 back-reference is truncated")
                first, second = data[source_pos], data[source_pos + 1]
                source_pos += 2
                length = (first >> 4) + 3
                distance = (((first & 0x0F) << 8) | second) + 1
                if distance > len(output):
                    raise ValueError(f"{symbol_id} has an invalid GBA LZ77 distance")
                for _ in range(length):
                    output.append(output[-distance])
                    if len(output) >= output_size:
                        break
            else:
                if source_pos >= len(data):
                    raise ValueError(f"{symbol_id} GBA LZ77 literal is truncated")
                output.append(data[source_pos])
                source_pos += 1
    padding = data[source_pos:]
    if len(padding) > 3 or any(padding):
        raise ValueError(f"{symbol_id} has non-alignment data after its GBA LZ77 stream")
    return bytes(output), source_pos


def validate_representation(data: bytes, asset: dict) -> None:
    if asset["output_representation"] == "raw":
        if len(data) != parse_int(asset["decompressed_size"]):
            raise ValueError(f"{asset['symbol_id']} raw output size mismatch")
        return
    if asset["output_representation"] == "gba_lz77_stream":
        output, _ = decompress_lz77(data, asset["symbol_id"])
        if len(output) != parse_int(asset["decompressed_size"]):
            raise ValueError(f"{asset['symbol_id']} decompressed size mismatch")
        return
    raise ValueError(f"{asset['symbol_id']} has unsupported output representation")


def rom_output(rom: bytes, asset: dict) -> bytes:
    offset = parse_int(asset["rom_offset"])
    size = parse_int(asset["size"])
    source = rom[offset : offset + size]
    if len(source) != size or sha1(source) != asset["expected_source_hash"]:
        raise ValueError(f"{asset['symbol_id']} source range/hash mismatch")
    decompressed, _ = decompress_lz77(source, asset["symbol_id"])
    if len(decompressed) != parse_int(asset["decompressed_size"]):
        raise ValueError(f"{asset['symbol_id']} decompressed size mismatch")
    output = source if asset["transform"] == "copy" else decompressed
    if sha1(output) != asset["expected_hash"]:
        raise ValueError(f"{asset['symbol_id']} output hash mismatch")
    validate_representation(output, asset)
    return output


def desired_output(rom: bytes | None, asset: dict) -> tuple[bytes, str]:
    custom_path = Path(asset["custom_path"])
    if custom_path.is_file():
        data = custom_path.read_bytes()
        validate_representation(data, asset)
        return data, "custom"
    if rom is None:
        raise ValueError(f"{asset['symbol_id']} needs baserom.gba because no custom override exists")
    return rom_output(rom, asset), "baserom"


def output_state(assets: list[dict], outputs: list[Path]) -> tuple[list[str], str, int]:
    failures: list[str] = []
    state_lines: list[str] = []
    custom_count = 0
    for asset, output_path in zip(assets, outputs):
        custom_path = Path(asset["custom_path"])
        origin = "custom" if custom_path.is_file() else "baserom"
        if origin == "custom":
            custom_count += 1
            desired = custom_path.read_bytes()
            try:
                validate_representation(desired, asset)
            except ValueError as error:
                failures.append(f"{custom_path.as_posix()}: {error}")
                continue
            expected_hash = sha1(desired)
        else:
            expected_hash = asset["expected_hash"]
        if not output_path.is_file():
            failures.append(f"{output_path.as_posix()}: missing")
            continue
        actual = output_path.read_bytes()
        try:
            validate_representation(actual, asset)
        except ValueError as error:
            failures.append(f"{output_path.as_posix()}: {error}")
            continue
        if sha1(actual) != expected_hash:
            failures.append(f"{output_path.as_posix()}: hash mismatch for {origin} source")
            continue
        state_lines.append(f"{output_path.as_posix()}={origin}:{sha1(actual)}")
    digest = sha1(("\n".join(state_lines) + "\n").encode("utf-8"))
    return failures, digest, custom_count


def write_stamp(path: Path, manifest_path: Path, count: int, state_hash: str, custom_count: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    content = "\n".join(
        [
            "verified=1",
            f"manifest={manifest_path.as_posix()}",
            f"manifest_sha1={sha1(manifest_path.read_bytes())}",
            f"file_count={count}",
            f"custom_count={custom_count}",
            f"output_state_sha1={state_hash}",
            "",
        ]
    )
    if path.is_file() and path.read_text(encoding="utf-8") == content:
        return
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(content, encoding="utf-8", newline="\n")
    temporary.replace(path)


def write_bytes_if_changed(path: Path, data: bytes) -> None:
    if path.is_file() and path.read_bytes() == data:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_bytes(data)
    temporary.replace(path)


def validate_stamp(path: Path, manifest_path: Path, count: int, state_hash: str, custom_count: int) -> str | None:
    if not path.is_file():
        return "stamp is missing"
    values = dict(line.split("=", 1) for line in path.read_text(encoding="utf-8").splitlines() if "=" in line)
    expected = {
        "verified": "1",
        "manifest_sha1": sha1(manifest_path.read_bytes()),
        "file_count": str(count),
        "custom_count": str(custom_count),
        "output_state_sha1": state_hash,
    }
    for key, value in expected.items():
        if values.get(key) != value:
            return f"stamp {key} mismatch"
    return None


def main() -> int:
    args = parse_args()
    manifest_path = Path(args.manifest)
    out_dir = Path(args.out)
    try:
        manifest = load_manifest(manifest_path)
        assets = manifest["assets"]
        outputs = [resolve_output(out_dir, asset["output_path"]) for asset in assets]
    except Exception as error:
        return fail(f"could not load manifest: {error}")

    if args.print_outputs:
        for path in outputs:
            print(path.as_posix())
        return 0

    if args.check:
        failures, state_hash, custom_count = output_state(assets, outputs)
        if args.stamp:
            stamp_error = validate_stamp(Path(args.stamp), manifest_path, len(outputs), state_hash, custom_count)
            if stamp_error:
                failures.append(f"{args.stamp}: {stamp_error}")
        if failures:
            return fail("output validation failed:\n  " + "\n  ".join(failures))
        print(f"Validated {len(outputs)} outputs ({custom_count} custom overrides).")
        return 0

    if not args.baserom or not Path(args.baserom).is_file():
        return fail("place a legally obtained BPEJ ROM at baserom.gba")
    try:
        rom_path = Path(args.baserom)
        rom = rom_path.read_bytes()
        validate_baserom(rom, manifest, rom_path)
        resolved = [desired_output(rom, asset) for asset in assets]
        if args.dry_run:
            for asset, path, (_, origin) in zip(assets, outputs, resolved):
                print(f"{asset['label']} [{origin}]: {asset['rom_offset']} + 0x{asset['size']:X} -> {path.as_posix()}")
            return 0
        for path, (data, _) in zip(outputs, resolved):
            write_bytes_if_changed(path, data)
        failures, state_hash, custom_count = output_state(assets, outputs)
        if failures:
            raise ValueError("; ".join(failures))
        if args.stamp:
            write_stamp(Path(args.stamp), manifest_path, len(outputs), state_hash, custom_count)
    except Exception as error:
        return fail(f"battle-background extraction failed: {error}")
    print(f"Extracted {len(outputs)} BPEJ battle-background assets ({custom_count} custom overrides).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
