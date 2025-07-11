#include "rng.hpp"
#include <ctime>
#include <iostream>

namespace statpascal {

thread_local std::mt19937 TRNG::generator;
std::uniform_real_distribution<> TRNG::uniform (0.0, 1.0);

void TRNG::randomize () {
    randomize (std::time (nullptr));
}

void TRNG::randomize (std::uint32_t seed) {
    generator.seed (seed);
}

double TRNG::val () {
    return uniform (generator);
}

std::int64_t TRNG::val (std::int64_t a, std::int64_t b) {
    return std::uniform_int_distribution<std::int64_t> (a, b) (generator);
}

}
