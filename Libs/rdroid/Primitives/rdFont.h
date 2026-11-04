#ifndef RDROID_RDFONT_H
#define RDROID_RDFONT_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

#include <rdroid/Math/rdVector.h>

#include <std/types.h>

J3D_EXTERN_C_START

#define RDFONT_ALIGNCENTER 1
#define RDFONT_ALIGNLEFT   2
#define RDFONT_ALIGNRIGHT  4

typedef rdVector4 rdFontColor[4];

void rdFont_Startup(void);
void rdFont_Shutdown(void);
int rdFont_Open(void);
void rdFont_Close(void);

rdFont* J3DAPI rdFont_Load(const char* pFilename);
int J3DAPI rdFont_LoadEntry(const char* pFilename, rdFont* pFont);
int J3DAPI rdFont_LoadMaterial(const char* pFilename, rdFont* pFont);

int J3DAPI rdFont_Write(const char* pFilename, const rdFont* pFont, tVBuffer*** paTextures); // Added
int J3DAPI rdFont_WriteEntry(const char* pFilename, const rdFont* pFont); // Added
int J3DAPI rdFont_WriteMaterial(const char* pFilename, const rdMaterial* pMat, tVBuffer*** paTextures); // Added

void J3DAPI rdFont_Free(rdFont* pFont);

void J3DAPI rdFont_SetFontColorDiffuse(const rdFontColor apColor); // Added
void J3DAPI rdFont_SetFontColor(const rdFontColor apColor);
rdFontColor* rdFont_DuplicateFontColor(void); // Added

// Returns normalized X in screen pixels that can be used in draw line funcs
static inline float rdFont_GetNormX(float x)
{
    return (x / RD_REF_WIDTH);
}

// Returns normalized Y in screen pixels that can be used in draw line funcs
static inline float rdFont_GetNormY(float y)
{
    return (y / RD_REF_HEIGHT);
}

static inline float rdFont_GetNormLineSpacing(const rdFont* pFont)
{
    return rdFont_GetNormY((float)pFont->lineSpacing);
}

/**
 * @brief Returns the pointer to the end of the last word in the text that can still fit within the screen width.
 *
 * The function calculates the line's end by measuring the width of each character using the provided font.
 * It continues until the accumulated width plus the next character exceeds the screen width (calculated as RD_REF_WIDTH * widthScale).
 * The pointer returned points to the end of the last word that still fits within these bounds.
 *
 * @param pText - Pointer to the input text string to be wrapped.
 * @param pFont - Pointer to the rdFont structure that provides the font metrics for character widths.
 * @param widthScale - Scaling factor applied to RD_REF_WIDTH pixels to determine the line's maximum width.
 *
 *
 * @return
 * - A pointer to the end of the last word that fits within the width (RD_REF_WIDTH * widthScale).
 * - `pText` if there is a single long word without spaces.
 * - `NULL` if the input `pText` is of size 0 or the whole text can fit onto the screen width.
 */
const char* J3DAPI rdFont_GetWrapLine(const char* pText, const rdFont* pFont, float widthScale);

/**
 * @brief Finds the wrap point in a text string based on available width and optional font size scaling.
 *
 * This function determines where to break a line of text to fit within a specified width.
 * It attempts to break at the last space character before exceeding the width limit.
 * The width calculation can be scaled by specifying a custom font size.
 *
 * @param pText - Pointer to the input text string.
 * @param pFont - Pointer to the rdFont structure that defines the font.
 * @param widthScale - Scale factor for the available width (relative to RD_REF_WIDTH).
 * @param fontSizePt - The desired font size in points. Pass pFont->fontSize for original size.
 * @return Pointer to the character after the wrap point, or NULL if no wrapping is needed.
 */
const char* J3DAPI rdFont_GetWrapLineEx(const char* pText, const rdFont* pFont, float widthScale, float fontSizePt); // New

/**
 * @brief Calculates the total width of the given text based on the provided font metrics.
 *
 * This function computes the total width of a string of text by summing up the width of each character
 * according to the font specified by the rdFont structure.
 *
 * @param pText Pointer to the input text string whose width is to be calculated.
 * @param pFont Pointer to the rdFont structure that provides the font metrics for character widths.
 *
 * @return The total width of the text, in pixels.
 */
size_t J3DAPI rdFont_GetTextWidth(const char* pText, const rdFont* pFont);

/**
 * @brief Calculates the width of a text string in reference resolution pixels with optional font size scaling.
 *
 * This function calculates the total width that the given text would occupy when rendered,
 * measured in reference resolution pixels (640x480 coordinate space). The width can be scaled
 * by specifying a custom font size.
 *
 * @param pText - Pointer to the input text string.
 * @param pFont - Pointer to the rdFont structure that defines the font.
 * @param fontSizePt - The desired font size in points. Pass pFont->fontSize for original size.
 * @return The total width of the text in reference resolution pixels, or 0 if pFont or pText is NULL.
 */
size_t J3DAPI rdFont_GetTextWidthEx(const char* pText, const rdFont* pFont, float fontSizePt); // New

/**
 * @brief Draws a line of text to screen at a normalized screen position using the specified font, stopping when the screen width is exceeded.
 *
 * The text is drawn starting at the normalized coordinates (x, y), where both x and y are within the range [0.0, 1.0], representing
 * the relative position on the screen. The z parameter represents the NDC (Normalized Device Coordinates) z-depth value.
 * The text will be drawn until it reaches the end of the screen width, and any remaining characters after that point will not be drawn.
 *
 * @param pText -  Pointer to the input text string to be drawn.
 * @param x - Normalized X position in the range [0.0, 1.0], representing horizontal placement on the screen.
 * @param y - Normalized Y position in the range [0.0, 1.0], representing vertical placement on the screen.
 * @param z - NDC Z-depth value for positioning the text in depth.
 * @param pFont - Pointer to the rdFont structure that defines the font for the text rendering.
 * @param alignFlags -  Flags for alignment, defining how the text should be aligned (RDFONT_ALIGN*).
 */
void J3DAPI rdFont_DrawTextLine(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags);

/**
 * @brief Draws a line of text to screen at a normalized screen position with custom font size scaling.
 *
 * This function behaves similarly to `rdFont_DrawTextLine`, except it additionally allows specifying
 * a custom font size in points, enabling the text to be rendered larger or smaller than the font's original size.
 * The font is scaled proportionally based on the ratio between the requested size and the font's base size.
 *
 * Unlike the clipped variant, this function does not perform Y-axis clipping and will attempt to draw
 * text even if it extends beyond the screen boundaries. However, it does perform basic X-axis clipping
 * to avoid drawing characters that are completely off-screen horizontally.
 *
 * @param pText - Pointer to the input text string to be drawn.
 * @param x - Normalized X position in the range [0.0, 1.0], representing horizontal placement on the screen.
 * @param y - Normalized Y position in the range [0.0, 1.0], representing vertical placement on the screen.
 * @param z - NDC Z-depth value for positioning the text in depth.
 * @param pFont - Pointer to the rdFont structure that defines the font for the text rendering.
 * @param alignFlags - Flags for alignment, defining how the text should be aligned (RDFONT_ALIGN*).
 * @param fontSizePt - The desired font size in points. Text will be scaled relative to pFont->fontSize.
 *                     For example, if pFont->fontSize is 14 and fontSizePt is 28, text will be rendered at 2x scale.
 */
void J3DAPI rdFont_DrawTextLineEx(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags, float fontSizePt); // New

/**
 * @brief Draws a line of text to screen at a normalized screen position with clipping based on Y-axis, stopping when the screen width is exceeded.
 *
 * This function behaves similarly to `rdFont_DrawTextLine`, except it additionally checks if the text fits in the Y-axis
 * (vertical placement). If the Y-coordinate is outside the screen height range (0.0 to 1.0), or the text line would be fully
 * outside the screen's vertical bounds, the text will not be drawn.
 *
 * @param pText - Pointer to the input text string to be drawn.
 * @param x - Normalized X position in the range [0.0, 1.0], representing horizontal placement on the screen.
 * @param y - Normalized Y position in the range [0.0, 1.0], representing vertical placement on the screen.
 * @param z - NDC Z-depth value for positioning the text in depth.
 * @param pFont - Pointer to the rdFont structure that defines the font for the text rendering.
 * @param alignFlags - Flags for alignment, defining how the text should be aligned (RDFONT_ALIGN*).
 */
void J3DAPI rdFont_DrawTextLineClipped(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags);

/**
 * @brief Draws a line of text to screen at a normalized screen position with clipping and custom font size scaling.
 *
 * This function behaves similarly to `rdFont_DrawTextLineClipped`, except it additionally allows specifying
 * a custom font size in points, enabling the text to be rendered larger or smaller than the font's original size.
 * The font is scaled proportionally based on the ratio between the requested size and the font's base size.
 *
 * The function performs Y-axis clipping - if the Y-coordinate is outside the screen height range (0.0 to 1.0),
 * or the text line would be fully outside the screen's vertical bounds, the text will not be drawn.
 *
 * @param pText - Pointer to the input text string to be drawn.
 * @param x - Normalized X position in the range [0.0, 1.0], representing horizontal placement on the screen.
 * @param y - Normalized Y position in the range [0.0, 1.0], representing vertical placement on the screen.
 * @param z - NDC Z-depth value for positioning the text in depth.
 * @param pFont - Pointer to the rdFont structure that defines the font for the text rendering.
 * @param alignFlags - Flags for alignment, defining how the text should be aligned (RDFONT_ALIGN*).
 * @param fontSizePt - The desired font size in points. Text will be scaled relative to pFont->fontSize.
 *                     For example, if pFont->fontSize is 14 and fontSizePt is 28, text will be rendered at 2x scale.
 */
void J3DAPI rdFont_DrawTextLineClippedEx(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags, float fontSizePt); // New

// Helper hooking functions
void rdFont_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDFONT_H
