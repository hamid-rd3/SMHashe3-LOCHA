#include <cstdint>
#include "../lib/AES-ppc.h"

uint32_t RK[100];
uint8_t pt[16];
uint8_t ct[16];

int main(void) {
    AES_Encrypt_PPC<10>(RK, pt, ct);
}
