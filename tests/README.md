# PicWorks unit tests

Headless tests for logic that does not require an X11 display. These are intended as a safety net before refactors (especially the TrueColor port in `plan.md`).

## Run

From the repository root (after `./configure && make`):

```bash
make test
```

Or only the pw/iomedley test binary:

```bash
make -C tests check
```

**libXfred** tests live under `lib/tests/` — run with `make -C lib check` (see `lib/tests/README.md`).

Set `PW64_TEST_FIXTURES` if fixtures are not in the default tree (each Makefile sets this for `make check`).

## Coverage today (22 tests in `tests/`, plus libXfred)

| Area | Where | What is exercised |
|------|-------|-------------------|
| **display_rgb** | `tests/test_display_rgb.c` | `pw_color_at`, `pw_lut_index_to_rgb` (fixed test masks), composite LUT preference |
| **iomedley** | `tests/` | BSQ/BIL/BIP dimension macros, byte swap, VAX↔IEEE float, PNM detect/load/write, `iom__ConvertToBIP` |
| **pw/pseudo.c** | `tests/` | `pseudocolor()`, `quantize()` including slot base `2` |
| **pw/quant.c** | `tests/` | (linked with pseudo tests) |
| **pw/color.c** | `tests/` | RGB↔HSV, mix, distance, `XColorToRGB` |
| **libXfred** | `lib/tests/` | Packbits, `is_dir`/`is_file`, `insert_str`, parsers, `XfHersheyWidth`, `get_sorted_dir` |

Widget code (`Button.c`, `Slider.c`, `List.c`, …) still needs **Tier C** X11 tests (`check-x11`, not implemented).

---

## libXfred: what we can add

See `lib/tests/README.md` for the current headless suite. `libXfred.a` is ~20 translation units of X11 UI; only a few files expose **pure C** helpers suitable for the `test.h` harness.

### Tier A — headless (no `DISPLAY`)

Implemented in `lib/tests/test_xfred.c`. Link `libXfred.a` with `-lX11` (no display opened).

| Symbol (file) | Proposed tests |
|---------------|----------------|
| `ToTiff` / `UnTiff` (`misc.c`) | ✅ roundtrip + run; add **empty** buffer, **single** byte, **128** identical bytes (max run), literal/run boundary |
| `insert_str` (`xgets.c`) | ✅ middle insert; add **at 0**, **at end**, empty `t` |
| `is_dir` / `is_file` (`misc.c`) | ✅ `/tmp` dir; add **regular file** (`/etc/hosts` or fixture), missing path (`is_file` returns 0; **do not** call `is_dir` on missing paths — no `stat` check) |
| `get_int` / `get_float` / `getbit` (`misc.c`) | Parse from `tmpfile()` / `fmemopen`: leading whitespace, `#` comment line, invalid token → -1 |
| `XfHersheyWidth` (`hershey.c`) | Width of `'A'` / `' '` for `ROMAN_SIMPLEX` (cset 16); monotonic wider for `'W'` vs `'i'` |
| `get_sorted_dir` (`chd.c`) | Create `tests/fixtures/chd/sorted/` with files `beta`, `alpha`, `gamma`; assert lexicographic order, `.` / `..` skipped |

**Linking note:** These functions are not in public headers; tests use `extern` declarations (same pattern as `test_lib.c`).

### Tier B — headless but awkward

| Symbol (file) | Issue | Tests |
|---------------|-------|-------|
| `complete_dir` / `complete_match` (`chd.c`) | Global `dn`, `dirs`; `sort_dirs` may **recurse** and mutate path on error | One test per process or explicit teardown; fixture tree under `tests/fixtures/chd/comp_*` |
| `file_completion` (`xgets.c`) | Needs `Display *` for `XBell` only | Low priority; mock display or skip |

### Tier C — needs X11 (`DISPLAY` or `xvfb-run`)

Optional target: `make -C tests check-x11` (not implemented yet). Skip in default `make test` if `DISPLAY` unset.

| Goal | Maps to pitfall (see `plan.md`) | Idea |
|------|----------------------------------|------|
| Solid fill | **XF-1** | `initx` → `XfCreateButton` + `XfSolidVisual` white fill → `UpdateButton` → `XGetImage` center pixel |
| Destroy after dispatch | **XF-3** | Callback sets pending destroy; main loop destroys after `XfButtonPush` |
| Visual ownership | **XF-4** | Two buttons, each with own `XfPixmapVisual`; destroy one; other still draws |
| `XfCreateVisual` sizing | API contract | `width==0` / `height==0` → `gen->width - x` per `VisInfo.c` |
| Font type | **XF-2** | Build-time / runtime assert text visual uses `XFontStruct *` |

**Runner example:**

```bash
xvfb-run -a make -C tests check-x11
```

### Tier D — manual

| Target | Role |
|--------|------|
| `lib/panes` | End-to-end Xfred buttons, colors, intro overlay, click dismiss |
| `lib/sampler` | All widget types on one panel (PB/CB/RB/LB/MB, sliders, joystick, AMap, list, composite, confirm, GetText) — `make -C lib sampler && ./lib/sampler` |
| `pw` GUI | Full app smoke (see `plan.md` manual checklist) |

---

## Not covered (and poor ROI for unit tests)

| Area | Why |
|------|-----|
| `stretch_gray` / `update_display` E2E | Full `image.c` link + `pw` globals; slot indices covered indirectly via `display_rgb` |
| VICAR/ISIS/ENVI loaders | Golden files under `tests/fixtures/` (iomedley) |
| `xgets`, `do_keysym`, `decode_control` | Static `anchor`; needs X event injection |
| `HitTest`, `ListIsDoubleClick`, `draw` | Widget + font layout |
| `AMap`, `Slider`, `Joystick` | Motion, expose, callbacks |
| `sp` | Separate binary |

---

## Implementation notes

- `tests/compat/Xfred.h` is a stub so `pseudo.c` / `quant.c` compile without the full widget tree.
- `insert_str()` mutates its **second** argument (`t`); callers must pass a writable buffer (see `test_insert_str`).
- `is_dir()` does not check `stat()` failure; do not call it on missing paths.
- `get_float()` does not skip `#` comment lines (unlike `get_int()`); tests use plain numeric input.
- `getbit()` does not re-read after `#` comment skip (`ch` stays `#`); comment lines are not covered by tests.
- `ToTiff(buf, len==0)` returns 2 (legacy); see `test_packbits_zero_length_input`.
- **`chd.c` globals:** if adding `complete_dir` tests, do not run them in parallel workers.
- **Refactor candidates** (enables more tests): export `anchor` for `xgets` tests; small `xfred_draw_solid(Display *, Drawable, …)` shared by `Button.c` and tests; `set_state` bounds check (**XF-5**) in `toggle.c`.

## Done (Tier A)

- `lib/tests/test_xfred.c` — 16 libXfred tests.
- `lib/tests/fixtures/chd/sorted/{alpha,beta,gamma}` — directory sort fixture.

## Suggested next PR

- Optional `make -C tests check-x11` with `xvfb-run` (Tier C: solid fill, deferred destroy).
