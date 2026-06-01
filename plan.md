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
| `sdata` | `unsigned char` **X pixel values** | `unsigned char` **palette indices** `0 … ncolors-1` |
| `Colors[0..1]` | reserved black/white (pixel cells) | reserved LUT entries (RGB only) |
| `Colors[2 .. 2+ncolors-1]` | image/overlay LUT + pixel cells | LUT RGB; `.pixel` unused for images |
| `Image->Colors[]` | per-composite LUT + `.pixel` | per-image LUT RGB only |
| Display buffer | 8-bit `XImage` pointing at `sdata` | **24/32-bit RGB** buffer (`rgb_data`) + `XImage` |
| Palette update | `XStoreColors` | update LUT in memory → **re-rasterize** `rgb_data` (full or dirty rects) |

### New display helper module

Add `pw/display_rgb.c` + `pw/display_rgb.h` (names tentative):

- `void pw_init_display(Display *d, int screen);` — cache visual, depth, masks, bytes per pixel.
- `int pw_display_depth(void);` — 24 or 32.
- `void pw_lut_index_to_rgb(const XColor *lut, int base, int ncolors, const unsigned char *indices, unsigned char *rgb, int npixels);`
- `XImage *pw_create_rgb_image(Display *d, unsigned char *rgb_data, int w, int h);`
- `void pw_free_rgb_image(XImage *img);` — free image and its buffer if owned.
- `void pw_refresh_image(Image img);` — indices + global/image LUT → `rgb_data` → optional pixmap.

Centralizing this avoids duplicating byte-order / pad logic across `composite.c`, `mag.c`, pan, and histogram code.

### `NColors` without `XAllocColorCells`

Replace `alloc_colors()` cell allocation with a **fixed software budget**:

- Default `NColors = 220` (or 253), configurable later; reserve `Colors[0]` black, `Colors[1]` white for UI conventions.
- Image/overlay slots: `Colors[2]` … `Colors[NColors+1]`.
- UI chrome colors (`pwRed`, `pwBackground`, …): allocate once via `XAllocColor` on the **default TrueColor** colormap (works on modern X).
- Remove `PlaneMask` / `pwHilite.pixel = pixel | PlaneMask[0]` trick; use a distinct allocated highlight color.
- Remove or no-op `MakePrivateColormap()` (keep stub only if needed for debugging).

---

## Architecture diagram

```mermaid
flowchart TB
  subgraph data [Data plane - unchanged]
    D[data 16-bit cube]
  end
  subgraph index [Index plane - semantic change]
    S[sdata palette indices]
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
  L --> pw_lut_index_to_rgb
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
| `pw/pw.c` | Replace `alloc_colors()`; drop 8-bit `IPixmap`; `IImage` uses RGB path; simplify colormap install |
| `pw/composite.c` | `update_display()`, `load_fast_mem()`, `overlay()`, `create_pan()`, `make_hist_image()` (hist builder); use `display_rgb` |
| `pw/image.c` | `stretch_gray`, `stretch_color`: write indices not `.pixel`; stop assigning `new->Colors[i].pixel` |
| `pw/pseudo.c` | `make_composite`, `out_map`, `base` param: indices 0..n-1 not pixel values |
| `pw/quant.c` | `quantize(..., base)`: base = 0 |
| `pw/colorspread.c` | `RGB_CS`: no `XStoreColors`; `CreateCSData`: indices; build RGB spread for widget |
| `pw/ColorControls.c` | `CompositeCS`: LUT-only update + `pw_refresh_image`; color spread widget RGB |
| `pw/setcolor.h` | Split: `setcolor_lut()` (memory only) vs `setcolor_x11()` (UI pixels) |
| `pw/display_rgb.c` | **new** — rasterization and `XCreateImage` |
| `pw/image.h` | Add `unsigned char *rgb_data;` (or embed in `ximage->data` with clear ownership) |
| `pw/Makefile` | Add `display_rgb.o` |

### Tier 2 — satellite views (same raster path)

| File | Changes |
|------|---------|
| `pw/mag.c` | Magnifier buffer: RGB `XImage`; `create_mag` reads indices from `sdata` |
| `pw/buttons.c` | Pan/histogram widgets: refresh via RGB images |
| `pw/write.c` | Export: map index → RGB via `Colors[]`, not `XQueryColors` on hardware cmap |

### Tier 3 — minor / verify

| File | Notes |
|------|-------|
| `pw/read.c`, `pw/readout.c` | Ensure nothing assumes `sdata` == pixel value |
| `pw/plot.c`, `pw/rose.c`, `pw/block.c`, `pw/hist.c` | UI colors only; verify `pwHilite` |
| `pw/restart.c` | If session restore touches colormap |
| `lib/xf.c` | `XfColor()` already uses `XAllocColor` — OK on TrueColor |
| `sp/sp.c` | Drop `PlaneMask` / color-cell alloc if present; SPECPR browser is mostly UI |

### Unchanged

- `iomedley/**` — file I/O only (internal 8-bit BMP colormaps are format semantics, not X11).
- `lib/Xfred*.c` — widgets use pixel values for GC foreground; allocate UI colors normally.

---

## Phased implementation

### Phase 0 — Baseline and guardrails (0.5–1 day)

1. Confirm build: `./configure && make` on target (XQuartz).
2. Document current failure mode (e.g. `allocated 0 pixels`, wrong colors, crash in `XCreateImage`).
3. Add compile-time flag `PW_TRUECOLOR` in `config.h` (default **on** for new builds; optional off for bisect).
4. Create minimal test assets list: single-band VICAR, 3-band composite, one overlay, SPECPR plot path.

**Exit criteria:** Reproducible “before” behavior documented; flag toggles port code paths.

---

### Phase 1 — Display infrastructure (1–2 days)

1. Implement `display_rgb.c`:
   - Query default visual; support depth 24 and 32.
   - RGB packing: use visual red/green/blue masks (`XMatchVisualInfo` / `Visual`) or portable 0x00RRGGBB in 32-bit LE first.
   - `XCreateImage` with `bitmap_pad` 32, `bytes_per_line` aligned.
2. Implement `pw_refresh_image(Image)`:
   - Input: `sdata`, `ncolors`, `Colors` or `image->Colors`.
   - Output: (re)allocate `rgb_data`, fill `ximage`.
3. Wire **no UI** smoke test: load cube → stretch_gray → single `XPutImage` to `IWindow`.

**Exit criteria:** Greyscale image visible on XQuartz; no `XAllocColorCells` calls in this path.

---

### Phase 2 — Index semantics in stretch/composite (2–3 days)

1. **`stretch_gray`**: `sdata[i] = (unsigned char)j` (index), not `Colors[j+2].pixel`.
2. **`stretch_color` / `pseudocolor`**: output indices; `out_map[i] = i`; remove `base` pixel offset.
3. **`overlay`**: `data[...] = (unsigned char)(start + i)` slot index, not `Colors[...].pixel`.
4. **`quantize`**: `base = 0`.
5. **`update_display`**: build RGB from combined LUT (global `Colors` for slots 2+, per-image `Image->Colors` for composites).
6. **`alloc_colors` rewrite**: fixed `NColors`; UI colors via `XAllocColor`; populate `Colors[i].red/green/blue` for slots 0..NColors+1.
7. Fix `IPixmap` / `load_fast_mem` depth to `pw_display_depth()`.

**Exit criteria:** Load B/W image, adjust stretch, change color count; panes update; overlays stack with correct slots.

---

### Phase 3 — Color controls and LUT editing (1–2 days)

1. `RGB_CS` / `LoadColorSpread` / `CompositeCS`: update LUT in memory only.
2. On LUT change: `pw_refresh_image` for active image (and pan/histogram if bound).
3. Color spread widget (`CSData`): either small RGB buffer or 8-bit indices + refresh on expose (prefer RGB for consistency).
4. `color_offsets` in `CompositeCS`: apply to `Image->Colors` then refresh.

**Exit criteria:** Drag color spread endpoints; false-color composite updates live; readout/histogram colors track LUT.

---

### Phase 4 — Satellite windows (1 day)

1. **Magnify**: index sampling from `sdata` → magnified RGB tile.
2. **Pan** (`create_pan`): copy indices, rasterize to small RGB `pan_image`.
3. **Histogram** (`make_hist_image`): build RGB plot (replace `WHITE(display)` / `pwBlue.pixel` in buffer with RGB values from UI colors).

**Exit criteria:** Pan joystick, magnifier, and histogram widgets match main image LUT.

---

### Phase 5 — Export, cleanup, `sp` (0.5–1 day)

1. **`write.c`**: `table[index] = channel from Colors[lut_index].red` (merge global + per-image LUT rules).
2. Remove dead code: `MakePrivateColormap`, `PlaneMask`, unused `ColorMap` global installs on image window if not needed.
3. **`sp`**: align color init with `pw` if shared patterns exist.
4. Update `pw.md` with TrueColor assumptions; note `PW_TRUECOLOR` removed once stable.

**Exit criteria:** PPM/raw export matches screen; `sp` builds and runs.

---

### Phase 6 — Performance and polish (optional)

1. **Dirty rectangles**: on LUT change, only re-rasterize exposed region if profiling shows need.
2. **Large images**: consider chunked `pw_refresh_image` or retain pixmap + partial updates for `MemMode` 1/2.
3. **HiDPI**: document that PW is 1:1 pixel (no scaling); future work if needed.

---

## LUT composition rules (document in code)

Overlays partition the index space:

- Slots `0–1`: reserved (black/white).
- Base image uses `ncolors` slots starting at `2`.
- Each overlay adds its `ncolors` at the next free slot (existing logic in `composite.c` / manual).

`pw_refresh_image` must resolve:

```c
/* index v in sdata → RGB */
slot = v;  /* v is already absolute slot index 2.. */
use global Colors[slot] or composite Image->Colors[slot-2] per image type */
```

Add a small helper `pw_color_at(Image img, unsigned char slot, XColor *out)` used by rasterizer and export.

---

## Risk register

| Risk | Mitigation |
|------|------------|
| `sdata` still treated as pixel in missed call sites | Grep for `.pixel` stores into `sdata`; code review checklist |
| Byte order on 24bpp XImage | Use `ImageByteOrder(display)`; test on XQuartz (LSBFirst) |
| `XPutImage` slow on large cubes | Keep pixmap cache (`MemMode`); profile before optimizing |
| Composite `Image->Colors` vs global `Colors` mismatch | Unit-style test: 3-band composite + LUT edit |
| XPM write (`write.c`) | Pass RGB `XImage`; verify `HAVE_XPM` path |
| Regression on Linux Xorg | CI or manual test matrix: 24 and 32 bpp |

---

## Testing checklist

### Smoke

- [ ] App starts without `Cant allocate colorcells`
- [ ] `printf("allocated %d pixels")` removed or reports software `NColors`
- [ ] Open VICAR B/W, default stretch
- [ ] Change `# colors` in file panel
- [ ] RGB composite from three bands
- [ ] One overlay on B/W base
- [ ] Color spread + scroll stretch
- [ ] Magnify under cursor
- [ ] Pan joystick
- [ ] Histogram mode switch (freq / dist)
- [ ] Write window / magnify export
- [ ] Move focus to another app — no colormap flash (TrueColor benefit)

### Stress

- [ ] Image larger than `IWindow` (scroll / subset)
- [ ] `MemMode` pixmap path (medium/high memory settings if still exposed)
- [ ] Max overlays approaching `NColors` budget
- [ ] Reload file while displayed

---

## Estimated effort

| Phase | Effort |
|-------|--------|
| 0 | 0.5–1 d |
| 1 | 1–2 d |
| 2 | 2–3 d |
| 3 | 1–2 d |
| 4 | 1 d |
| 5 | 0.5–1 d |
| 6 | optional |
| **Total** | **~6–10 days** focused work |

---

## Success definition

PicWorks runs on **XQuartz / modern Xorg TrueColor** with:

- Correct greyscale and false-color display
- Working overlays and color-budget UI
- LUT editing without `XStoreColors`
- No dependency on `PseudoColor` or `XAllocColorCells`
- Same file format and scientific workflows as documented in `manual/pw.manual`

---

## References in tree

- Display pipeline: `pw/image.c` (`stretch_*`), `pw/composite.c` (`update_display`, `overlay`)
- Colormap alloc: `pw/pw.c` (`alloc_colors`, `MakePrivateColormap`)
- LUT UI: `pw/ColorControls.c`, `pw/colorspread.c`
- Manual color budget: `manual/pw.manual` (COLOR OVERLAYS section)
- Build / deps: `pw.md`
