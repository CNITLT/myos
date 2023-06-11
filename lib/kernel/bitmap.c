#include "bitmap.h"
#include "string.h"
#include "debug.h"

void bitmap_init(bitmap* p_bitmap) {
    memset(p_bitmap->bits, 0, p_bitmap->len_byte);
}


bit_state bitmap_get(bitmap* p_bitmap, uint32_t index_bit) {
    assert(index_bit < p_bitmap->len_byte * 8);
    return bit_state(p_bitmap->bits[index_bit / 8] >> (index_bit % 8));
}


bit_state bitmap_set(bitmap* p_bitmap, uint32_t index_bit, bit_state value) {
    assert(index_bit < p_bitmap->len_byte * 8);
    bit_state old = bitmap_get(p_bitmap, index_bit);
    p_bitmap->bits[index_bit / 8] &= ~(1 << index_bit % 8);
    p_bitmap->bits[index_bit / 8] |= (value << index_bit % 8);
    return old;
}

uint32_t bitmap_find_range(bitmap* p_bitmap, uint32_t range_bit) {
    assert(range_bit <= p_bitmap->len_byte * 8);
    uint32_t count = 0;
    for (uint32_t i = 0; i < p_bitmap->len_byte * 8; i++) {
        if (bitmap_get(p_bitmap, i) == BIT_STATE_UNUSE) {
            count++;
            if (count == range_bit) {
                return i - range_bit + 1;
            }
        }
        else {
            count = 0;
        }
    }
    return BITMAP_RANGE_NOTFOUND;
}


void bitmap_range_set(bitmap* p_bitmap, uint32_t index_bit, uint32_t range_bit, bit_state value) {
    assert(index_bit + range_bit - 1 < p_bitmap->len_byte * 8);
    for (uint32_t i = index_bit; i < p_bitmap->len_byte * 8 && i - index_bit< range_bit; i++) {
        bitmap_set(p_bitmap, i, value);
    }
}