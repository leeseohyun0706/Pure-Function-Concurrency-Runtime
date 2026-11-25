

#ifndef CONCAL_DUAL_H
#define CONCAL_DUAL_H
#include <cmath>
struct Dual {
    double value;
    double derivative{1.0};

    static Dual add(Dual a, Dual b) {
        return Dual{
            a.value + b.value,
            a.derivative + b.derivative
        };
    }

    static Dual sub(Dual a, Dual b) {
        return Dual{
            a.value - b.value,
            a.derivative - b.derivative
        };
    }

    static Dual mul(Dual a,  Dual b) {
        return Dual{
            a.value * b.value,
                a.derivative * b.value + a.value * b.derivative // Product rule
            };
    }

    static Dual div( Dual a,  Dual b) {
        return Dual{
            a.value / b.value,
                (a.derivative * b.value - a.value * b.derivative) / std::pow(b.value, 2) // Quotient rule
            };
    }

    static Dual sin(Dual x) {
        return Dual{
            std::sin(x.value),
            std::cos(x.value) * x.derivative // Chain Rule
        };
    }

    static Dual cos(Dual x) {
        return Dual{
            std::cos(x.value),
            -std::sin(x.value) * x.derivative // Chain Rule
        };
    }

    static Dual pow(Dual x, int p) {
        return Dual{
        std::pow(x.value, p),
            p * std::pow(x.value, p - 1) * x.derivative
        };
    }

    static Dual exp(Dual x) {
        return Dual{
            std::exp(x.value),
            std::exp(x.value) * x.derivative // dy[e^x] = e^x * dx
        };
    }

    static Dual sqrt(Dual x) {
        return Dual{
        std::sqrt(x.value),
            (1 / (2 * std::sqrt(x.value))) * x.derivative // dy[sqrt(x)] = dy[x^(1/2)] = 1/(2*sqrt(x)) * dx
        };
    }

};


#endif //CONCAL_DUAL_H
