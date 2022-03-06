#include <cstdint>
#include <arm_neon.h>

#if defined(NEW_HAVE_ARM_ACLE)
#include <arm_acle.h>
#endif

uint8_t * key;
uint8_t * blk;
uint8_t * out;

int main(void) {
    uint8x16_t block = vld1q_u8(blk);
    block = vaeseq_u8(block, vld1q_u8(key));
    vst1q_u8(out, block);
}
