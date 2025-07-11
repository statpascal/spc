// gcc -fPIC -I../../src -shared -o external.so external.cpp

#include <stdio.h>
#include <string>
#include <iostream>
#include <cmath>

#include "anyvalue.hpp"
#include "vectordata.hpp"

extern "C" { 

int add (int a, int b) {
    return a + b;
}

int mul (int a, int b) {
    return a * b;
}

void inc (int *a) {
    ++*a;
}

struct rec {
    int a;
    long long b;
    char c;
    double d;
    bool e;
    struct {
        unsigned char g;
        unsigned short h;
    } f;
};

void printrec (rec *r) {
    printf ("%d %lld %c %g %d %d %d\n", r->a, r->b, r->c, r->d, r->e, r->f.g, r->f.h);
}

void changerec (rec *r) {
    r->a = 7;
    r->b = -1;
    r->c = 'D';
    r->d = 2.71828;
    r->e = true;
    r->f.h = 99;
}

void printstring (const char *p) {
    puts (p);
}

void printstring1 (statpascal::TAnyValue p) {
    if (p.hasValue ())
        std::cout << p.get<std::string> ();
}

void setstring (statpascal::TAnyValue *p) {
    std::string s = "A string defined in C++";
    *p = statpascal::TAnyValue (std::move (s));
}

void modifystring (statpascal::TAnyValue *p) {
    if (p->hasValue ()) {
        p->copyOnWrite ();
        for (std::string::value_type &c: p->get<std::string> ())
            c = std::toupper (c);
    }
}

}
