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

float flat_correct_point(float data, float dark, float flat) {
    return (data - dark) / (flat - dark);
}

__kernel void
flat_correct(global float *sinogram,
             global float *corrected,
             global const float *dark,
             global const float *flat)

{
    const int idx = get_global_id(0);
    const int idy = get_global_id(1);
    const int width = get_global_size(0);

    //corrected[idy * width + idx] = sinogram[idy * width + idx];
    corrected[idy * width + idx] =
            flat_correct_point(sinogram[idy * width + idx], dark[idx], flat[idx]);
}

__kernel void
absorptivity(global float *sinogram,
             global float *corrected,
             global const float *dark,
             global const float *flat)
{
    const int idx = get_global_id(0);
    const int idy = get_global_id(1);
    const int width = get_global_size(0);

    corrected[idy * width + idx] =
            -log(flat_correct_point(sinogram[idy * width + idx], dark[idx], flat[idx]));
}
