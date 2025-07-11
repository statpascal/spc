#include <iostream>
#include <cstdint>

using func1 = std::uint8_t (*) (std::uint8_t, std::uint16_t, std::int32_t, std::int64_t, double, char *);
using func2 = double (*) (std::uint8_t &, std::uint16_t &, std::int32_t &, std::int64_t &, double &, char *&);

extern "C" void callbacktest (func1 f1, func2 f2, void (*p1) (), void (*p2) (std::int32_t)) {
    // set value in SP program
    std::string s = "Hello, SP";
    int f1res = f1 (130, 33000, 1000 * 1000, 1000 * 1000 * 1000, 3.1415, &s [0]);
    std::cout << f1res << std::endl;
    
    // retreive values back from SP
    std::uint8_t a;
    std::uint16_t b;
    std::int32_t c;
    std::int64_t d;
    double e;
    char *f;
    double f2res = f2 (a, b, c, d, e, f);
    std::cout << f2res << std::endl;
    std::cout << static_cast<int> (a) << ' ' << b << ' ' << c << ' ' << d << ' ' << e << ' ' << f << std::endl;
    
    p1 ();
    p2 (35000);
}

extern "C" void recursiontest (void (*p) (std::int32_t), std::int32_t n) {
    p (n - 1);
}

extern "C" std::int32_t simpletest (std::int32_t a, std::int16_t b, std::int32_t (*f) (std::int32_t, std::int16_t)) {
    return f (a, b);
}
