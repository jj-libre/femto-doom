"""The engine sources femtowad writes, and checking that they are current."""

from __future__ import annotations

from collections.abc import Callable
from pathlib import Path

from .cheader import licence
from .infogen import check_references, info_c, info_h
from .viewgen import r_view_c, r_view_h

VIEW_LICENCE = licence("Tangents and, per supported view width, the view angle tables.", "viewgen.py")

SOURCES: dict[str, Callable[[], str]] = {
    "info.h": info_h,
    "info.c": info_c,
    "r_view.h": lambda: r_view_h(VIEW_LICENCE),
    "r_view.c": lambda: r_view_c(VIEW_LICENCE),
}


def write_sources(engine: Path, check: bool) -> bool:
    """Writes the generated files into src/ of the femto-doom checkout at `engine`, or with `check`
    only compares them; whether they were all current."""
    check_references()
    current = True
    for name, generate in SOURCES.items():
        text = generate()
        path = engine / "src" / name
        old = path.read_bytes().decode("latin-1").replace("\r\n", "\n") if path.exists() else None
        if old == text:
            continue
        current = False
        if check:
            print(f"{path} is not what femtowad generates")
        else:
            path.write_bytes(text.encode("latin-1"))
            print(f"{path} written")
    return current
