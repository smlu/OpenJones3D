The GOB fixtures are synthetic containers created for unit tests only. They do
not contain original game assets.

- `basic.gob` contains `text/hello.txt` with `Hello\nWorld\n` and
  `data/raw.bin` with `00 01 02 FE FF 5A`.
- `empty.gob` is a valid container with zero directory entries.
- `bad_*.gob` mutate one header or directory field at a time to exercise
  loader rejection paths.
- `garbage.gob` and `missing_*_header.gob` exercise non-GOB and
  truncated-header inputs.
