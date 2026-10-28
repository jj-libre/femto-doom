# SPDX-License-Identifier: GPL-2.0-or-later
from __future__ import annotations

from femtowad.cli import find_engine
from femtowad.sourcegen import write_sources


def test_generated_sources_are_current() -> None:
    engine = find_engine()
    assert engine is not None
    assert write_sources(engine, check=True)
