# Build options

Defined on the compiler's command line; all are optional.

## Size and detail

| Option | Default | Effect |
|---|---|---|
| `SCREEN_WIDTH`, `SCREEN_HEIGHT` | 240, 160 | the screen in pixels; must match `femtowad wads --screen` |
| `VIEW_WINDOW_WIDTH`, `VIEW_WINDOW_HEIGHT` | 60, 128 | the 3D view in pixels; the width must be in `VIEW_WIDTHS` in femtowad's `viewgen.py` |
| `MAP_WIDTH` | `SCREEN_WIDTH` | automap width |
| `TEXTURE_SHIFT`, `SPRITE_SHIFT` | 0 | walls and sprites keep every 2^n-th row; `TEXTURE_SHIFT` must match `femtowad wads --texture-shift` |
| `HIGH_DETAIL` | off | composed wall textures keep every column instead of every 4th; must match `femtowad wads --high-detail` |
| `NUKAGE_COLOR` | 122 | colour of nukage floors |
| `FLAT_WALL` | off | walls in one colour |

## Memory

| Option | Default | Effect |
|---|---|---|
| `MAX_DRAWSEGS` | by view size | wall segments per frame |
| `MAX_OPENINGS` | `VIEW_WINDOW_WIDTH` × 16 | clip range entries per frame |
| `MAX_INTERCEPTS` | 64 | lines and things one trace can cross |
| `THING_POOL_RESERVE` | 0 | thing slots kept free at map load |
| `MIN_FRAGMENT` | 16 | smallest free zone fragment split off a block |
| `NO_LOOKUP_TABLE` | off | divide instead of using the 256 KB reciprocal table |
| `RECIPROCAL_DIV(v)` | `0xffffffff / v` | the division used without the table |
| `SMALL_TRIG` | off | half-size tangent table |
| `W_FETCH` | off | WAD reads go through the platform's `W_Fetch`, see [platform.md](platform.md) |

## Features

| Option | Default | Effect |
|---|---|---|
| `NO_DEMOS` | off | no attract-mode demo |
| `LOW_MEMORY` | off | maps other than E1M1 and E1M8 spawn only player starts and keys, E1M6 plays E1M1, demos play E1M1 |
| `ST_PARTIAL_REFRESH` | off | the status bar redraws only widgets that changed |
| `DISABLE_STATUS_BAR` | off | no status bar |
| `I_Printf` | `printf` | name of the platform's log function |

## Debugging

`RANGECHECK` checks table indices, `INSTRUMENTED` logs zone use on every allocation, `DEBUG` checks zone block ids,
`ONE_WALL_TEXTURE` draws every wall with one texture.
