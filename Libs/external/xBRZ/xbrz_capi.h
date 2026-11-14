#ifndef XBRZ_CAPI_H
#define XBRZ_CAPI_H

// C API for the xBRZ upscaler (OpenJones3D addition, not part of xBRZ)

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XBRZ_CAPI_MIN_SCALE 2
#define XBRZ_CAPI_MAX_SCALE 6

typedef enum eXbrzColorFormat
{
    XBRZ_COLORFORMAT_RGB  = 0, // 0x00RRGGBB, upper 8 bits unused
    XBRZ_COLORFORMAT_ARGB = 1  // 0xAARRGGBB, straight (not premultiplied) alpha; BGRA byte order in memory
} XbrzColorFormat;

/**
 * Scales a 32-bit image by an integer factor with xBRZ.
 *
 * @param factor    - Scale factor, XBRZ_CAPI_MIN_SCALE to XBRZ_CAPI_MAX_SCALE
 * @param pSrc      - Source pixels, srcWidth * srcHeight, rows top-down
 * @param pDst      - Destination pixels, (srcWidth * factor) * (srcHeight * factor)
 * @param srcWidth  - Source width
 * @param srcHeight - Source height
 * @param format    - Pixel format of pSrc and pDst
 * @return 1 on success, 0 on invalid arguments or failure
 */
int xbrz_Scale(int factor, const uint32_t* pSrc, uint32_t* pDst, int srcWidth, int srcHeight, XbrzColorFormat format);

/**
 * Bilinear scale of a 32-bit image to any size (BGRA byte order).
 * @return 1 on success, 0 on invalid arguments or failure
 */
int xbrz_BilinearScale(const uint32_t* pSrc, int srcWidth, int srcHeight, uint32_t* pDst, int dstWidth, int dstHeight);

#ifdef __cplusplus
}
#endif

#endif // XBRZ_CAPI_H
