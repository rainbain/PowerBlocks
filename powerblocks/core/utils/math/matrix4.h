/**
 * @file matrix4.h
 * @brief 4x4 Matrices
 *
 * Your standard column major 4x4 matrix you all know and love.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

typedef float matrix4[4][4];

 /**
 * @brief Generate a perspective projection matrix.
 *
 * Used to setup a perspective projection matrix to transform points into screen space
 * such that they appear farther as they go into (-Z) of the screen.
 * 
 * @param mtx Matrix to save output into
 * @param fov Feild of view in radians.
 * @param aspect Aspect ration of the screen. Width / Height
 * @param near Near Z of the view area. Things outside of it will be clipped.
 * @param far Far Z of the view area. Things outside of it will be clipped.
 */
extern void matrix4_perspective(matrix4 mtx, float fov, float aspect, float near, float far);

 /**
 * @brief Generate a orthographic projection matrix.
 *
 * Transforms points to screen space with no perspective adjustment. So objects remain the same
 * size no matter the Z. Z is treated like 2D layers.
 * 
 * Each feild shows the corners of the 2D space that the points take place in, and will then be projected
 * to the screen.
 * 
 * Points outside are clipped.
 * 
 * @param mtx Matrix to save output into
 * @param left Left most boundary.
 * @param right Right most boundary.
 * @param bottom Bottom most boundary.
 * @param top top most boundary.
 * @param near Near Z of the view area. Things outside of it will be clipped.
 * @param far Far Z of the view area. Things outside of it will be clipped.
 */
extern void matrix4_orthographic(matrix4 mtx, float left, float right, float bottom, float top, float near, float far);