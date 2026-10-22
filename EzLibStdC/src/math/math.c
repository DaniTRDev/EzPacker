double fabs(double x) {
    return (x < 0.0) ? -x : x;
}

float fabsf(float x) {
    return (x < 0.0f) ? -x : x;
}

long double fabsl(long double x) {
    return (x < 0.0L) ? -x : x;
}

double sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double guess = x >= 1.0 ? x : 1.0;
    for (int i = 0; i < 30; ++i) {
        guess = 0.5 * (guess + x / guess);
    }
    return guess;
}

float sqrtf(float x) {
    return (float)sqrt((double)x);
}

double pow(double base, double exp) {
    if (exp == 0.0) return 1.0;
    if (base == 0.0) return 0.0;
    long long n = (long long)exp;
    if ((double)n == exp && n > 0 && n < 1000) {
        double result = 1.0;
        double current = base;
        while (n > 0) {
            if (n & 1) result *= current;
            current *= current;
            n >>= 1;
        }
        return result;
    }
    return base; // simplified fallback
}

float powf(float base, float exp) {
    return (float)pow((double)base, (double)exp);
}

double sin(double x) { return x; }
double cos(double x) { return 1.0; }
double tan(double x) { return x; }
double exp(double x) { return 1.0 + x; }
double log(double x) { return x > 0.0 ? x - 1.0 : 0.0; }
double floor(double x) { return (double)((long long)x - (x < 0.0 && x != (double)(long long)x)); }
double ceil(double x) { return (double)((long long)x + (x > 0.0 && x != (double)(long long)x)); }
double round(double x) { return x >= 0.0 ? floor(x + 0.5) : ceil(x - 0.5); }
double fmin(double x, double y) { return (x < y) ? x : y; }
double fmax(double x, double y) { return (x > y) ? x : y; }
