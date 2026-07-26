These synthetic test vectors exercise std/Win95 file parsers without using
original game assets.

- `stdGob/*.gob` are small GOB containers using the reconstructed GOB header
  and directory-entry layout. The payloads are synthetic text/binary bytes
  created for tests, not original game assets.
- `std3D/dx9` and `std3D/dx6` contain backend-specific 24-bit BMP masks for
  Direct3D system tests. Most are 320x240. The DX6 explicit-mipmap cube, DX9
  automatic-mipmap cube, and combined DX9 mipmap/MSAA/anisotropic cube
  sequences are 640x480. Both explicit and automatically generated mip chains
  have combined DX9 sequences. Magenta pixels are ignored by the comparison
  helpers; other pixels are expected rendered pixels.
- `std3DTestVectorTest.c` audits all rendering BMPs during the deterministic
  `stdTests` run. It verifies the expected 96 DX6, 355 DX9, and one OpenGL BMP,
  including filenames, dimensions, uncompressed 24-bit headers, exact payload
  bounds, nonblank content, and animation-frame progression. Update its
  inventory whenever vectors are intentionally added or removed.
- Each Direct3D backend folder carries the vector set that backend can run. The
  shared cube masks cover vertices, wireframe, white solid faces, textured
  faces, textured faces with vertex color, and solid faces with interpolated RGB
  vertex colors across nine 10-degree rotation frames.
- `std3D/dx9/mipmap_lod_mask.bmp`,
  `std3D/dx9/anisotropic_texture_filter_mask.bmp`,
  `std3D/dx9/anisotropic_texture_filter_cube_mask.bmp`, and
  `std3D/dx9/msaa_triangle_mask.bmp` cover simple feature probes for mip-chain
  LOD selection, DX9 anisotropic textured rendering, and DX9 MSAA rendering.
- `std3D/dx9/mipmap_lod_cube_frame*_mask.bmp`,
  `std3D/dx9/msaa_*_cube_frame*_mask.bmp`, and the corresponding DX6 mipmap
  vectors cover full rotating feature cubes across 32 frames. The sequence
  rotates near the camera, moves backward while rotating, rotates at the far
  distance, then moves forward while rotating. DX9 MSAA cube masks cover
  wireframe, white solid, textured, textured with vertex color, and solid with
  interpolated RGB vertex color. That is 160 MSAA cube masks in DX9, plus the
  simpler triangle MSAA probe. The tests request 16x MSAA and use the highest
  supported fallback from 8x, 4x, or 2x.
- `std3D/dx9/mipmap_msaa_anisotropic_cube_frame*_mask.bmp` contains the
  32-frame 640x480 DX9 sequence that combines explicit mip levels, trilinear
  mip filtering when supported, anisotropic minification, and the selected
  MSAA level.
- `std3D/dx9/mipmap_autogen_cube_frame*_mask.bmp` contains the 32-frame
  640x480 DX9 sequence generated from a single 512x512 source level using
  `D3DUSAGE_AUTOGENMIPMAP`.
- `std3D/dx9/mipmap_autogen_msaa_anisotropic_cube_frame*_mask.bmp` contains
  the 32-frame 640x480 DX9 sequence that combines automatic mipmap generation,
  anisotropic minification, and the selected MSAA level.
- `std3D/dx6/mipmap_lod_mask.bmp` and
  `std3D/dx6/mipmap_lod_cube_frame*_mask.bmp` cover DX6 mip-chain LOD
  selection. The rotating cube sequence is 640x480; the simple probe remains
  320x240. DX6 does not have MSAA or the DX9 anisotropic filtering vectors.
- The mipmap LOD cube vectors use an explicit UV-grid-derived mip chain so the
  full texture is projected once onto each visible cube face in both DX6 and
  DX9 runs. DX9 can generate mipmaps at runtime, but these vectors keep the
  levels explicit for stable cross-backend mask comparisons. Each child level
  is generated from its parent with a deterministic 2x2 box filter. DX9 uses
  all ten levels from 512x512 through 1x1; DX6 uses all nine levels from
  256x256 through 1x1.
- `std3D/dx9/uv_grid_directx.bmp` is a 512x512 conversion of the Three.js
  DirectX UV-grid texture. `std3D/dx6/uv_grid_directx.bmp` is a 256x256
  conversion for Direct3D 6 devices that report the original 256 texture limit.
  `std3D/opengl/uv_grid_opengl.bmp` is kept for the future OpenGL backend test
  vectors. Their MIT license notice is kept next to the std3D assets.
- Set `JONES3D_SYSTEM_TEST_UPDATE_VECTORS=1` only for an intentional golden
  update, then rerun without it to validate the saved BMPs normally.
