# Lesson 11 — The game loop

**Program:** `gameloop.c` · **Links:** `-lncurses`

```sh
make 11
```

SPACE cycles the pacing strategy, `+`/`-` change the target rate. Watch the
*measured* fps against the *target*.

## The shape of every interactive program

```c
while (running) {
    handle_input();      /* drain everything pending */
    update_state();      /* one tick of simulation */
    draw();              /* compose and push one update */
    pace();              /* wait out the rest of the frame */
}
```

The interesting part is `pace()`. Get it wrong and the program either burns a
CPU core or feels sluggish.

## Draining input

```c
int ch;
while ((ch = getch()) != ERR) handle_key(ch);
```

With `nodelay` on, this consumes everything the terminal has buffered and
then returns `ERR`. It matters because terminals buffer key repeats: hold an
arrow key for a second and you may have twenty events queued. Handle one per
frame and the snake keeps turning for a second after you let go.

Draining also means the *last* key of the frame wins, which is what players
expect.

`flushinp()` is the sledgehammer version — discard buffered input entirely,
without processing it. Use it after a long pause or before a "press any key"
prompt, as the snake game's `game_over_screen` does.

## Three ways to pace

### 1. Fixed sleep — what the snake game does

```c
napms(target_ms);
```

The frame takes `work + target`, so the real rate is always below target. If
drawing takes 5 ms and you sleep 33, you get 26 fps, not 30.

For snake this is fine: the work is a few hundred `mvaddch` calls, so the
error is under a millisecond, and nobody can perceive a snake moving at 7.9
cells/sec instead of 8.

It stops being fine when the work is variable. A frame that occasionally
takes 20 ms makes the game visibly stutter.

### 2. Compensated sleep

```c
long remaining = target_ms - work_ms;
if (remaining > 0) napms(remaining);
```

Measure the work, sleep the remainder. The rate holds steady as long as the
work fits in the budget.

Note what it does *not* do: when a frame overruns, it doesn't run extra
frames to catch up. That "accumulator" approach is standard in graphics
engines, but in a terminal it produces a burst of teleporting movement — a
dropped frame is better.

### 3. `timeout()` — let curses wait

```c
timeout(remaining);
int k = getch();     /* returns as soon as a key arrives, or on timeout */
```

The sleep and the input wait become the same operation. The frame is still
bounded, but a keypress is acted on the *instant* it arrives instead of after
the remaining sleep.

That difference is real input latency. With a 100 ms tick and a fixed sleep,
a key pressed just after the drain waits up to 100 ms. With `timeout` it
waits zero.

**This is the best default for an interactive TUI**, and it's the one change
that would most improve the snake game's feel.

## Monotonic time

```c
static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}
```

`CLOCK_MONOTONIC`, not `gettimeofday()` or `time()`. Wall-clock time can jump
backwards — NTP corrections, DST, a user setting the clock — and a negative
frame duration will produce a `napms()` of a nonsense value.

`clock_gettime` needs `_POSIX_C_SOURCE >= 199309L`, which `-D_DEFAULT_SOURCE`
provides. That's why it's in the tutorial `CFLAGS`.

## `napms` vs `usleep`

`napms(ms)` is curses' own sleep. Prefer it inside a curses program: it's
portable, it's in milliseconds (which is the unit you're thinking in), and
it's aware of the curses I/O state.

`usleep()` works and is what the snake game uses, but it's been removed from
POSIX-2008 in favour of `nanosleep`, and it needs feature-test macros to be
visible under `-std=c11`.

## Separating tick rate from frame rate

`gameloop.c` and the snake game both advance the simulation once per frame,
so speeding up the game means shortening the frame. That's the simple design,
and it couples animation smoothness to game speed.

The alternative is a fixed frame rate with an accumulator:

```c
accumulator += elapsed_ms;
while (accumulator >= tick_ms) {
    update_state();
    accumulator -= tick_ms;
}
draw();
```

Now you can redraw at 60 fps while the snake moves 8 times a second, which
gets you smooth interpolated movement. For a grid-based game where things
move one whole cell at a time there's nothing to interpolate — which is why
snake doesn't bother.

## Speed ramps

```c
useconds = INITIAL_DELAY_US - (score * 500);
if (useconds < 50000) useconds = 50000;
```

The snake game's difficulty curve, and worth reading carefully:

- It's **linear in score**, not in length — golden food (+50) causes a
  noticeable jump in speed.
- The **floor matters**. Without it, a score of 240 gives a delay of zero
  and the game becomes unplayable; at 300 it goes negative, and
  `usleep()` with a huge unsigned value would hang.

Any speed ramp needs a clamp at both ends. Clamping only where you expect
to hit is how you find out your expectation was wrong.

## Exercises

1. Run the three strategies at 5 ms and compare measured fps.
2. Add `napms(10)` inside the draw phase to simulate expensive work, then
   watch strategy 1 fall behind while 2 and 3 hold.
3. Convert the snake game's main loop to `timeout()` and see whether the
   controls feel more responsive.
4. Add an accumulator so the balls move at a fixed rate while the frame
   rate varies.

---

Previous: **[Lesson 10](../10-panels/lesson.md)** ·
Next: **[Lesson 12 — Mouse](../12-mouse/lesson.md)**
