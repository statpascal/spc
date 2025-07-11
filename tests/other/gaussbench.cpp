#include <math.h>

const double MY_PI = 3.1415926;

extern "C" double gaussden_c (double x, double m, double s) {
    double xs = (x - m) / s;
    if (fabs (xs) > 37) 
        return 0.0;
    else 
        return exp (-xs * xs / 2.0) / (sqrt (2 * MY_PI) * s);
}

