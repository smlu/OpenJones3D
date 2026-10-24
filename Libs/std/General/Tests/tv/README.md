These synthetic test vectors exercise std/General file parsers without using
original game assets. The numeric engine constants below are separate from
these synthetic parser inputs.

Binary formats are stored as normal binary files:

- `stdMath/sin_table.bin` and `stdMath/tan_table.bin` preserve the lookup-table
  bytes independently of the compiled decimal literals. See `stdMath/README.md`
  for their source, checksums, and numerical-fixture scope.
- `stdBmp/valid_2x2_24bpp.bmp` is a minimal bottom-up BI_RGB 24 bpp BMP
  based on the public BMP/DIB header layout: 14-byte file header, 40-byte
  BITMAPINFOHEADER, `BM` signature, BGR pixel bytes, and DWORD-aligned rows.
- `stdBmp/bmpsuite_g_rgb24.bmp` is `g/rgb24.bmp` from BMP Suite 2.8, a
  public-domain generated BMP test image. See `stdBmp/README.md`.
- `stdBmp/bmpsuite_*.bmp` are additional public-domain generated BMP Suite
  vectors. See `stdBmp/README.md`.
- `stdConfig/existing_config.json` is a valid synthetic nested configuration
  used to verify typed reads and registry-key mappings.
- `stdConfig/invalid_config.json` contains an intentional trailing comma used
  to verify malformed-file rejection and clean startup retry behavior.
