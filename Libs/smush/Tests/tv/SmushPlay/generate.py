"""Generate authored, asset-free SANM lifecycle fixtures and RGB565 goldens."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import struct


WIDTH = 8
HEIGHT = 8
FRAME_COUNT = 3
FRAME_HEADER_SIZE = 0x416
BL16_HEADER_SIZE = 0x230


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def expected_pixels(frame):
    """Specify pixels directly; no engine or decoder is used to make goldens."""
    colors = (0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000, 0xFFE0, 0xF81F, 0x07FF)
    result = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if frame == 0:
                pixel = colors[(x + y) % len(colors)]
            elif frame == 1:
                red = (3 * x + 5 * y + 1) & 31
                green = (7 * x + 3 * y + 2) & 63
                blue = (5 * x + 7 * y + 3) & 31
                pixel = (red << 11) | (green << 5) | blue
            else:
                pixel = 0
            result.append(pixel)
    return struct.pack('<' + 'H' * len(result), *result)


def chunk(tag, payload):
    assert len(tag) == 4
    return tag + struct.pack('>I', len(payload)) + payload + (b'\0' if len(payload) & 1 else b'')


def make_movie(frames):
    header = bytearray(FRAME_HEADER_SIZE)
    struct.pack_into('<hh', header, 0, 1, FRAME_COUNT)
    struct.pack_into('<i', header, 0x0E, 66666)
    body = chunk(b'SHDR', header)
    body += chunk(b'FLHD', chunk(b'Bl16', struct.pack('<hhhh', 0, WIDTH, HEIGHT, 3)))
    for number, pixels in enumerate(frames):
        frame = bytearray(BL16_HEADER_SIZE)
        struct.pack_into('<h', frame, 0x008, WIDTH)
        struct.pack_into('<h', frame, 0x00C, HEIGHT)
        struct.pack_into('<h', frame, 0x010, number)
        struct.pack_into('<h', frame, 0x228, 3)
        body += chunk(b'FRME', chunk(b'Bl16', frame + pixels))
    return chunk(b'SANM', body)


def gzip_movie(movie):
    output = io.BytesIO()
    with gzip.GzipFile(filename='', mode='wb', fileobj=output, compresslevel=9, mtime=0) as file:
        file.write(movie)
    result = output.getvalue()
    assert result[:10] == bytes.fromhex('1f8b08000000000002ff')
    assert gzip.decompress(result) == movie
    return result


def generated_files():
    frames = [expected_pixels(frame) for frame in range(FRAME_COUNT)]
    movie = make_movie(frames)
    compressed = gzip_movie(movie)
    golden = b''.join(frames)
    assert len(movie) == 3198
    assert movie[-4:] == b'\0' * 4
    assert len(golden) == FRAME_COUNT * WIDTH * HEIGHT * 2
    assert any(frames[0]) and any(frames[1]) and frames[0] != frames[1]
    assert frames[2] == b'\0' * (WIDTH * HEIGHT * 2)

    outputs = {
        'SANM.snm': movie,
        'SANM.san': movie,
        'ZNM.znm': compressed,
        'expected-rgb565.bin': golden,
    }
    metadata = {
        'schema': 1,
        'provenance': 'Authored synthetic inputs and expected pixels; no retail assets, executable or decoder output used.',
        'width': WIDTH,
        'height': HEIGHT,
        'frame_count': FRAME_COUNT,
        'callback_count': FRAME_COUNT + 1,
        'callback_indexes': list(range(FRAME_COUNT + 1)),
        'callback_image_indexes': list(range(FRAME_COUNT)) + [FRAME_COUNT - 1],
        'terminal_state': {'name': 'ANIMPLAY_STATE_END', 'value': 4, 'frame_index': FRAME_COUNT},
        'bitmap': {'layout': 'RGB565', 'byte_order': 'little', 'pixel_size': 2, 'bits_per_pixel': 16, 'pitch': WIDTH * 2},
        'movie': {'version': 1, 'shdr_count': FRAME_COUNT, 'shdr_size': FRAME_HEADER_SIZE,
                  'bl16_header_size': BL16_HEADER_SIZE, 'bl16_codec': 0, 'bl16_bitmap_type': 3,
                  'frame_interval_microseconds': 66666, 'has_audio': False, 'trailing_four_bytes_hex': movie[-4:].hex()},
        'gzip': {'mtime': 0, 'filename': '', 'os_byte': 255, 'compression_level': 9,
                 'uncompressed_sha256': sha256(movie)},
        'expected_frames': [{'index': i, 'byte_count': len(frame), 'sha256': sha256(frame)} for i, frame in enumerate(frames)],
        'files': {name: {'size': len(data), 'sha256': sha256(data)} for name, data in outputs.items()},
    }
    outputs['metadata.json'] = (json.dumps(metadata, indent=2) + '\n').encode('utf-8')
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='verify the committed files without rewriting them')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    for name, data in generated_files().items():
        target = root / name
        if args.check:
            if not target.exists() or target.read_bytes() != data:
                raise SystemExit('Fixture differs: ' + str(target))
        else:
            target.write_bytes(data)
        print('{} {} {}'.format(name, len(data), sha256(data)))


if __name__ == '__main__':
    main()
