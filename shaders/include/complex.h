#ifndef COMPLEX_H
#define COMPLEX_H

struct complex
{
    float r;
    float i;
};

complex cmul(complex c0, complex c1)
{
    complex c;
    c.r = c0.r * c1.r - c0.i * c1.i;
    c.i = c0.r * c1.i + c0.i * c1.r;
    return c;
}

complex cadd(complex c0, complex c1)
{
    complex c;
    c.r = c0.r + c1.r;
    c.i = c0.i + c1.i;
    return c;
}

complex conjugate(complex c0)
{
    complex conj = complex(c0.r, -c0.i);
    return conj;
}


#endif // COMPLEX_H