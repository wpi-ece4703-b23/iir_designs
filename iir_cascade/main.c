#include "xlaudio.h"
#include "xlaudio_armdsp.h"
#include <stdlib.h>

typedef struct cascadestate {
    float32_t s[2];   // state
    float32_t b[3];  // nominator coeff  b0 b1 b2
    float32_t a[2];  // denominator coeff   a1 a2
} cascadestate_t;

float32_t cascadeiir(float32_t x, cascadestate_t *p) {
    float32_t v = x - (p->s[0] * p->a[0]) -  (p->s[1] * p->a[1]);
    float32_t y = (v * p->b[0]) + (p->s[0] * p->b[1]) + (p->s[1] * p->b[2]);
    p->s[1] = p->s[0];
    p->s[0] = v;
    return y;
}

float32_t cascadeiir_transpose(float32_t x, cascadestate_t *p) {
    float32_t y = (x * p->b[0]) + p->s[0];
    p->s[0]     = (x * p->b[1]) - (y * p->a[0]) + p->s[1];
    p->s[1]     = (x * p->b[2]) - (y * p->a[1]);
    return y;
}

void createcascade(float32_t b0,
                   float32_t b1,
                   float32_t b2,
                   float32_t a1,
                   float32_t a2,
                   cascadestate_t *p) {
    p->b[0] = b0;
    p->b[1] = b1;
    p->b[2] = b2;
    p->a[0] = a1;
    p->a[1] = a2;
    p->s[0] = p->s[1] = 0.0f;
}

cascadestate_t stage1;
cascadestate_t stage2;

void initcascade() {
    createcascade(  /* b0 */  1.0f,
                    /* b1 */  0.0f,
                    /* b2 */  1.0f,
                    /* a1 */ -0.7071f,
                    /* a2 */ 0.25f,
                    &stage1);
    createcascade(  /* b0 */  1.0f,
                    /* b1 */  1.0f,
                    /* b2 */  0.0f,
                    /* a1 */ +0.7071f,
                    /* a2 */ 0.25f,
                    &stage2);
}

uint16_t processCascade(uint16_t x) {

    float32_t input = xlaudio_adc14_to_f32(0x1800 + rand() % 0x1000);
    float32_t v;

    v = cascadeiir(input, &stage1);
    v = cascadeiir(v, &stage2);

    return xlaudio_f32_to_dac14(v*0.125);
}

uint16_t processCascade_transpose(uint16_t x) {

    float32_t input = xlaudio_adc14_to_f32(0x1800 + rand() % 0x1000);
    float32_t v;

    v = cascadeiir_transpose(input, &stage1);
    v = cascadeiir_transpose(v, &stage2);

    return xlaudio_f32_to_dac14(v*0.125);
}

#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    initcascade();

    xlaudio_init_intr(FS_32000_HZ, XLAUDIO_J1_2_IN, processCascade_transpose);
    xlaudio_run();

    return 1;
}
