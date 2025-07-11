#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t result;
    unsigned size, length, rotation, e;
    for (size = 2; size <= 64; size *= 2)
        for (length = 1; length < size; ++length) {
            result = 0xffffffffffffffffULL >> (64 - length);
            for (e = size; e < 64; e *= 2)
                result |= result << e;
            for (rotation = 0; rotation < size; ++rotation) {
                printf("0x%016llx (size=%u, length=%u, rotation=%u)\n",
                   (unsigned long long)result, 
                   size, length, rotation);
                result = (result >> 63) | (result << 1);
            }
        }
    return 0;
}