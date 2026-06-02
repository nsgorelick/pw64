# PicWorks (pw)

PicWorks is an X11-based scientific raster image viewer (version 5.013a, 1999). It was built for fast visualization of multispectral and geospatial imagery, especially cubes with VICAR headers, but supports many other uncompressed raster formats through its I/O layer.

## Architecture

The tree is organized as a small Autoconf project with several compile-time subprojects:

| Directory | Role |
|-----------|------|
| `lib/` | **Xfred** — custom X11 widget library (`libXfred.a`): buttons, sliders, joysticks, lists, colormaps, Hershey text, etc. |
| `pw/` | Main application (`pw`) — image display, color controls, compositing, magnify, histograms, plotting hooks |
| `sp/` | External **SPECPR** file browser (`sp`) — used by pw’s plot tool to pick wavelengths and library spectra |
| `iomedley/` | **I/O medley** (`libiomedley.a`) — format readers/writers (VICAR, ISIS, ENVI, PNM, JPEG, PNG, TIFF, GIF, BMP, raw, …) |
| `bitmaps/` | X bitmap resources for the UI |

The canonical X11 entry point for application code is `lib/Xfred.h`, which pulls in the widget headers and Xlib. A wrapper at the repo root includes that header via `#include "lib/Xfred.h"`.

Configure enables an **internal SPECPR reader** by default (`--enable-specpr` → `INTERNAL_SP`), so only `pw` is installed. With `--disable-specpr`, both `pw` and `sp` are built and installed.

## Display model (8-bit colormap)

PicWorks targets **indexed color on an 8-bit PseudoColor display**, not direct RGB blitting. The manual (`manual/pw.manual`) assumes ~256 display levels shared among the base image, overlays, and UI chrome.

Pipeline in brief:

1. Cube samples live in `Image->data` (often 16-bit); stretching produces `Image->sdata`.
2. `sdata` bytes are **X colormap pixel IDs** (from `XAllocColorCells`), not simple indices 0…255.
3. `XStoreColors` updates hardware LUT entries for false-color and per-image palettes (`pw/ColorControls.c`, `pw/colorspread.c`).
4. The main view is an 8-bit `ZPixmap` `XImage` (`pw/composite.c` `update_display()`), shown with `XPutImage` on `IWindow`.

Startup (`pw/pw.c` `alloc_colors()`) grabs a contiguous block of color cells; on failure it installs a **private colormap** (`MakePrivateColormap()`). Overlays partition the same cell budget across the greyscale image and up to 15 overlay planes (`manual/pw.manual`, COLOR OVERLAYS).

Key symbols: global `Colors[]`, `NColors`, `ColorMap`, `PlaneMask` (highlight uses a second plane bit on PseudoColor hardware).

### Modern X11 (XQuartz and typical Linux)

Contemporary servers default to **TrueColor 24/32-bit**. They often still list **depth 8** under supported pixmap formats, but that only means 8-bit pixmaps are representable—not that a **PseudoColor** window visual exists.

On a typical XQuartz setup:

- Root/default visual: **TrueColor, 24 planes**
- All visuals: **TrueColor** (no `PseudoColor`)
- `xdpyinfo` may show `depth 8` in the pixmap-format list while **no** `class: PseudoColor` line appears

Implications for the current code:

| Mechanism | On TrueColor default |
|-----------|----------------------|
| `XAllocColorCells` | Fails or is meaningless |
| `XStoreColors` for image LUT | Does not drive what you see |
| 8-bit `XCreateImage` + `DefaultDepth()` | Depth/visual mismatch |
| Private colormap fallback | No real indexed visual to attach to |

Symptoms: `Cant allocate colorcells`, very small `allocated N pixels`, wrong or blank image, or unstable colors. **Building and linking is not enough**—the display path must be ported.

**Planned fix:** TrueColor display port (software LUT → 24/32-bit RGB, then `XPutImage`). Phased design, file list, and tests are in [`plan.md`](plan.md). Until that lands, do not expect a correct GUI on XQuartz despite a successful `make`.

## Build system

- **Autoconf 2.61** — top-level `configure.in` generates `configure`, `config.h`, and Makefiles for the root, `lib/`, `pw/`, and `sp/`.
- **`iomedley/`** is a `AC_CONFIG_SUBDIRS` subtree with its own `configure`; it in turn builds vendored **libjpeg**, **libgif**, **libpng-1.2.3**, and **libtiff** as static archives merged into `libiomedley.a` via `mungelibs`.
- **GNU `config.guess` / `config.sub`** at the repo root (and copies under `iomedley/`) must recognize the host triplet (e.g. `aarch64-apple-darwin24.x` on Apple Silicon). Old 2003-era scripts emit `-apple-darwin…` without a CPU prefix and fail in `config.sub`.

Typical flow:

```bash
./configure
make
```

Binaries: `pw/pw`, `sp/sp` (when built). These paths are **generated** and not in git (see Version control below).

`configure` produces `Makefile`, `config.h`, and `config.status` at the top level; `iomedley/configure` produces `iomedley/iom_config.h` and sub-makes for vendored codecs. A clean tree requires running `./configure` before `make`.

### Configure options (selected)

| Option | Default | Effect |
|--------|---------|--------|
| `--prefix` | `/usr/local` | Install prefix |
| `--with-xpm` / `--without-xpm` | with | libXpm for XPM images |
| `--with-magick` / `--without-magick` | with | ImageMagick 5.4.2 (optional; often absent) |
| `--enable-specpr` / `--disable-specpr` | internal SP | Build only `pw` vs. `pw` + `sp` |
| `--x-includes=DIR` `--x-libraries=DIR` | auto | X11 headers/libs if not auto-detected |

Configure records X paths as `XINCLUDES` and `XLIBS` substitutions. Those must appear in **each** component Makefile (`lib/`, `pw/`, `sp/`) — the top-level `Makefile` alone is not enough for sub-makes.

### macOS / XQuartz

On Darwin, X11 comes from **XQuartz**, usually under `/opt/X11`. Configure can probe `/opt/X11/include` and `/opt/X11/lib` when standard `xmkmf` discovery fails.

Expected link libraries for `pw` (from configure): `X11`, `Xt`, `Xext`, `Xpm` (if found), `m`, `z`, plus `libXfred` and `libiomedley`. **Motif (`Xm`)** is probed but not required for a successful configure on macOS.

`PATH` should include `/opt/X11/bin` for `xmkmf` and runtime. `DISPLAY` must be set to use the GUI.

To inspect the server (useful before debugging color problems):

```bash
xdpyinfo | grep -E "default visual|depth of root|class:|depth:"
xdpyinfo | awk '/visual id:/{v=$0} /class:/{c=$0} /depth:/{d=$0} d ~ /8 planes/{print v,c,d}'
```

If the second command prints nothing, there is no 8-bit window visual—only pixmap depth support.

### Compiler notes (modern Clang / GCC)

Much of the code is late-1980s / 1990s K&R C. On current toolchains:

- **`pw/`** and **`sp/`** use `-std=gnu89` and selective `-Wno-*` flags for legacy patterns (missing return types, old-style function definitions, callback prototypes).
- **`iomedley/`** and vendored codecs needed small header/prototype updates for C99+ (e.g. `stdlib.h`, `string.h`, `qsort` comparators).
- **`lib/`** is mostly standard C with X11 includes via `CPPFLAGS` from configure.

## Dependencies

**Required**

- C compiler, `make`, `ranlib`
- X11 development files (XQuartz on macOS): `libX11`, `libXt`, `libXext`
- `zlib` (`libz`)

**Optional at configure time**

- **libXpm** — XPM pixmap support
- **ImageMagick 5.4.2** — many extra formats via `Magick-config` (legacy; rarely installed today)
- **libXm** — Motif (checked; not required for the build tested on macOS)

**Bundled (no system install needed)**

- libjpeg, libgif, libpng 1.2.3, libtiff (inside `iomedley/`)

## I/O and formats

`iomedley` centralizes format detection and loading in `iom_LoadHeader()` and related `iom_Get*Header()` routines declared in `iomedley/iomedley.h`. Supported paths include VICAR, ISIS, ENVI, AVIRIS, GRD, GOES, IMath, PNM, JPEG, PNG, TIFF, GIF, BMP, ERS, and raw data.

Without ImageMagick, configure still succeeds; format coverage depends on the bundled libraries and native readers in `iomedley/io_*.c`.

## SPECPR workflow

- **`sp`** — standalone SPECPR spectrum/file requestor.
- **`pw` plot** — can invoke `sp` externally, or use the compiled-in reader when `INTERNAL_SP` is defined (default in this tree).

SPECPR-related code lives in `pw/specpr.c`, `pw/sp.c`, and `sp/`.

## Version control

The git tree tracks **source and build templates** only (see `.gitignore`). Not checked in:

- `backup/` (removed from history; keep locally only if needed)
- Subversion `.svn/` metadata (was erroneously committed earlier)
- Build products: `*.o`, `*.a`, `libiomedley.a`, generated `Makefile`, `config.log`, `config.status`, `config.h`, `iom_config.h`
- Programs: `pw/pw`, `sp/sp`, libtiff utility binaries under `iomedley/libtiff/tools/`

After clone: `./configure && make` rebuilds everything.

## Key headers and config

- `config.h` — generated by `./configure` (template: `config.h.in` if present)
- `iom_config.h` — generated under `iomedley/` by its `configure` (template: `iom_config.h.in`)
- `lib/values.h` — local stand-in when system `<values.h>` is absent
- `pw/image.h`, `pw/vicar.h`, `iomedley/iomedley.h` — image cube and format structures

## Install layout

Default prefix `/usr/local`:

- `$(prefix)/bin/pw` (and optionally `sp`)
- Man page generation is stubbed in the top-level `Makefile` install targets

## Tests

Headless unit tests live under `tests/`. After `./configure && make`, run:

```bash
make test
```

See `tests/README.md` for scope and gaps. They cover **iomedley** I/O helpers, **pseudocolor** / **quantize**, **color** math, and **lib** packbits utilities—not the X11 display path.

## References

- Original README and FTP distribution notes in `README`
- Tutorial package (historical): `speclab.cr.usgs.gov:pub/pw/pw.manual.tar`
- Contact in README: Noel Gorelick / USGS SPECLAB lineage
- [`plan.md`](plan.md) — TrueColor port plan (display path for modern X11)
- Upstream clone: https://github.com/nsgorelick/pw64
