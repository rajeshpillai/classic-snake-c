# classic-snake

A small, self-contained Snake clone for the terminal, written in C on top of
ncurses. One source file, no dependencies beyond a curses library.

```
######################################################
# Score: 120 High: 340                               #
#                          %                         #
#            *                        %              #
#                                                    #
#                 ooooO                              #
#      %                          $                  #
#                     %                              #
######################################################
```

## Build

You need a C compiler and the ncurses development headers.

```sh
# Debian / Ubuntu
sudo apt install build-essential libncurses-dev

# Fedora / RHEL
sudo dnf install gcc make ncurses-devel

# macOS (ncurses ships with the Xcode command line tools)
xcode-select --install
```

Then:

```sh
make        # build ./snake
make run    # build and play
```

## Play

| Key            | Action        |
| -------------- | ------------- |
| `w` / `↑`      | up            |
| `s` / `↓`      | down          |
| `a` / `←`      | left          |
| `d` / `→`      | right         |
| `p`            | pause / resume |
| `q`            | quit          |

A 180-degree turn into your own neck is ignored rather than being instant
death, so mashing the opposite arrow won't kill you.

## Rules

- **Walls wrap.** Running off one edge brings you back on the other. The `#`
  border is decoration, not a hazard.
- **`%` obstacles kill.** A handful are scattered at random each game, scaled
  to the height of your terminal. The middle of the board is kept clear so you
  never spawn nose-first into one.
- **`*` normal food** is worth 10 points and one segment.
- **`$` golden food** is worth 50 points and three segments, but it rots after
  about 60 ticks and is replaced by an ordinary piece. Roughly one food spawn
  in five is golden.
- **You speed up as you score.** The tick delay starts at 120 ms and drops by
  0.5 ms per point, down to a floor of 50 ms.
- Chasing your own tail tip is legal — that cell moves out of the way on the
  same tick. Anything further up your body is not.

## High scores

The best score is kept in `~/.snake_highscore` (or `./.snake_highscore` if
`$HOME` isn't set). It's a plain text file with a single number, so it's safe
to delete or edit if you want to reset it.

## Requirements

The game needs a terminal of at least **20x10**. Anything smaller and it exits
with a message rather than drawing off-screen.

## Make targets

| Target       | What it does                                                    |
| ------------ | --------------------------------------------------------------- |
| `make`       | Build `./snake`                                                  |
| `make run`   | Build and launch                                                 |
| `make debug` | Rebuild unoptimised with `-g` and ASan/UBSan, for chasing bugs   |
| `make clean` | Remove the binary and object files                               |
| `make install` | Install to `$(PREFIX)/bin`, default `/usr/local/bin`           |
| `make uninstall` | Remove the installed binary                                  |

`PREFIX`, `CC` and `CFLAGS` can all be overridden, e.g.
`make install PREFIX=~/.local`.

## Layout

Everything lives in [snake.c](snake.c), grouped into sections:

| Section          | What's in it                                          |
| ---------------- | ----------------------------------------------------- |
| high score       | `high_score_path`, `load_high_score`, `save_high_score` |
| placement        | `occupied`, `spawn_food`, `spawn_obstacles`, `init_game` |
| drawing          | `draw`                                                |
| input            | `handle_input`                                        |
| simulation       | `step` — one tick of movement, collision and scoring   |
| screens          | `game_over_screen`                                    |
| main             | curses setup and the game loop                        |

`step()` touches no curses drawing state, which makes it straightforward to
test on its own by including `snake.c` from a harness and driving the board
directly.

## Tuning

The knobs are the `#define`s at the top of [snake.c](snake.c):
`MAX_SNAKE_LEN`, `MAX_OBSTACLES`, `INITIAL_DELAY_US`, `GOLDEN_CHANCE`,
`GOLDEN_LIFETIME`, and the `MIN_WIDTH` / `MIN_HEIGHT` floor.
