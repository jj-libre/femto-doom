"""Builds femto-doom's WAD pair, DOOM64KL.WAD (graphics) and DOOM64ML.WAD (maps), from a Doom IWAD.

The maps keep the IWAD's own nodes. The layout follows jWadUtil by Frenkel Smeijers, which made
the WADs of Doom64KB, the engine femto-doom grew from.

usage: uv run --project tools/femtowad femtowad <doom1.wad> <out-dir> [--texshift N] [--high-detail]
"""

from __future__ import annotations

import argparse
from pathlib import Path

from .art import generate_art
from .engine import Engine
from .graphics import build_graphics, texture_names
from .maps import build_maps
from .wad import Wad


def find_engine() -> Path | None:
    """The femto-doom checkout this tool lives in."""
    return next((d for d in Path(__file__).resolve().parents if (d / "info.c").is_file()), None)


def main() -> None:
    parser = argparse.ArgumentParser(prog="femtowad", description=__doc__.split("\n\n")[0])
    parser.add_argument("iwad", type=Path, help="Doom IWAD, such as the shareware doom1.wad")
    parser.add_argument("out", type=Path, help="directory for DOOM64KL.WAD and DOOM64ML.WAD")
    parser.add_argument("--texshift", type=int, default=0, help="the engine's TEXSHIFT")
    parser.add_argument("--high-detail", action="store_true", help="the engine is built with HIGH_DETAIL")
    parser.add_argument("--engine", type=Path, default=find_engine(), help="the femto-doom checkout the WADs are for")
    args = parser.parse_args()
    if args.engine is None:
        parser.error("no femto-doom checkout found, pass --engine")

    engine = Engine.read(args.engine)
    iwad = Wad.read(args.iwad)
    graphics = build_graphics(iwad, generate_art(iwad), engine, args.texshift, args.high_detail)
    maps = build_maps(iwad, texture_names(iwad), engine)
    args.out.mkdir(parents=True, exist_ok=True)
    for name, wad in (("DOOM64KL.WAD", graphics), ("DOOM64ML.WAD", maps)):
        data = wad.to_bytes()
        (args.out / name).write_bytes(data)
        print(f"{args.out / name}: {len(data)} B, {len(wad)} lumps")
