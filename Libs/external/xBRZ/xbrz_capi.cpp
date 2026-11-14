// C API for the xBRZ upscaler (OpenJones3D addition, not part of xBRZ)
#include "xbrz_capi.h"
#include "xbrz.h"

extern "C" int xbrz_Scale(int factor, const uint32_t* pSrc, uint32_t* pDst, int srcWidth, int srcHeight, XbrzColorFormat format)
{
    if ( !pSrc || !pDst || srcWidth <= 0 || srcHeight <= 0
        || factor < XBRZ_CAPI_MIN_SCALE || factor > XBRZ_CAPI_MAX_SCALE || factor > xbrz::SCALE_FACTOR_MAX )
    {
        return 0;
    }

    try
    {
        // Unbuffered ARGB avoids the one-time 100-300 ms buffer setup of ColorFormat::argb,
        // which matters more than the scaling speed for a few small dialog images.
        const xbrz::ColorFormat colFmt = format == XBRZ_COLORFORMAT_ARGB
            ? xbrz::ColorFormat::argbUnbuffered
            : xbrz::ColorFormat::rgb;

        xbrz::scale(static_cast<size_t>(factor), pSrc, pDst, srcWidth, srcHeight, colFmt);
        return 1;
    }
    catch ( ... ) // Don't let C++ exceptions (e.g. std::bad_alloc) cross into C code
    {
        return 0;
    }
}

extern "C" int xbrz_BilinearScale(const uint32_t* pSrc, int srcWidth, int srcHeight, uint32_t* pDst, int dstWidth, int dstHeight)
{
    if ( !pSrc || !pDst || srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0 )
    {
        return 0;
    }

    try
    {
        xbrz::bilinearScale(pSrc, srcWidth, srcHeight, pDst, dstWidth, dstHeight);
        return 1;
    }
    catch ( ... )
    {
        return 0;
    }
}
