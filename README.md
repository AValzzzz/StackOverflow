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
- **F11** or **Alt+Enter** — toggle fullscreen (in the Web build, this hands off to the browser's own fullscreen API instead)

## Building

Requires a C11 compiler and [raylib](https://www.raylib.com/). On Linux, `pkg-config` is used to locate raylib automatically (falls back to `-lraylib -lGL -lm -lpthread -ldl -lrt -lX11` if `pkg-config` can't find it). Windows builds are cross-compiled with `x86_64-w64-mingw32-gcc` against the raylib SDK vendored under `lib/raylib-win64/`.

```sh
make            # build native Linux binary -> bin/stack_overflow
make windows    # cross-compile Windows binary -> bin/stack_overflow.exe
make web        # build the Web/WebAssembly version -> web/index.html
make run        # build (if needed) and launch
make BUILD=release  # optimized build (default is debug, -O0 -g); works for web too
make clean      # remove build outputs
make re         # clean + rebuild
```

Run the game from the repository root (`make run`, or `./bin/stack_overflow`) so it can find the `assets/` directory via its relative paths.

## Web build (Emscripten / WebAssembly)

The game also compiles to WebAssembly via [Emscripten](https://emscripten.org/), for playing directly in a browser (e.g. an itch.io HTML5 page). It's the same C11 source and the same `assets/`/`save/` conventions as the native builds — no gameplay code was rewritten for the web.

### Requirements

- The [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) installed and activated, so `emcc` is on `PATH`:
  ```sh
  source /path/to/emsdk/emsdk_env.sh
  ```
- A raylib build for `PLATFORM_WEB`, vendored at `lib/raylib-web/` (the same way `lib/raylib-linux/` and `lib/raylib-win64/` are vendored for their platforms). If that directory doesn't exist yet, generate it with:
  ```sh
  ./scripts/build_raylib_web.sh
  ```
  That script downloads raylib source and builds `libraylib.a` with `emcc`; see the script for details. It only needs to be re-run when updating the raylib version.

### Build

```sh
make web                  # debug build -> web/index.html, .js, .wasm, .data
make web BUILD=release    # -O2 optimized build
make dist-web              # release build, zipped to dist/StackOverflow-Web.zip (index.html at the zip root, ready for itch.io)
```

`assets/` is packaged into `web/index.data` via Emscripten's `--preload-file`, so the browser build is fully self-contained — it never reads from the developer's local `assets/` folder at runtime, only from the files shipped alongside `index.html`.

### Local testing

Emscripten output must be served over HTTP (opening `index.html` via `file://` will not work — WebAssembly and the packaged asset data both require a real server):

```sh
cd web
python3 -m http.server 8000
```

Then open `http://localhost:8000` in a browser.

### Save persistence in the browser

`save/*.dat` and the tutorial-completion flags are written with the same plain `fopen()`/`fwrite()` calls as the native build (see `src/save.c`, `src/settings.c`, `src/tutorial.c` — unchanged for the Web target). In the browser, that `save/` directory is mounted on an [IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs) filesystem backed by IndexedDB (set up in `src/main.c`, web-only code path): the game reads any previously persisted data at startup, and a background sync (every 3 seconds, and on tab-hide) flushes writes back to IndexedDB so progress survives a page reload. If IndexedDB is unavailable, the sync callback still fires (with an error logged to the console) and the game keeps running against the in-memory filesystem for that session — it never blocks startup or crashes on missing browser storage.

### Known Web limitations

- Browsers block audio autoplay before a user gesture; the page shows an "Enable audio" button when the audio context starts suspended (see `platform/web/shell.html`) rather than trying to force playback.
- `F11`/`Alt+Enter` call raylib's `ToggleBorderlessWindowed()`, which on `PLATFORM_WEB` uses the browser's Fullscreen API — this generally works from a keypress, but browsers may restrict it further inside an iframe (e.g. itch.io); the page's own "Fullscreen" button and itch.io's fullscreen control are the reliable fallback.
- Tested with headless Firefox (Selenium/geckodriver) during development; do a manual spot-check in Chrome (and Safari, if available) before shipping, since this environment had no Chromium install available to test against directly.

### Packaging for itch.io

```sh
make dist-web
```

produces `dist/StackOverflow-Web.zip` with `index.html` at the root of the archive (not nested in a `web/` folder), containing everything the game needs (`index.html`, `index.js`, `index.wasm`, `index.data`). On itch.io: create/edit the project, set the kind to **HTML**, upload that zip as a new file, and check "This file will be played in the browser". itch.io should auto-detect `index.html` as the entry point. Keep the existing Windows/Linux downloads alongside it — uploading the Web build doesn't replace or require removing them.

## Project layout

```
src/            Game source (C11)
assets/         Sprites, fonts, and audio (see assets/CREDITS.md for licensing)
lib/raylib-win64/  Vendored raylib SDK for Windows cross-compilation
lib/raylib-web/    Vendored raylib SDK built for PLATFORM_WEB (see scripts/build_raylib_web.sh)
platform/web/   Web build support files (HTML shell template used by `make web`)
scripts/        Developer scripts (e.g. regenerating lib/raylib-web/)
save/           Local save/progress data (tutorial completion flag, etc.)
web/            Generated Web build output (gitignored; created by `make web`)
Specifications_Stack_Overflow.pdf  Original design document (cahier des charges)
```

## Credits

Card, UI, and audio assets are credited in [`assets/CREDITS.md`](assets/CREDITS.md) (mostly Kenney.nl, CC0).
