# Stack Overflow

A solo, turn-based roguelite card-puzzle game built with [raylib](https://www.raylib.com/) in C. Play traditional cards onto a 3x3 "Memory Grid" to trigger scoring alignments, all while keeping the grid's total value under the **Stack Limit** — go over, and it's a `FATAL ERROR: Stack Overflow` crash.

The whole game leans into a dark "code editor" aesthetic: monospace HUD, terminal-styled crash messages, and a shop full of programming-flavored upgrades (Wildcard, Buffer Reload, Garbage Collector, Segfault Handler...).

## Gameplay

- A standard 52-card deck is shuffled each round; the 3x3 grid starts filled with 9 cards, and your hand holds 4.
- Play a card from your hand onto a free (unlocked) cell of the grid.
- Face cards trigger special effects when placed:
  - **Ace** — flexes between 1 and 11 to help you stay under the limit.
  - **Jack** — swap two cards on the grid.
  - **Queen** — absorb the value of adjacent cards and lock the cell.
  - **King** — optionally flip row/column detection to read diagonals as lines.
- Matching 3 cards in a row, column, or diagonal (same suit, straight, three-of-a-kind, or straight flush) scores points and clears/refills those cells.
- After each move, the grid's total value is checked against the round's Stack Limit — exceed it and the run ends.
- Clear each round's score objective to advance, earn gold, and spend it in the shop between rounds on one-shot Scripts and permanent Modules.

## Controls

- **Left click** — select/place a card, confirm menu choices
- **Right click** — cancel a special-power selection (Jack/Queen/King targeting)
- **H** — toggle help / tutorial
- **Y / N** — respond to King's flip prompt
- **R** — restart after a crash
- **Esc** — back out of menus/popups
- **F11** or **Alt+Enter** — toggle fullscreen

## Building

Requires a C11 compiler and [raylib](https://www.raylib.com/). On Linux, `pkg-config` is used to locate raylib automatically (falls back to `-lraylib -lGL -lm -lpthread -ldl -lrt -lX11` if `pkg-config` can't find it). Windows builds are cross-compiled with `x86_64-w64-mingw32-gcc` against the raylib SDK vendored under `lib/raylib-win64/`.

```sh
make            # build native Linux binary -> bin/stack_overflow
make windows    # cross-compile Windows binary -> bin/stack_overflow.exe
make run        # build (if needed) and launch
make BUILD=release  # optimized build (default is debug, -O0 -g)
make clean      # remove build outputs
make re         # clean + rebuild
```

Run the game from the repository root (`make run`, or `./bin/stack_overflow`) so it can find the `assets/` directory via its relative paths.

## Project layout

```
src/            Game source (C11)
assets/         Sprites, fonts, and audio (see assets/CREDITS.md for licensing)
lib/raylib-win64/  Vendored raylib SDK for Windows cross-compilation
save/           Local save/progress data (tutorial completion flag, etc.)
Specifications_Stack_Overflow.pdf  Original design document (cahier des charges)
```

## Credits

Card, UI, and audio assets are credited in [`assets/CREDITS.md`](assets/CREDITS.md) (mostly Kenney.nl, CC0).
