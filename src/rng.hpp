/** \file rng.hpp
*/

#pragma once

#include <random>

namespace statpascal {

class TRNG {
public:
    static void randomize ();
    static void randomize (std::uint32_t seed);
    
    static double val ();
    static std::int64_t val (std::int64_t a, std::int64_t b);

private:
    static thread_local std::mt19937 generator;
    static std::uniform_real_distribution<> uniform;
};

}