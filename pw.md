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

## Build system

- **Autoconf 2.61** — top-level `configure.in` generates `configure`, `config.h`, and Makefiles for the root, `lib/`, `pw/`, and `sp/`.
- **`iomedley/`** is a `AC_CONFIG_SUBDIRS` subtree with its own `configure`; it in turn builds vendored **libjpeg**, **libgif**, **libpng-1.2.3**, and **libtiff** as static archives merged into `libiomedley.a` via `mungelibs`.
- **GNU `config.guess` / `config.sub`** at the repo root (and copies under `iomedley/`) must recognize the host triplet (e.g. `aarch64-apple-darwin24.x` on Apple Silicon). Old 2003-era scripts emit `-apple-darwin…` without a CPU prefix and fail in `config.sub`.

Typical flow:

```bash
./configure
make
```

Binaries: `pw/pw`, `sp/sp` (when built).

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

## Key headers and config

- `config.h` — Autoconf feature macros for the top-level app
- `iom_config.h` — feature macros for iomedley (e.g. `HAVE_LIBPNG`, `HAVE_LIBZ`, endianness)
- `lib/values.h` — local stand-in when system `<values.h>` is absent
- `pw/image.h`, `pw/vicar.h`, `iomedley/iomedley.h` — image cube and format structures

## Install layout

Default prefix `/usr/local`:

- `$(prefix)/bin/pw` (and optionally `sp`)
- Man page generation is stubbed in the top-level `Makefile` install targets

## References

- Original README and FTP distribution notes in `README`
- Tutorial package (historical): `speclab.cr.usgs.gov:pub/pw/pw.manual.tar`
- Contact in README: Noel Gorelick / USGS SPECLAB lineage
