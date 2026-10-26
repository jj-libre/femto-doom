"""femto-doom's data tool.

`wads` builds DOOM64KL.WAD (graphics) and DOOM64ML.WAD (maps) from a Doom IWAD. The maps keep the
IWAD's own nodes. The layout follows jWadUtil by Frenkel Smeijers, which made the WADs of
Doom64KB, the engine femto-doom grew from.

`info` writes the engine's info.c and info.h from info.py, the one source of thing types, their
states and sprites, which `wads` uses too.

usage: uv run --project tools/femtowad femtowad wads <doom1.wad> <out-dir> [--texshift N] [--high-detail]
       uv run --project tools/femtowad femtowad info [--check]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .art import generate_art
from .graphics import build_graphics, texture_names
from .infogen import write_info
from .maps import build_maps
from .wad import Wad


def find_engine() -> Path | None:
    """The femto-doom checkout this tool lives in."""
    return next((d for d in Path(__file__).resolve().parents if (d / "src" / "info.c").is_file()), None)


def wads(args: argparse.Namespace) -> None:
    iwad = Wad.read(args.iwad)
    graphics = build_graphics(iwad, generate_art(iwad), args.texshift, args.high_detail)
    maps = build_maps(iwad, texture_names(iwad))
    args.out.mkdir(parents=True, exist_ok=True)
    for name, wad in (("DOOM64KL.WAD", graphics), ("DOOM64ML.WAD", maps)):
        data = wad.to_bytes()
        (args.out / name).write_bytes(data)
        print(f"{args.out / name}: {len(data)} B, {len(wad)} lumps")


def info(args: argparse.Namespace) -> None:
    if args.engine is None:
        sys.exit("no femto-doom checkout found, pass --engine")
    if not write_info(args.engine, args.check) and args.check:
        sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser(prog="femtowad", description=__doc__.split("\n\n")[0])
    commands = parser.add_subparsers(required=True)

    wads_parser = commands.add_parser("wads", help="build the WAD pair from an IWAD")
    wads_parser.add_argument("iwad", type=Path, help="Doom IWAD, such as the shareware doom1.wad")
    wads_parser.add_argument("out", type=Path, help="directory for DOOM64KL.WAD and DOOM64ML.WAD")
    wads_parser.add_argument("--texshift", type=int, default=0, help="the engine's TEXSHIFT")
    wads_parser.add_argument("--high-detail", action="store_true", help="the engine is built with HIGH_DETAIL")
    wads_parser.set_defaults(run=wads)

    info_parser = commands.add_parser("info", help="write info.c and info.h from info.py")
    info_parser.add_argument("--check", action="store_true", help="only fail if they differ from info.py")
    info_parser.add_argument("--engine", type=Path, default=find_engine(), help="the femto-doom checkout")
    info_parser.set_defaults(run=info)

    args = parser.parse_args()
    args.run(args)
