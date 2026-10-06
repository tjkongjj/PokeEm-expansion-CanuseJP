#!/usr/bin/env python3

"""Build the Frontier Move overlay for Legends Z-A: Mega Dimension."""

from __future__ import annotations

import argparse
from collections import defaultdict
import json
from pathlib import Path
import re
import subprocess
import sys
import unicodedata


ZA_DATA_PATH = "tools/learnset_helpers/porymoves_files/za.json"
ZA_DLC_COMMIT = "3e273221ff97d1752e0c4cd073dd88e2f47b151a"
ZA_BASE_REF = f"{ZA_DLC_COMMIT}^:{ZA_DATA_PATH}"
LEARN_METHODS = ("LevelMoves", "TMMoves", "TutorMoves", "EggMoves")

SPECIES_ALIASES = {
    "DEOXYS": "SPECIES_DEOXYS_NORMAL",
    "FLAB\u00c3\u00a9B\u00c3\u00a9": "SPECIES_FLABEBE",
    "HOOPA": "SPECIES_HOOPA_CONFINED",
    "INDEEDEE": "SPECIES_INDEEDEE_M",
    "MEOWSTIC": "SPECIES_MEOWSTIC_M",
    "SHAYMIN": "SPECIES_SHAYMIN_LAND",
    "TOXTRICITY": "SPECIES_TOXTRICITY_AMPED",
}


def normalize_identifier(value: str) -> str:
    value = unicodedata.normalize("NFKD", value)
    value = "".join(char for char in value if not unicodedata.combining(char))
    return re.sub(r"[^A-Z0-9]", "", value.upper())


def read_constants(path: Path, prefix: str) -> set[str]:
    return set(re.findall(rf"\b{prefix}[A-Z0-9_]+\b", path.read_text(encoding="utf-8")))


def read_git_json(root: Path, ref: str) -> dict:
    result = subprocess.run(
        ["git", "show", ref],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    return json.loads(result.stdout)


def collect_moves(document: dict) -> dict[str, set[str]]:
    result = {}
    for species, learnset in document.items():
        moves = set()
        for method in LEARN_METHODS:
            entries = learnset.get(method, [])
            if method == "LevelMoves":
                moves.update(entry["Move"] for entry in entries)
            else:
                moves.update(entries)
        result[species] = moves
    return result


def parse_frontier_learnsets(path: Path, table_name: str) -> dict[str, set[str]]:
    source = path.read_text(encoding="utf-8")
    pattern = re.compile(
        rf"static const u16 {table_name}_(SPECIES_[A-Z0-9_]+)\[\]\s*=\s*\{{(.*?)\}};",
        re.S,
    )
    result = {}
    for species, body in pattern.findall(source):
        moves = set(re.findall(r"\bMOVE_[A-Z0-9_]+\b", body))
        moves.discard("MOVE_UNAVAILABLE")
        result[species] = moves
    if not result:
        raise ValueError(f"No learnsets found in {path}")
    return result


def make_species_resolver(species_constants: set[str]):
    by_identifier = {
        normalize_identifier(species.removeprefix("SPECIES_")): species
        for species in species_constants
    }

    def resolve(source_species: str) -> str | None:
        alias = SPECIES_ALIASES.get(source_species)
        if alias in species_constants:
            return alias
        return by_identifier.get(normalize_identifier(source_species))

    return resolve


def build_overlay(root: Path, base_document: dict, current_document: dict):
    species_constants = read_constants(root / "include/constants/species.h", "SPECIES_")
    move_constants = read_constants(root / "include/constants/moves.h", "MOVE_")
    full = parse_frontier_learnsets(
        root / "src/data/pokemon/frontier_full_learnsets.h",
        "sFrontierFullLearnset",
    )
    events = parse_frontier_learnsets(
        root / "src/data/pokemon/frontier_event_learnsets.h",
        "sFrontierEventLearnset",
    )
    resolve_species = make_species_resolver(species_constants)
    base_moves = collect_moves(base_document)
    current_moves = collect_moves(current_document)

    overlay = defaultdict(set)
    ignored_species = set()
    ignored_moves = set()
    added_pairs = 0
    for source_species, moves in current_moves.items():
        additions = moves - base_moves.get(source_species, set())
        added_pairs += len(additions)
        if not additions:
            continue

        species = resolve_species(source_species)
        if species is None:
            ignored_species.add(source_species)
            continue

        supported = {move for move in additions if move in move_constants}
        ignored_moves.update(additions - supported)
        known = full.get(species, set()) | events.get(species, set())
        overlay[species].update(supported - known)

    overlay = {
        species: sorted(moves)
        for species, moves in sorted(overlay.items())
        if moves
    }
    return overlay, sorted(ignored_species), sorted(ignored_moves), added_pairs


def make_document(overlay, ignored_species, ignored_moves, added_pairs):
    return {
        "metadata": {
            "description": "Pokemon Legends Z-A: Mega Dimension additions for Frontier Move",
            "source": f"https://github.com/rh-hideout/pokeemerald-expansion/commit/{ZA_DLC_COMMIT}",
            "base_source": f"{ZA_DLC_COMMIT}^:{ZA_DATA_PATH}",
            "current_source": ZA_DATA_PATH,
            "source_added_pairs": added_pairs,
            "included_species": len(overlay),
            "included_pairs": sum(map(len, overlay.values())),
            "ignored_unmapped_source_species": ignored_species,
            "ignored_unsupported_moves": ignored_moves,
        },
        "species": overlay,
    }


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--base", type=Path, help="Pre-DLC za.json (defaults to the upstream parent commit)")
    parser.add_argument("--current", type=Path, help="Current za.json")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    output = args.output or root / "src/data/pokemon/frontier_za_dlc_moves.json"
    current_path = args.current or root / ZA_DATA_PATH

    if args.base:
        base_document = json.loads(args.base.read_text(encoding="utf-8"))
    else:
        base_document = read_git_json(root, ZA_BASE_REF)
    current_document = json.loads(current_path.read_text(encoding="utf-8"))

    overlay, ignored_species, ignored_moves, added_pairs = build_overlay(
        root,
        base_document,
        current_document,
    )
    document = make_document(overlay, ignored_species, ignored_moves, added_pairs)
    content = json.dumps(document, ensure_ascii=False, indent=2) + "\n"

    if args.check:
        if not output.exists() or output.read_text(encoding="utf-8") != content:
            print(f"{output} is out of date", file=sys.stderr)
            return 1
        print(f"{output} is up to date")
        return 0

    output.write_text(content, encoding="utf-8")
    print(
        f"Wrote {len(overlay)} species and {sum(map(len, overlay.values()))} "
        f"moves to {output}"
    )
    if ignored_species:
        names = ", ".join(ignored_species).encode("ascii", "backslashreplace").decode("ascii")
        print(f"Ignored {len(ignored_species)} unmapped species: {names}")
    if ignored_moves:
        print(f"Ignored {len(ignored_moves)} unsupported moves: {', '.join(ignored_moves)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
