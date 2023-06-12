#ifndef RANDOM_H
#define RANDOM_H

#define PI 3.1415926535897932384626433832795

float rand(vec2 co)
{
    // From: http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
    // Produces approximately uniformly distributed values in the interval [0,1].
    float r = fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);

    if (r == 0.0)
    {
        r = 0.000000000001;
    }
    return r;
}


vec2 gaussrand(vec2 co, vec2 offsets)
{
    // Box-Muller method for sampling from the normal distribution
    // http://en.wikipedia.org/wiki/Normal_distribution#Generating_values_from_normal_distribution

    // offsets are applied here otherwise we will end up with the same random numbers
    // ('rand' is not that random, more of a hash function.)
    float U = rand(co + vec2(offsets.x, offsets.x));
    float V = rand(co + vec2(offsets.y, offsets.y));

    return vec2(sqrt(-2.0 * log(U)) * sin(2.0 * PI * V), sqrt(-2.0 * log(U)) * cos(2.0 * PI * V));
}

#endif