#ifndef STD_PIXEL_FORMAT_GL_H
#define STD_PIXEL_FORMAT_GL_H

#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START
static const tSysPixelFormat stdPixelFormatGL_RGB555 =
{
    .glInternalFormat = GL_RGB5_A1,
    .glFormat = GL_BGRA,
    .glType = GL_UNSIGNED_SHORT_1_5_5_5_REV
};

static const tSysPixelFormat stdPixelFormatGL_RGB565 =
{
    .glInternalFormat = GL_RGB16,
    .glFormat = GL_BGR,
    .glType = GL_UNSIGNED_SHORT_5_6_5_REV
};

static const tSysPixelFormat stdPixelFormatGL_BGR888 =
{
    .glInternalFormat = GL_RGB,
    .glFormat = GL_RGB,
    .glType = GL_UNSIGNED_BYTE
};

static const tSysPixelFormat stdPixelFormatGL_RGB888 =
{
    .glInternalFormat = GL_RGB8,
    .glFormat = GL_BGR,
    .glType = GL_UNSIGNED_BYTE
};

static const tSysPixelFormat stdPixelFormatGL_BGR8888 = {
    .glInternalFormat = GL_RGBA8,
    .glFormat = GL_RGBA,
    .glType = GL_UNSIGNED_INT_8_8_8_8
};

static const tSysPixelFormat stdPixelFormatGL_RGB8888 =
{
    .glInternalFormat = GL_RGBA8,
    .glFormat = GL_BGRA,
    .glType = GL_UNSIGNED_INT_8_8_8_8_REV
};

static const tSysPixelFormat stdPixelFormalGL_ARGB4444 =
{
    .glInternalFormat = GL_RGBA4,
    .glFormat = GL_BGRA,
    .glType = GL_UNSIGNED_SHORT_4_4_4_4_REV
};

static const tSysPixelFormat stdPixelFormalGL_ARGB5551 =
{
    .glInternalFormat = GL_RGB5_A1,
    .glFormat = GL_BGRA,
    .glType = GL_UNSIGNED_SHORT_1_5_5_5_REV
};

static const tSysPixelFormat stdPixelFormatGL_ABGR8888 =
{
    .glInternalFormat = GL_RGBA8,
    .glFormat = GL_RGBA,
    .glType = GL_UNSIGNED_INT_8_8_8_8
};

static const tSysPixelFormat stdPixelFormatGL_ARGB8888 =
{
    .glInternalFormat = GL_RGBA8,
    .glFormat = GL_BGRA,
    .glType = GL_UNSIGNED_INT_8_8_8_8_REV
};

static const tSysPixelFormat stdPixelFormatGL_RGBA8888 =
{
    .glInternalFormat = GL_RGBA8,
    .glFormat = GL_RGBA,
    .glType = GL_UNSIGNED_INT_8_8_8_8_REV
};

J3D_EXTERN_C_END

#endif
