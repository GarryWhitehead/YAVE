/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "noise_generator.h"

#include <utility/colour.h>

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <random>

namespace yave
{
NoiseGenerator::NoiseGenerator(uint32_t seed)
{
    permutations_.resize(256);
    std::iota(permutations_.begin(), permutations_.end(), 0);

    std::default_random_engine engine(seed);

    std::shuffle(permutations_.begin(), permutations_.end(), engine);

    // Duplicate the permutation vector
    permutations_.insert(permutations_.end(), permutations_.begin(), permutations_.end());
}

NoiseGenerator::~NoiseGenerator() {}

double NoiseGenerator::fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

double NoiseGenerator::lerp(double t, double a, double b) { return a + t * (b - a); }

double NoiseGenerator::grad(int hash, double x, double y, double z)
{
    int h = hash & 15;
    // Convert lower 4 bits of hash into 12 gradient directions
    double u = h < 8 ? x : y, v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double NoiseGenerator::generateNoise(double x, double y, double z)
{
    int X = (int)floor(x) & 0xFF;
    int Y = (int)floor(y) & 0xFF;
    int Z = (int)floor(z) & 0xFF;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    // Compute fade curves for each of x, y, z
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);

    // Hash coordinates of the 8 cube corners
    int A = permutations_[X] + Y;
    int AA = permutations_[A] + Z;
    int AB = permutations_[A + 1] + Z;
    int B = permutations_[X + 1] + Y;
    int BA = permutations_[B] + Z;
    int BB = permutations_[B + 1] + Z;

    // Add blended results from 8 corners of cube
    double res = lerp(
        w,
        lerp(
            v,
            lerp(u, grad(permutations_[AA], x, y, z), grad(permutations_[BA], x - 1, y, z)),
            lerp(
                u, grad(permutations_[AB], x, y - 1, z), grad(permutations_[BB], x - 1, y - 1, z))),
        lerp(
            v,
            lerp(
                u,
                grad(permutations_[AA + 1], x, y, z - 1),
                grad(permutations_[BA + 1], x - 1, y, z - 1)),
            lerp(
                u,
                grad(permutations_[AB + 1], x, y - 1, z - 1),
                grad(permutations_[BB + 1], x - 1, y - 1, z - 1))));
    return (res + 1.0) / 2.0;
}


} // namespace yave