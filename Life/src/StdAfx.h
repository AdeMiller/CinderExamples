// STL
#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

// Microsoft Parallel Patterns Library
#include <ppl.h>
#include <amp.h>

// Boost filesystem
#include "boost/filesystem.hpp"

// Cinder
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"

template <int MAX, typename T>
inline T wrap_map(const T& x) restrict(amp, cpu)
{
    if (x < 0)
        return (MAX + x);
    if (x >= MAX)
        return (x - MAX);
    return x;
}

template <typename T>
inline T clamp(const T& v, const T& min, const T& max)
{
    return std::max(std::min(v, max), min);
}
