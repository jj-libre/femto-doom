# femtowad

femto-doom's data tool. Builds the engine's two WADs from a Doom IWAD and writes its generated
sources. Needs [uv](https://docs.astral.sh/uv/).

## WADs

```bash
uv tool install ./tools/femtowad
femtowad wads doom1.wad wads/
```

| Option | Default | Must match the engine's |
|---|---|---|
| `--screen WxH` | 240x160 | `SCREEN_WIDTH`, `SCREEN_HEIGHT` |
| `--texture-shift N` | 0 | `TEXTURE_SHIFT` |
| `--high-detail` | off | `HIGH_DETAIL` |

The format is in [wad-format.md](../../docs/wad-format.md).

## Generated sources

[info.py](src/femtowad/info.py) is the one source of thing types, their states and sprites.
`generate` writes `info.c` and `info.h` from it, and `r_view.c` and `r_view.h`, the view tables
of every supported view width, into the engine's [src](../../src):

```bash
uv run --project tools/femtowad femtowad generate
```

`--check` only fails if they are out of date.

## Development

```bash
cd tools/femtowad
uv run pytest
uv run ruff check
uv run ruff format --check
uv run mypy src tests
```
