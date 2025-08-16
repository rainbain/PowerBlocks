/**
 * @file gx_immediate.h
 * @brief Extension of GX with the immediate draw calls.
 *
 * Implements things like gxVertex3f and stuff.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

// This header is not supposed to be included directly.
// Instead include gx.h

// These functions are implemented in the order
// They should be described to hardware.
// That is gxVertex, gxNormal, gxColor, gxTexCoord

/* -------------------VERTEX--------------------- */

#define gxVertex2f(x, y) GX_WPAR_F32 = x; \
                         GX_WPAR_F32 = y;
#define gxVertex3f(x, y, z) GX_WPAR_F32 = x; \
                            GX_WPAR_F32 = y; \
                            GX_WPAR_F32 = z;

#define gxVertex2ub(x, y) GX_WPAR_U8 = x; \
                          GX_WPAR_U8 = y;
#define gxVertex3ub(x, y, z) GX_WPAR_U8 = x; \
                             GX_WPAR_U8 = y; \
                             GX_WPAR_U8 = z;

#define gxVertex2b(x, y) GX_WPAR_S8 = x; \
                         GX_WPAR_S8 = y;
#define gxVertex3b(x, y, z) GX_WPAR_S8 = x; \
                            GX_WPAR_S8 = y; \
                            GX_WPAR_S8 = z;

#define gxVertex2us(x, y) GX_WPAR_U16 = x; \
                          GX_WPAR_U16 = y;
#define gxVertex3us(x, y, z) GX_WPAR_U16 = x; \
                             GX_WPAR_U16 = y; \
                             GX_WPAR_U16 = z;

#define gxVertex2s(x, y) GX_WPAR_S16 = x; \
                         GX_WPAR_S16 = y;
#define gxVertex3s(x, y, z) GX_WPAR_S16 = x; \
                            GX_WPAR_S16 = y; \
                            GX_WPAR_S16 = z;

#define gxVertex2ui(x, y) GX_WPAR_U32 = x; \
                          GX_WPAR_U32 = y;
#define gxVertex3ui(x, y, z) GX_WPAR_U32 = x; \
                             GX_WPAR_U32 = y; \
                             GX_WPAR_U32 = z;

#define gxVertex2i(x, y) GX_WPAR_S32 = x; \
                         GX_WPAR_S32 = y;
#define gxVertex3i(x, y, z) GX_WPAR_S32 = x; \
                            GX_WPAR_S32 = y; \
                            GX_WPAR_S32 = z;

#define gxVertexIdx1ub(i) GX_WPAR_U8 = i;
#define gxVertexIdx1us(i) GX_WPAR_U16 = i;

/* -------------------NORMALS--------------------- */

#define gxNormal3f(x, y, z) GX_WPAR_F32 = x; \
                            GX_WPAR_F32 = y; \
                            GX_WPAR_F32 = z;

#define gxNormal3b(x, y, z) GX_WPAR_S8 = x; \
                            GX_WPAR_S8 = y; \
                            GX_WPAR_S8 = z;

#define gxNormal3s(x, y, z) GX_WPAR_S16 = x; \
                            GX_WPAR_S16 = y; \
                            GX_WPAR_S16 = z;

#define gxNormal3i(x, y, z) GX_WPAR_S32 = x; \
                            GX_WPAR_S32 = y; \
                            GX_WPAR_S32 = z;

#define gxNormalIdx1ub(i) GX_WPAR_U8 = i;
#define gxNormalIdx1us(i) GX_WPAR_U16 = i;

/* -------------------COLORS--------------------- */

#define gxColor3f(r, g, b) GX_WPAR_F32 = r; \
                           GX_WPAR_F32 = g; \
                           GX_WPAR_F32 = b;
#define gxColor4f(r, g, b, a) GX_WPAR_F32 = r; \
                              GX_WPAR_F32 = g; \
                              GX_WPAR_F32 = b; \
                              GX_WPAR_F32 = a;

#define gxColor1ub(v) GX_WPAR_U8 = v;
#define gxColor3ub(r, g, b) GX_WPAR_U8 = r; \
                            GX_WPAR_U8 = g; \
                            GX_WPAR_U8 = b;
#define gxColor4ub(r, g, b, a) GX_WPAR_U8 = r; \
                               GX_WPAR_U8 = g; \
                               GX_WPAR_U8 = b; \
                               GX_WPAR_U8 = a;

#define gxColor1b(v) GX_WPAR_S8 = v;
#define gxColor3b(r, g, b) GX_WPAR_S8 = r; \
                           GX_WPAR_S8 = g; \
                           GX_WPAR_S8 = b;
#define gxColor4b(r, g, b, a) GX_WPAR_S8 = r; \
                              GX_WPAR_S8 = g; \
                              GX_WPAR_S8 = b; \
                              GX_WPAR_S8 = a;
         
#define gxColor1us(v) GX_WPAR_U16 = v;
#define gxColor3us(r, g, b) GX_WPAR_U16 = r; \
                            GX_WPAR_U16 = g; \
                            GX_WPAR_U16 = b;
#define gxColor4us(r, g, b, a) GX_WPAR_U16 = r; \
                               GX_WPAR_U16 = g; \
                               GX_WPAR_U16 = b; \
                               GX_WPAR_U16 = a;

#define gxColor1s(v) GX_WPAR_S16 = v;
#define gxColor3s(r, g, b) GX_WPAR_S16 = r; \
                           GX_WPAR_S16 = g; \
                           GX_WPAR_S16 = b;
#define gxColor4s(r, g, b, a) GX_WPAR_S16 = r; \
                              GX_WPAR_S16 = g; \
                              GX_WPAR_S16 = b; \
                              GX_WPAR_S16 = a;

#define gxColor1ui(v) GX_WPAR_U32 = v;
#define gxColor3ui(r, g, b) GX_WPAR_U32 = r; \
                            GX_WPAR_U32 = g; \
                            GX_WPAR_U32 = b;
#define gxColor4ui(r, g, b, a) GX_WPAR_U32 = r; \
                               GX_WPAR_U32 = g; \
                               GX_WPAR_U32 = b; \
                               GX_WPAR_U32 = a;

#define gxColor1i(v) GX_WPAR_U32 = v;
#define gxColor3i(r, g, b) GX_WPAR_S32 = r; \
                           GX_WPAR_S32 = g; \
                           GX_WPAR_S32 = b;
#define gxColor4i(r, g, b, a) GX_WPAR_S32 = r; \
                              GX_WPAR_S32 = g; \
                              GX_WPAR_S32 = b; \
                              GX_WPAR_S32 = a;

#define gxColorIdx1ub(i) GX_WPAR_U8 = i;
#define gxColorIdx1us(i) GX_WPAR_U16 = i;

/* -------------------TEXTURE COORDINATES--------------------- */

#define gxTexCoord1f(s) GX_WPAR_F32 = s;
#define gxTexCoord2f(s, t) GX_WPAR_F32 = s; \
                           GX_WPAR_F32 = t;

#define gxTexCoord1ub(s) GX_WPAR_U8 = s;
#define gxTexCoord2ub(s, t) GX_WPAR_U8 = s; \
                            GX_WPAR_U8 = t;

#define gxTexCoord1b(s) GX_WPAR_S8 = s;
#define gxTexCoord2b(s, t) GX_WPAR_S8 = s; \
                            GX_WPAR_S8 = t;

#define gxTexCoord1us(s) GX_WPAR_U16 = s;
#define gxTexCoord2us(s, t) GX_WPAR_U16 = s; \
                            GX_WPAR_U16 = t;

#define gxTexCoord1s(s) GX_WPAR_S16 = s;
#define gxTexCoord2s(s, t) GX_WPAR_S16 = s; \
                           GX_WPAR_S16 = t;
        
#define gxTexCoord1ui(s) GX_WPAR_U32 = s;
#define gxTexCoord2ui(s, t) GX_WPAR_U32 = s; \
                            GX_WPAR_U32 = t;

#define gxTexCoord1i(s) GX_WPAR_S32 = s;
#define gxTexCoord2i(s, t) GX_WPAR_S32 = s; \
                           GX_WPAR_S32 = t;

#define gxTexCoordIdx1ub(i) GX_WPAR_U8 = i;
#define gxTexCoordIdx1us(i) GX_WPAR_U16 = i;

/* -------------------MATRIX INDEX--------------------- */
#define gxMatrixIdx1ub(i) GX_WPAR_U8 = i;