from __future__ import annotations

import math

import pytest

from femtowad.art import Screen, bar_picture, box_weights, gamma_corrected, resample
from femtowad.palette import Color
from femtowad.picture import Picture, Pixels

GREYS = [Color(i, i, i) for i in range(256)]


def test_screen() -> None:
    assert Screen.parse("320X200") == Screen(320, 200)
    assert [Screen(240, 160).bar_height, Screen(320, 200).bar_height] == [32, 32]
    assert [Screen(320, 240).bar_height, Screen(480, 320).bar_height] == [38, 51]
    assert Screen(240, 160).bar_scale == (0.75, 1)


@pytest.mark.parametrize(("source", "target"), [(4, 4), (320, 240), (32, 38), (6, 5)])
def test_box_weights_cover_the_source(source: int, target: int) -> None:
    weights = box_weights(source, target)
    assert len(weights) == target
    for cells in weights:
        assert math.isclose(sum(w for w, _ in cells), source / target)
    covered = [0.0] * source
    for cells in weights:
        for w, s in cells:
            covered[s] += w
    assert all(math.isclose(c, 1) for c in covered)


def test_resample_same_size() -> None:
    pixels: Pixels = [[0, 255, None], [17, None, 200]]
    assert resample(pixels, 3, 2, GREYS) == pixels


def test_resample_transparency() -> None:
    assert resample([[None, None], [None, 9]], 1, 1, GREYS) == [[None]]
    assert resample([[None, 9], [None, 9]], 1, 1, GREYS) == [[None]]
    assert resample([[None, 9], [9, 9]], 1, 1, GREYS) == [[9]]


def test_resample_mean() -> None:
    assert resample([[0, 100]], 1, 1, GREYS) == [[70]]


def test_bar_picture() -> None:
    face = Picture.from_pixels([[7] * 24] * 30, left=-4, top=-2).encode()
    digit = Picture.from_pixels([[7] * 4] * 6).encode()
    assert bar_picture(face, Screen(320, 200), GREYS) == face
    assert bar_picture(digit, Screen(240, 160), GREYS) == digit
    small = Picture.parse(bar_picture(face, Screen(240, 160), GREYS))
    assert (small.width, small.height, small.left, small.top) == (18, 30, -3, -2)
    large = Picture.parse(bar_picture(digit, Screen(480, 320), GREYS))
    assert (large.width, large.height) == (6, 9)


def test_gamma() -> None:
    playpal = bytes(range(256))
    for level in range(1, 6):
        brighter = gamma_corrected(playpal, level)
        assert brighter[0] == 0 and brighter[255] == 255
        assert all(b >= a for a, b in zip(playpal, brighter, strict=True))
