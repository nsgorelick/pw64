# PicWorks TrueColor Port Plan

## Context

PicWorks (`pw`) was built for **8-bit PseudoColor** displays: image pixels in `sdata` hold X colormap **pixel IDs**, and `XStoreColors` / `XAllocColorCells` drive false-color and overlay behavior. Modern X servers (including XQuartz) expose **TrueColor 24/32-bit** defaults only—depth 8 may appear in pixmap formats, but there is no `PseudoColor` visual.

This plan ports the **display path** to TrueColor while preserving scientific behavior (stretch, LUT editing, compositing, overlays, color budget).

**Non-goals (initial phases):** GUI toolkit rewrite, Wayland, replacing `iomedley`, ImageMagick upgrade.

---

## Design summary

### Core invariant change

| Layer | Today | After port |
|-------|--------|------------|
| `data` | 16-bit (etc.) cube samples | unchanged |
| `sdata` | `unsigned char` **X pixel values** (opaque IDs from `XAllocColorCells`) | `unsigned char` **absolute LUT slot indices** in the shared color budget (typically `2 … NColors+1`) |
| `Colors[0..1]` | reserved black/white (pixel cells) | reserved LUT entries (RGB only); `.pixel` for UI only |
| `Colors[2 …]` | image/overlay LUT + hardware cells | LUT RGB per slot; `.pixel` unused for image LUT |
| `Image->Colors[]` | per-composite LUT + `.pixel` | per-composite LUT RGB only (see resolution rules below) |
| Display buffer | 8-bit `XImage` **aliases `sdata`** | separate **`rgb_data`** (24/32-bit) + `XImage`; `sdata` never passed to `XCreateImage` |
| Palette update | `XStoreColors` | update LUT in memory → **`pw_refresh_image`** (full or dirty rects) |

**Important:** `sdata` is **not** a per-image local index `0 … ncolors-1`. Base greyscale, false-color composites, and overlays all use **absolute slots** in one shared budget (same as today, but storing slot numbers instead of X pixel IDs). Local color index `j` for a plane maps to slot `2 + j` for the base image; overlays use `start + i` where `start` accumulates prior planes’ `ncolors`.

### Index encoding specification (canonical)

Document this in `display_rgb.h` and enforce in all producers/consumers of `sdata`:

| Value in `sdata` | Meaning |
|------------------|---------|
| `0`, `1` | Reserved (black/white LUT entries); rarely appear in image buffers |
| `2 … 2+base_ncolors-1` | Base image greyscale or composite output slots |
| `2+start …` | Overlay slots; `start` = sum of `ncolors` for base + prior overlays |

**Producers** (must write slot indices, never `.pixel`):

- `stretch_gray`: `sdata[i] = (unsigned char)(2 + j)` for bin `j` in `0 … ncolors-1`
- `stretch_color` / `pseudocolor`: `out_map[k] = (unsigned char)(2 + k)` (not `Colors[k+2].pixel`)
- `overlay`: `data[…] = (unsigned char)(start + i)` when overlay color `i` is enabled (`map[i] != 0`)
- `quantize(…, base)`: `base = 2` (first slot of that plane’s LUT range), not `Colors[2].pixel`

**Consumers:** `pw_refresh_image`, `pw_color_at`, export (`write.c`), magnifier sampling.

**Grep audit caveat:** `pw/read.c` uses a local `short *sdata` for cube extraction—that is unrelated to `Image->sdata`. Exclude it from display-path audits.

**Dead code:** `pw/image2.c` duplicates `image.c` and is not in the build; delete or merge during the port to avoid confusion.

### New display helper module

Add `pw/display_rgb.c` + `pw/display_rgb.h`:

- `void pw_init_display(Display *d, int screen);` — cache visual, depth, masks, bytes per pixel.
- `int pw_display_depth(void);` — 24 or 32.
- `int pw_color_at(Image img, unsigned char slot, XColor *out);` — resolve RGB for a slot (see LUT rules).
- `void pw_lut_index_to_rgb(Image img, const unsigned char *indices, unsigned char *rgb, int npixels);` — bulk rasterize via `pw_color_at`.
- `XImage *pw_create_rgb_image(Display *d, unsigned char *rgb_data, int w, int h);`
- `void pw_free_rgb_image(XImage *img);` — free image and its buffer if owned.
- `void pw_refresh_image(Image img);` — `sdata` + LUTs → (re)allocate `rgb_data`, update `ximage`, optional pixmap.

Centralizing this avoids duplicating byte-order / pad logic across `composite.c`, `mag.c`, pan, histogram, and color-spread code.

### `pw_color_at` resolution rules

```c
/*
 * slot: absolute index in sdata (2 .. NColors+1)
 * Returns RGB for rasterization and export.
 */
```

| Image type | Slot range | RGB source |
|------------|------------|------------|
| Greyscale base | `2 … 2+ncolors-1` | `Colors[slot]` |
| False-color composite | `2 … 2+ncolors-1` | Prefer `Image->Colors[slot - 2]` if populated (per-composite LUT from merged planes); else `Colors[slot]` |
| Display after overlays | mixed slots in one buffer | Always `Colors[slot]` (overlays only patch `sdata`; global LUT holds overlay slot colors) |

After **any** change to `sdata` (stretch, overlay merge, `image_hold` restore), call `pw_refresh_image` before `XPutImage` / pixmap copy.

### XImage ownership

Today `update_display()` does `XCreateImage(…, new->sdata, …, 8, 0)` — the image **aliases** the index buffer.

After port:

- `sdata` = index plane only; stable across LUT edits until stretch/overlay recomputes it.
- `rgb_data` = owned display buffer; `ximage->data` points here (or pixmap cache at `pw_display_depth()`).
- `free`/`realloc` of `sdata` invalidates any stale `ximage`; only `pw_refresh_image` rebuilds display buffers.
- `load_fast_mem` / `IPixmap` / scrolling (`IXPos`, `IYPos`) must use the **RGB** `ximage` or pixmap at display depth, not depth 8.

### `NColors` without `XAllocColorCells`

Replace `alloc_colors()` cell allocation with a **fixed software budget**:

- Default `NColors = 220` (or 253), configurable later; reserve `Colors[0]` black, `Colors[1]` white for UI conventions.
- Image/overlay slots: `Colors[2]` … `Colors[NColors+1]`.
- UI chrome (`pwRed`, `pwBackground`, …): `XAllocColor` on the **default TrueColor** colormap (`.pixel` valid for GCs only).
- Remove `PlaneMask` / `pwHilite.pixel = pixel | PlaneMask[0]`; use a distinct allocated highlight color.
- Remove or no-op `MakePrivateColormap()` (stub only if needed for bisect).
- **UI state:** replace pointer hacks like `MixBox->ext == Colors[0].pixel` with integer slot IDs or enums (`ColorControls.c`).

Validate overlay budget in UI: `overlay()` silently returns when `start > NColors` — surface an error before silent failure.

---

## Architecture diagram

```mermaid
flowchart TB
  subgraph data [Data plane - unchanged]
    D[data 16-bit cube]
  end
  subgraph index [Index plane - slot indices in sdata]
    S[sdata absolute slot 2..]
    L[Colors / Image.Colors LUT]
  end
  subgraph display [Display plane - new]
    R[rgb_data 24/32-bit]
    X[XImage + optional pixmap]
  end
  D --> stretch_gray / stretch_color / pseudo
  stretch_gray --> S
  stretch_color --> S
  pseudo --> S
  overlay --> S
  L --> pw_color_at
  S --> pw_lut_index_to_rgb
  pw_lut_index_to_rgb --> R
  R --> X
  X --> XPutImage
```

---

## File impact map

### Tier 1 — must change (display correctness)

| File | Changes |
|------|---------|
| `pw/display_rgb.c` | **new** — rasterization, `pw_color_at`, `XCreateImage` at visual depth |
| `pw/display_rgb.h` | **new** — index encoding spec, public API |
| `pw/pw.c` | `alloc_colors()` rewrite; `IPixmap` at `pw_display_depth()` (not hardcoded 8); `LoadImageWindow` uses RGB image; drop private colormap install on image window |
| `pw/composite.c` | `update_display()` → refresh RGB, never alias `sdata`; `overlay()`, `quantize` base; `load_fast_mem`; `create_hist` (see below); `create_pan` |
| `pw/image.c` | `stretch_gray` / `stretch_color`: slot indices; `out_map`; drop `Colors[i].pixel` on image LUT |
| `pw/pseudo.c` | `out_map[i] = 2+i`; `build_table` lut indexing for slot-based inputs; gate/remove debug `printf` |
| `pw/quant.c` | `quantize(…, base)` with `base = 2` (+ plane offset for overlays) |
| `pw/colorspread.c` | `RGB_CS`: no `XStoreColors`; `CreateCSData`: RGB buffer or slot indices + refresh |
| `pw/ColorControls.c` | LUT-only updates + `pw_refresh_image`; fix `MixBox->ext` pixel compare; 8-bit `XfXImageVisual` → RGB |
| `pw/setcolor.h` | `setcolor_lut()` vs `setcolor_x11()` |
| `pw/image.h` | `rgb_data`; ownership comments |
| `pw/Makefile` | `display_rgb.o` |

### Tier 2 — satellite views (same raster path)

| File | Changes |
|------|---------|
| `pw/mag.c` | `mag_data` as RGB tile; `ColorMag` / `UncolorMag` use slot→RGB; `create_mag` samples `sdata` indices |
| `pw/buttons.c` | Pan/histogram bindings refresh RGB images |
| `pw/write.c` | Remove `XQueryColors` export path; map slot → RGB via `pw_color_at`; XPM from RGB `XImage` |
| `pw/composite.c` | `create_pan`: RGB `pan_image` (not `bitmap_pad` 8 on index buffer) |

### Tier 3 — verify / smaller touch

| File | Notes |
|------|-------|
| `pw/read.c` | **Not** display `sdata` — local cube buffer only |
| `pw/readout.c` | `AddToDnList`: keep `color->pixel` for GC (UI), not image slots |
| `pw/plot.c` | Plot case uses 8-bit `XfXImageVisual` — convert or isolate |
| `pw/rose.c`, `pw/block.c`, `pw/hist.c` | UI colors + `ColorMag` slot values |
| `pw/restart.c` | Restore must not depend on `XAllocColorCells`; C1/C2 RGB OK |
| `lib/xf.c` | `XfColor()` / `XAllocColor` — OK on TrueColor |
| `sp/sp.c` | Align color init; drop `PlaneMask` if present |

### Remove / ignore

| File | Action |
|------|--------|
| `pw/image2.c` | Duplicate of `image.c`; not built — delete during port |

### libXfred (`lib/`) — widget layer

- **Not display-path logic**, but heavily used by `pw/` for panels, buttons, sliders, color spread, confirm dialogs.
- **`lib/Button.c` was corrected** while reviving `lib/panes.c` (2025): `XfSolidVisual` now fills; callback list walk is safe if a callback destroys the button.
- **Audit `pw/`** against the pitfalls in [libXfred / X11 widget pitfalls](#libxfred--x11-widget-pitfalls-audit-checklist) below — same mistakes likely appear outside `panes.c`.

### Unchanged

- `iomedley/**` — file I/O only.

### Known breakage on TrueColor today (fix in Tier 1/2)

| Location | Problem |
|----------|---------|
| `create_hist` | `hist_data[i] = WHITE(display)` / `pwBlue.pixel` stored in **bytes** — valid only when pixel IDs are small; garbage on TrueColor |
| `CreateCSData` | Writes `colors[k].pixel` into spread widget buffer |
| `write.c` | `XQueryColors(display, ColorMap, …)` assumes indices 0–255 match hardware map |

---

## libXfred / X11 widget pitfalls (audit checklist)

**Origin:** Debugging `lib/panes.c` (Xfred demo) on XQuartz exposed several bugs that are **library or API-contract issues**, not TrueColor-specific. The same patterns almost certainly exist in `pw/` widgets built on `XfCreateButton`, `XfAddButtonVisual`, and `XfButtonPush`.

**Reference implementations in-tree:**

| Pattern | Good example |
|---------|----------------|
| Modal text + white panel | `lib/confirm.c` — `XfSolidVisual` fill, then `XfTextVisual` with `BLACK` fg; line `ypos` from font metrics |
| Filled solid regions | `lib/AMap.c`, `lib/Slider.c`, `lib/Joystick.c` — `XFillRectangle` for `XfSolidVisual` |
| Font on text visuals | `lib/Button.c` `Make2State()` — passes `XFontStruct *` into `XfCreateVisual(…, XfTextVisual, …)` |

### Pitfall summary

| ID | Symptom | Root cause | Correct approach | Audit (`pw/`, `lib/`) |
|----|---------|------------|------------------|-------------------------|
| **XF-1** | Widget “filled” areas stay **black**; intro text invisible | `defaultButtonCallback` treated `XfSolidVisual` like `XfOutlineVisual` (`XDrawRectangle` only) | **`XFillRectangle`** for `XfSolidVisual` (fixed in `lib/Button.c`) | `grep -r XfSolidVisual pw lib` — any custom expose/update callbacks? |
| **XF-2** | **Segfault on expose** / garbage text | `XfTextVisual` expects **`XFontStruct *`**; code passes **`Font`** (`fid` XID) cast as a pointer | `XLoadQueryFont` → store pointer → `font->fid` for `XSetFont` | `grep -r XfTextVisual pw lib` — trace 3rd vararg after text string |
| **XF-3** | **Crash on click** (often **next** click after dismissing a dialog) | `XfDestroyButton()` called **inside** a callback invoked from `XfButtonPush()`; dispatcher then reads **`execs->next`** on freed memory | **Defer destroy** until after `XfButtonPush` returns; library saves `next` before calling each callback | `grep -r XfDestroyButton pw lib` — ensure not called from same-button `ButtonPress` handler |
| **XF-4** | Random draw corruption / crash on destroy | `XfAddButtonVisual()` **does not copy** `VisualInfo`; sharing one struct across buttons or states corrupts **`next`** / double-frees pixmaps | **One visual struct per (button, state)** attachment; recreate pixmaps per button | Same `VisualInfo*` passed to multiple `XfAddButtonVisual` calls |
| **XF-5** | **Segfault in `defaultButtonCallback`** | `Update()` / logic sets **`state` to -1** or `>= maxstate` (e.g. unchecked `Land()` / `Next()` return) | Validate state before `B->state = …`; guard `B->States[state]` | `grep -rn 'Update(' pw lib` and state arithmetic |
| **XF-6** | Text “missing” on TrueColor (and often 8-bit) | `XfTextVisual` draws **foreground only**; default `XCreateSimpleWindow` **background is 0**; **black-on-black** if fg is `BLACK(display)` without a fill | Add **`XfSolidVisual`** fill (or `XFCreateButton(…, bg, …)`) **before** text; use **black on white** like `confirm.c` | Dialogs/overlays using `XfTextVisual` without solid underlay |
| **XF-7** | Wrong UI state after undo / restore | Snapshotting widget state with **`% 7`** (or similar) when states use **offsets** (e.g. base + 7 for “highlighted”) | Store **full integer state** in undo/history | `grep -r '% 7' pw lib` near `->state` |
| **XF-8** | OOB access from button callbacks | **`sscanf(b->name, …)`** or `ext` without validation | Check `sscanf` return count; bounds-check parsed indices | `grep -r sscanf pw lib` on widget names |
| **XF-9** | Wrong colors in **image buffers** (Tier 1) | Storing **`Colors[i].pixel`** or **`WHITE(display)`** in **8-bit index buffers** | Slot indices in `sdata`; UI `.pixel` only for GCs (see [Index encoding](#index-encoding-specification-canonical)) | `grep '\.pixel' pw` writes into `sdata` / `hist_data` / spread buffers |

### XF-1 detail: solid vs outline

`VisInfo.h` defines `XfSolidVisual` and `XfOutlineVisual` as distinct types. Callers (including `lib/confirm.c`, `lib/panes.c`) assume **solid means filled**. Until `Button.c` was fixed, only AMap/Slider/Joystick update paths filled solids; **all stock `XfCreateButton` expose/update paths** outlined them instead.

**After port work:** Any `pw` code that implements its **own** button-like expose handler (e.g. `ColorControls.c` 8-bit `XfXImageVisual`) should be checked separately; do not assume Tier 1 RGB work fixes Xfred drawing.

### XF-3 detail: deferred destroy pattern

```c
static Button destroy_pending;

void on_dismiss(Button B, XEvent *E) {
    (void)E;
    destroy_pending = B;   /* not XfDestroyButton(B) here */
}

/* main loop */
destroy_pending = NULL;
XfButtonPush(XfEventButton(&E), &E);
if (destroy_pending != NULL) {
    XfDestroyButton(destroy_pending);
    destroy_pending = NULL;
}
```

`lib/Button.c` also copies `execs->next` **before** invoking each callback so a destroy cannot corrupt the walk (still prefer deferral so `B` remains valid for the rest of the dispatcher).

### XF-9 vs XF-6

| Context | Use |
|---------|-----|
| **Image `sdata`, histogram bytes, color-spread buffer** | LUT **slot index** or RGB via `pw_color_at` — never hardware pixel ID (see Tier 1 table) |
| **Xfred widget GC / text / borders** | `XAllocColor` → `Colors[i].pixel`, `BLACK(display)`, `WHITE(display)` — normal for TrueColor **UI** |

### Suggested audit pass (before / during Phase 3)

1. Run greps from the table on `pw/` and `lib/`.
2. Manually open confirm dialogs, color spread, file panels, plot windows — confirm fills and text on XQuartz.
3. Click-to-dismiss any overlay built on `XfCreateButton` — no crash on same or next click.
4. Keep `lib/panes` and `lib/sampler` in the build (`make -C lib panes sampler`) as **manual Xfred regression** alongside `make test`.

### libXfred unit tests (what we can add)

See **`tests/README.md`** for run instructions. Today `make test` only pulls **`misc.c`** routines from `libXfred.a` (`ToTiff`/`UnTiff`, `is_dir`, `insert_str` in `xgets.c`). Most of `lib/` is X11 widgets and is **not** covered.

| Tier | Needs `DISPLAY`? | Approach | Examples |
|------|------------------|----------|----------|
| **A — pure logic** | No | Add `tests/test_xfred.c`, `extern` the C symbols (no header exports today) | Packbits edge cases; `is_file`; `get_int`/`get_float`/`getbit` on `tmpfile`; `insert_str`; `XfHersheyWidth`; `get_sorted_dir` on a `mkdtemp` tree |
| **B — filesystem** | No | Temp dirs; watch **global state** in `chd.c` (`dn`, `dirs`) — tests must run **serially** or reset globals | `get_sorted_dir` ordering; `complete_dir` / `complete_match` on `"./tmpwfred/fo"` style paths |
| **C — X11 integration** | Yes (`Xvfb` / XQuartz) | Optional `tests/test_xfred_x11.c`, gated on `DISPLAY` or `xvfb-run` in CI | `initx`; `XfCreateVisual` size rules (`width==0` → full widget); **XF-1** solid fill pixel readback; **XF-3** destroy-after-`XfButtonPush`; pixmap not freed when sibling button destroyed (**XF-4**) |
| **D — manual** | Yes | Keep `lib/panes` + pitfall checklist | Colored cells, intro text, click dismiss |

**Good Tier A targets (high value / low effort):**

| Source | Tests |
|--------|--------|
| `misc.c` | `ToTiff`/`UnTiff`: length 0/1, 128-repeat run, literal vs run boundary; `is_file` vs `is_dir` on known paths |
| `xgets.c` | `insert_str` at 0, end, middle (already one case) |
| `hershey.c` | `XfHersheyWidth('M', ROMAN_SIMPLEX) > 0`; space / non-printable; invalid cset index behavior |
| `misc.c` | `get_int`/`get_float` from memory `FILE *` (`fmemopen` or `tmpfile`) with `#` comments and whitespace |

**Tier B notes:** `complete_dir()` calls `sort_dirs()` which **recursively** mutates the path buffer on failure — tests should use dedicated fixture directories under `tests/fixtures/chd/` (e.g. `comp_a/`, `comp_b/`) rather than `/tmp` alone.

**Tier C — map pitfalls to tests:**

| Pitfall | Automated check (with X11) |
|---------|----------------------------|
| XF-1 | Create button + `XfSolidVisual` fill; `XGetImage` samples interior pixel ≠ 0 |
| XF-2 | Compile-time: grep test only uses `XFontStruct *` in `XfCreateVisual` calls (no `Font` fid) |
| XF-3 | ButtonPress callback sets `destroy_pending`; assert no crash; button gone after loop iteration |
| XF-4 | Two buttons, same pixmap pattern, destroy one, `XCopyPlane` on survivor still succeeds |
| XF-5 | Call `set_state(B, -1)` / `maxstate`; expect no-op if guard added to `set_state` / `UpdateButton` |

**Hard to unit-test without refactor (document only):**

- `xgets` / `do_keysym` / `decode_control` — static `anchor` in `xgets.c`
- `HitTest`, `ListIsDoubleClick`, `draw` — need widget structs + font metrics
- `toggle_state` — needs live `Button` + callback
- `confirm.c` / `Composite.c` — layout + expose paths

**Suggested implementation order:** extend Tier A in `test_xfred.c` (≈1 day) → optional `xvfb-run make -C tests check-x11` for Tier C (≈1–2 days) → keep Tier D as manual smoke.

**Linking:** Continue linking `libXfred.a` for Tier A (symbols already in archive). Tier C may link `libXfred.a` + `-lX11` only; no need to pull `pw/*.o`.

---

## Phased implementation

### Phase 0 — Baseline and guardrails (1–2 days)

1. Confirm build: `./configure && make` on target (XQuartz).
2. Document current failure mode (`allocated 0 pixels`, wrong colors, `XCreateImage` depth mismatch).
3. Add `PW_TRUECOLOR` in `config.h` (default **on** for new work; **off** for bisect against legacy path). Document removal criteria (all smoke checks pass on XQuartz + one Linux matrix).
4. Freeze **index encoding specification** (section above) in `display_rgb.h`.
5. Test assets: single-band VICAR (or IMath), 3-band composite, one overlay; optional SPECPR plot path.
6. **Automated tests (no X11):** extend `tests/` with synthetic `sdata` + LUT → expected RGB bytes for `pw_color_at` / `pw_lut_index_to_rgb` (before GUI work).

**Exit criteria:** Spec written; `make test` includes raster golden cases; bisect flag toggles; failure modes documented.

---

### Phase 1 — Display infrastructure (2–3 days)

**Scope:** Prove rasterization in isolation — **not** full `stretch_gray` on XQuartz yet (that still writes `.pixel` until Phase 2).

1. Implement `display_rgb.c`:
   - Query default visual; support depth **24 and 32** using `Visual` red/green/blue masks and `bits_per_rgb` (do not assume `0x00RRGGBB` alone).
   - `XCreateImage` with `bitmap_pad` 32, aligned `bytes_per_line`.
2. Implement `pw_color_at` and `pw_lut_index_to_rgb` per resolution rules.
3. Implement `pw_refresh_image(Image)` for **greyscale base only** (synthetic `sdata` filled with slots `2..`).
4. Unit tests: solid ramps, multi-slot LUT, invalid slot handling.
5. Optional dev hook: minimal window that `XPutImage`s a synthetic buffer (no file load).

**Exit criteria:** `make test` raster tests pass; optional dev `XPutImage` shows correct greyscale ramp on XQuartz; **no** `XAllocColorCells` in new code paths.

**Deferred to Phase 2:** load cube → `stretch_gray` → correct main window (requires index semantics in stretch).

---

### Phase 2 — Index semantics and main image path (4–6 days)

1. **`stretch_gray`**: `sdata[i] = (unsigned char)(2 + j)`.
2. **`stretch_color` / `pseudocolor`**: `out_map[k] = (unsigned char)(2 + k)`; `pseudocolor` `base = 2`; resize lut arrays if needed for slot-indexed inputs.
3. **`overlay`**: `data[…] = (unsigned char)(start + i)`; `quantize(…, 2 + start_offset)`.
4. **`update_display`**: stop aliasing `sdata`; call `pw_refresh_image`; free/recreate `ximage` on `rgb_data`.
5. **`alloc_colors` rewrite**: fixed `NColors`; populate `Colors[i].red/green/blue`; UI via `XAllocColor`.
6. **`IPixmap` / `load_fast_mem`**: depth `pw_display_depth()`; pixmap + scroll paths use RGB `ximage`.
7. **`image_hold` / overlay restore**: refresh after every `sdata` mutation.

**Exit criteria:** Load B/W VICAR (or PNM test file), adjust stretch, change `# colors`; overlays stack with correct slots; no `XStoreColors` for image LUT.

---

### Phase 3 — Color controls and LUT editing (2–4 days)

**Prerequisite:** Phase 0 raster unit tests + Phase 2 overlay/composite golden case.

1. `RGB_CS` / `LoadColorSpread` / `CompositeCS`: LUT in memory only.
2. On LUT change: `pw_refresh_image` (+ pan/mag if visible).
3. Color spread widget: **RGB `CSData`** (preferred) or slot indices + refresh on expose.
4. `color_offsets` in `CompositeCS` → `Image->Colors` then refresh.
5. Fix `MixBox->ext` and similar pixel-pointer UI state.

**Exit criteria:** Drag color spread endpoints; false-color composite updates live; readout swatches track LUT.

**Performance note:** Full-frame refresh on every LUT drag may be too slow for large images — if profiling shows &gt;100 ms, pull **dirty-rectangle refresh** from Phase 6 into this phase.

---

### Phase 4 — Satellite windows (2–3 days)

1. **Magnify**: sample `sdata` slots → RGB `mag_data` / `mag_image`.
2. **Pan** (`create_pan`): downsample indices, rasterize to RGB `pan_image`.
3. **Histogram** (`create_hist`): build RGB plot using UI RGB values, **not** `WHITE(display)` / `pwBlue.pixel` truncated into bytes.
4. **`plot.c`** (if used): migrate 8-bit plot `XImage` or document limitation.

**Exit criteria:** Pan, magnifier, histogram match main image LUT; no depth-8 `XCreateImage` on index buffers.

---

### Phase 5 — Export, cleanup, `sp` (1–2 days)

1. **`write.c`**: replace entire `XQueryColors` / `lcolors[j].pixel = j` path with `pw_color_at`; PPM/raw/XPM from RGB `XImage`.
2. Remove dead code: `MakePrivateColormap`, `PlaneMask`, unused `ColorMap` installs.
3. Delete `pw/image2.c` if still present.
4. **`sp`**: align color init (can slip if external-only and low priority).
5. Update `pw.md`; plan `PW_TRUECOLOR` removal once stable.

**Exit criteria:** Export matches on-screen colors; `sp` builds (run optional).

---

### Phase 6 — Performance and polish (optional → may be required)

1. **Dirty rectangles** on LUT edit (often **required** for interactive color spread on large subsets).
2. **Large images**: chunked `pw_refresh_image`; `MemMode` pixmap partial updates.
3. **HiDPI**: document 1:1 pixel policy.
4. **CI matrix:** manual or scripted smoke on XQuartz + Linux Xorg 24/32 bpp.

---

## Risk register

| Risk | Mitigation |
|------|------------|
| **Spec drift:** local `0..ncolors-1` vs absolute slots | Single spec in `display_rgb.h`; code review checklist; grep for `.pixel` assigned into `sdata` |
| Missed call sites (~50+ in `pw/`) | Grep `sdata`, `\.pixel`, `XCreateImage.*8`; exclude `read.c` |
| `XImage` still aliases `sdata` | `update_display` only calls `pw_refresh_image`; no `XCreateImage` on `sdata` |
| `create_hist` / `CreateCSData` byte truncation | Tier 1/2 file list; explicit smoke for histogram + color spread |
| `write.c` `XQueryColors` | Remove in Phase 5; test export against raster golden |
| Byte order / 32-bit pad byte | Use `Visual` masks; test XQuartz (LSBFirst) + Linux |
| `XPutImage` slow on large cubes | Pixmap cache (`MemMode`); dirty rects |
| Composite `Image->Colors` vs global `Colors` | Document `pw_color_at` rules; unit test 3-band composite + LUT edit |
| Overlay budget overflow | UI validation when `start + ncolors > NColors` |
| `pseudocolor` / `build_table` `base` | Slot-based base (`2`), not pixel |
| Regression on Linux Xorg | 24 and 32 bpp manual matrix |
| Dual `PW_TRUECOLOR` paths linger | Define flag removal in Phase 5 exit criteria |
| Xfred solid/text/destroy bugs (XF-1–XF-8) | [Audit checklist](#libxfred--x11-widget-pitfalls-audit-checklist); [unit test tiers](#libxfred-unit-tests-what-we-can-add); `lib/panes` smoke |
| Custom expose handlers bypass fixed `Button.c` | Tier 3 files (`ColorControls.c`, `plot.c`); verify fill + text locally |

---

## Testing

### Automated (headless, `make test`)

Existing suite: iomedley, `pseudocolor`, `quantize`, color math, packbits (`tests/README.md`).

**Add before / during Phase 1–2:**

- [ ] `pw_color_at` / `pw_lut_index_to_rgb` golden RGB buffers (greyscale ramp, composite LUT, overlay slots)
- [ ] Regression: overlay `map[i]==0` skips color `i`
- [ ] Optional: stretch output slot indices without X11 (mock `Image` + `Colors[]`)

**libXfred (headless, `tests/test_xfred.c`):**

- [x] Packbits edge cases; `is_file`; `get_int` / `get_float` / `getbit`
- [x] `XfHersheyWidth` for known charset
- [x] `get_sorted_dir` on fixture dir under `tests/fixtures/chd/`
- [ ] Optional: `complete_dir` serial tests (globals in `chd.c`)

**libXfred (optional X11 / `check-x11`):**

- [ ] `XfSolidVisual` fill (XF-1 pixel probe)
- [ ] Deferred `XfDestroyButton` (XF-3)
- [ ] Per-button visuals survive sibling destroy (XF-4)

**Add before Phase 3:**

- [ ] 3-band composite + LUT edit → expected RGB sample pixels

Display/GUI remains manual smoke below.

### Manual smoke

- [ ] `lib/panes` — colored cells, readable intro, click-to-dismiss without crash (Xfred regression)
- [ ] App starts without `Cant allocate colorcells`
- [ ] `allocated N pixels` removed or reports software `NColors`
- [ ] Open VICAR B/W, default stretch
- [ ] Change `# colors` in file panel
- [ ] RGB composite from three bands
- [ ] One overlay on B/W base; disable one overlay color via `map`
- [ ] Color spread + scroll stretch
- [ ] Magnify under cursor
- [ ] Pan joystick
- [ ] Histogram mode switch (freq / dist)
- [ ] Write window / magnify export
- [ ] Move focus to another app — no colormap flash

### Stress

- [ ] Image larger than `IWindow` (scroll / subset)
- [ ] `MemMode` pixmap path
- [ ] Max overlays approaching `NColors` budget (expect clear error, not silent failure)
- [ ] Reload file while displayed

---

## Estimated effort

| Phase | Effort (focused) | Notes |
|-------|------------------|-------|
| 0 | 1–2 d | Spec + raster unit tests |
| 1 | 2–3 d | No full stretch E2E yet |
| 2 | 4–6 d | Largest slice; `composite.c` |
| 3 | 2–4 d | May include dirty rects |
| 4 | 2–3 d | Hist/mag/pan/plot |
| 5 | 1–2 d | `write.c`; cleanup |
| 6 | 1–3 d | Often required for LUT UX |
| **Total** | **~13–23 days** | Original 6–10 d optimistic for unknown-codebase port |

---

## Success definition

PicWorks runs on **XQuartz / modern Xorg TrueColor** with:

- Correct greyscale and false-color display
- Working overlays and color-budget UI
- LUT editing without `XStoreColors`
- `sdata` holds **absolute slot indices**; display uses separate RGB buffers
- No dependency on `PseudoColor` or `XAllocColorCells`
- Same file format and scientific workflows as documented in `manual/pw.manual`
- `make test` covers raster/LUT logic; manual checklist covers GUI

---

## References in tree

- Display pipeline: `pw/image.c` (`stretch_*`), `pw/composite.c` (`update_display`, `overlay`)
- Colormap alloc: `pw/pw.c` (`alloc_colors`, `MakePrivateColormap`)
- LUT UI: `pw/ColorControls.c`, `pw/colorspread.c`
- Xfred widgets + pitfalls: `lib/Button.c`, `lib/confirm.c`, `lib/panes.c`; [audit checklist](#libxfred--x11-widget-pitfalls-audit-checklist)
- Tests: `tests/README.md`
- Manual color budget: `manual/pw.manual` (COLOR OVERLAYS section)
- Build / deps: `pw.md`
