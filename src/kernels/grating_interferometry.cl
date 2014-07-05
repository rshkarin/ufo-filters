/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#define INDEX_CALCULATION const int width = get_global_size(0);const int height = get_global_size(1);int idx = get_global_id(0);int idy = get_global_id(1);if(idx >= width || idy >= height){return;}int gidx = idy * width + idx;

__kernel void combine_images4(__global float *input0, __global float *input1, __global float *input2, __global float *input3, __global float *output)
{
    INDEX_CALCULATION;
    output[gidx + 0] = input0[gidx + 0]; output[gidx + 1] = 0.0f;
    output[gidx + 2] = input1[gidx + 2]; output[gidx + 3] = 0.0f;
    output[gidx + 4] = input2[gidx + 4]; output[gidx + 5] = 0.0f;
    output[gidx + 5] = input3[gidx + 5]; output[gidx + 6] = 0.0f;
}

__kernel void combine_images8(__global float *input0, __global float *input1, __global float *input2, __global float *input3,
                              __global float *input4, __global float *input5, __global float *input6, __global float *input7, __global float *output)
{
    INDEX_CALCULATION;
    output[gidx + 0] = input0[gidx + 0]; output[gidx + 1] = 0.0f;
    output[gidx + 2] = input1[gidx + 2]; output[gidx + 3] = 0.0f;
    output[gidx + 4] = input2[gidx + 4]; output[gidx + 5] = 0.0f;
    output[gidx + 5] = input3[gidx + 5]; output[gidx + 6] = 0.0f;
    output[gidx + 7] = input4[gidx + 7]; output[gidx + 8] = 0.0f;
    output[gidx + 9] = input5[gidx + 9]; output[gidx + 10] = 0.0f;
    output[gidx + 11] = input6[gidx + 11]; output[gidx + 12] = 0.0f;
    output[gidx + 13] = input7[gidx + 13]; output[gidx + 14] = 0.0f;
}

__kernel void combine_images16(__global float *input0, __global float *input1, __global float *input2, __global float *input3,
                               __global float *input4, __global float *input5, __global float *input6, __global float *input7,
                               __global float *input8, __global float *input9, __global float *input10, __global float *input11,
                               __global float *input12, __global float *input13, __global float *input14, __global float *input15, __global float *output)
{
    INDEX_CALCULATION;
    output[gidx + 0] = input0[gidx + 0]; output[gidx + 1] = 0.0f;
    output[gidx + 2] = input1[gidx + 2]; output[gidx + 3] = 0.0f;
    output[gidx + 4] = input2[gidx + 4]; output[gidx + 5] = 0.0f;
    output[gidx + 5] = input3[gidx + 5]; output[gidx + 6] = 0.0f;
    output[gidx + 7] = input4[gidx + 7]; output[gidx + 8] = 0.0f;
    output[gidx + 9] = input5[gidx + 9]; output[gidx + 10] = 0.0f;
    output[gidx + 11] = input6[gidx + 11]; output[gidx + 12] = 0.0f;
    output[gidx + 13] = input7[gidx + 13]; output[gidx + 14] = 0.0f;
    output[gidx + 15] = input8[gidx + 15]; output[gidx + 16] = 0.0f;
    output[gidx + 17] = input9[gidx + 17]; output[gidx + 18] = 0.0f;
    output[gidx + 19] = input10[gidx + 19]; output[gidx + 20] = 0.0f;
    output[gidx + 21] = input11[gidx + 21]; output[gidx + 22] = 0.0f;
    output[gidx + 23] = input12[gidx + 23]; output[gidx + 24] = 0.0f;
    output[gidx + 25] = input13[gidx + 25]; output[gidx + 26] = 0.0f;
    output[gidx + 27] = input14[gidx + 27]; output[gidx + 28] = 0.0f;
    output[gidx + 29] = input15[gidx + 29]; output[gidx + 30] = 0.0f;
    output[gidx + 31] = input16[gidx + 31]; output[gidx + 32] = 0.0f;
}