# classic-snake

A small, self-contained Snake clone for the terminal, written in C on top of
ncurses. One source file, no dependencies beyond a curses library.

**▶ [Play it in your browser](https://USERNAME.github.io/classic-snake/)** —
the same `snake.c`, compiled to WebAssembly.

> **Set your Pages URL.** Replace `USERNAME` above with your GitHub username
> once Pages is enabled (Settings → Pages → Source: `main`, folder: `/docs`).
> One command: `sed -i 's|USERNAME|your-username|' README.md`

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

## Playing in a browser

[docs/index.html](docs/index.html) is the whole game compiled to WebAssembly —
one self-contained file with the wasm inlined, so it works from GitHub Pages
or straight off your disk.

**`snake.c` is not modified for the web build.** It is compiled byte-for-byte
identically for both targets. The differences are entirely in the build flags:

| Flag | Why |
| --- | --- |
| `-Iweb` | makes `#include <ncurses.h>` resolve to [web/ncurses.h](web/ncurses.h) instead of the real library |
| `-Dusleep=shim_usleep` | routes the game's pacing through `emscripten_sleep()` |
| `-sASYNCIFY` | lets the blocking `while (running)` loop yield to the browser and resume |

[web/curses_shim.c](web/curses_shim.c) implements the ~30 curses symbols the
game actually uses, backed by a cell buffer that JavaScript paints onto a
canvas. Each cell is one `uint32` — `char | pair << 8 | attrs << 16` — so the
renderer reads the whole screen out of the wasm heap in a single typed-array
pass.

The browser build also gets the high score persisted to IndexedDB (via
Emscripten's IDBFS mounted at `$HOME`, so `snake.c`'s plain `fopen` keeps
working), an audible blip for `beep()`, and swipe plus on-screen d-pad
controls on touch devices.

### Building it

```sh
source ~/emsdk/emsdk_env.sh    # needs the Emscripten SDK
make web                       # -> docs/index.html
make web-serve                 # build and serve at localhost:8000
```

Install the SDK with:

```sh
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
```

### Deploying

[gh-deploy.sh](gh-deploy.sh) builds, verifies and publishes to GitHub Pages.
It creates the repo if it doesn't exist and points Pages at the branch, both
through the `gh` CLI, so a first deploy needs no clicking through Settings.

```sh
./gh-deploy.sh              # dry run: build and verify, push nothing
./gh-deploy.sh --push       # build, verify and publish
```

`docs/` is generated and git-ignored — the built page is force-pushed to the
`gh-pages` branch as a single orphan commit, so the generated bundle never
lands in `main`'s history. Before pushing, the script checks that the wasm
really got inlined, that no stray `.wasm` was emitted, and that the file is
still valid UTF-8 (Pages serves it as `charset=utf-8`, and Emscripten embeds
the wasm as a raw binary string — invalid sequences would corrupt it).

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
| `make web`   | Compile to WebAssembly → `docs/index.html` (needs emsdk)          |
| `make web-serve` | Build the web version and serve it at `localhost:8000`        |
| `make debug` | Rebuild unoptimised with `-g` and ASan/UBSan, for chasing bugs   |
| `make clean` | Remove the binary and object files                               |
| `make clean-web` | Remove the generated `docs/index.html`                        |
| `make install` | Install to `$(PREFIX)/bin`, default `/usr/local/bin`           |
| `make uninstall` | Remove the installed binary                                  |

`PREFIX`, `CC` and `CFLAGS` can all be overridden, e.g.
`make install PREFIX=~/.local`.

## Layout

```
snake.c              the whole game -- terminal and web builds share it
Makefile             native + wasm targets
gh-deploy.sh         build, verify and publish to GitHub Pages
web/                 browser support, none of it game logic
  ncurses.h            curses subset the game needs, as a drop-in header
  curses_shim.c        implementation, backed by a cell buffer
  shell.html           canvas renderer, input, persistence
docs/                generated by `make web`, git-ignored
tutorial/            a 15-lesson ncurses course
```

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
