// Added: Opaque platform handle mapping. Inside Windows code the opaque handle types are the real ones, so that code needs no casts.
// Include this first, before any other header.
#ifndef STD_STDSYSTAGSWIN32_H
#define STD_STDSYSTAGSWIN32_H

#define sSysWindow   HWND__
#define sSysInstance HINSTANCE__
#define sSysDrawContext HDC__
#define sSysBitmap HBITMAP__

#endif // STD_STDSYSTAGSWIN32_H
