# libXfred unit tests

Headless tests for pure C helpers in `libXfred.a` (no `DISPLAY` required).

## Run

From the repository root (after `lib/libXfred.a` exists):

```bash
make -C lib check
```

Or as part of the full suite:

```bash
make test
```

`make check` sets `PW64_TEST_FIXTURES` to `lib/tests/fixtures` automatically.

## Coverage (16 tests)

Packbits `ToTiff`/`UnTiff`, `is_dir`/`is_file`, `insert_str`, `get_int`/`get_float`/`getbit`, `XfHersheyWidth`, `get_sorted_dir` on `fixtures/chd/sorted/`.

Widget code still needs manual smoke (`panes`, `sampler`) or future X11 tests under `xvfb-run`.
