/**
 * @file utah-teapot.c
 * @brief Asset of the utah teapot.
 *
 * Asset of the utah teapot.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "utah_teapot.h"

#include "powerblocks/core/graphics/gx.h"

void utah_teapot_draw() {
    int vertices = utah_teapot_model_indices_count;
    gx_begin(GX_TRIANGLES, 0, vertices);
    for(int i = 0; i < vertices; i++) {
        int j = utah_teapot_model_indices[i] * 3;

        gxVertex3s(
            utah_teapot_model_positions[j + 0],
            utah_teapot_model_positions[j + 1],
            utah_teapot_model_positions[j + 2]
        );

        gxNormal3s(
            utah_teapot_model_normals[j + 0],
            utah_teapot_model_normals[j + 1],
            utah_teapot_model_normals[j + 2]
        )
    }
    gx_end();
}