#include<ripple.h>

void foo(int a[128][128], int b[128][128]) {
    ripple_block_t bs = ripple_set_block_shape(0, 128);
    size_t v0 = ripple_id(bs, 0);

    for (int i = 0; i < 64; i++) {
        int temp = a[v0][i + 63];
        a[v0][i + 63] = b[v0 * i][v0];
        b[v0 * i][v0] = temp;
    }
}
