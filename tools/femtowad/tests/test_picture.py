# SPDX-License-Identifier: GPL-2.0-or-later
from __future__ import annotations

import pytest

from femtowad.picture import MAX_HEIGHT, Picture, Pixels

PIXELS: Pixels = [
    [1, None, 5],
    [2, None, 5],
    [None, None, 5],
    [3, 4, 5],
]


def test_pixels_round_trip() -> None:
    picture = Picture.from_pixels(PIXELS, left=-2, top=7)
    assert picture.pixels() == PIXELS
    parsed = Picture.parse(picture.encode())
    assert parsed == picture
    assert (parsed.left, parsed.top) == (-2, 7)


def test_posts() -> None:
    columns = Picture.from_pixels(PIXELS).columns
    assert columns[0] == bytes((0, 2, 1, 1, 2, 2, 3, 1, 3, 3, 3, 0xFF))
    assert columns[1] == bytes((3, 1, 4, 4, 4, 0xFF))


def test_equal_columns_stored_once() -> None:
    once = Picture.from_pixels([[1, 2]]).encode()
    twice = Picture.from_pixels([[1, 1]]).encode()
    assert len(twice) == len(once) - len(Picture.from_pixels([[1]]).columns[0])


def test_keep_every() -> None:
    picture = Picture.from_pixels([[1, 2, 3, 4, 5]]).keep_every(2)
    assert picture.pixels() == [[1, 1, 3, 3, 5]]


def test_too_high() -> None:
    with pytest.raises(ValueError):
        Picture.from_pixels([[1]] * (MAX_HEIGHT + 1))
