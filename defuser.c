#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);

    uint32_t step0 = (high + 0x9079u) ^ low;
    uint32_t mixed = mid ^ 0x980au;
    uint32_t step1 = mixed + high * 4u;
    uint32_t step2 = (step1 * 5u + low) ^ 0x20a5u;
    uint32_t result = step0 ^ step2;

    values[0] = result;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;

    values[0] = (uint32_t)((challenge >> 24) & 31u);
    uint32_t shift = 9u;

    for (uint32_t i = 1u; i < 6u; ++i) {
        uint32_t base = values[i - 1] * 3u - i;
        uint32_t extra = (uint32_t)((challenge >> shift) & 15u) + 1u;
        values[i] = base ^ extra;
        shift += 2u;
    }

    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    values[0] = (uint32_t)(challenge & 7u);
    uint32_t shifted = (uint32_t)(challenge >> 3);
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t input = shifted ^ high;

    /* Numbering follows the jump table, not the p3_case_* suffixes. */
    switch (values[0]) {
        case 0u:
            values[1] = input * 3u + 0x47c3u;
            break;
        case 1u:
            values[1] = (input * 2u) ^ 0x98acu;
            break;
        case 2u:
            values[1] = (input * 2u) ^ 0xc22u;
            break;
        case 3u:
            values[1] = input * 5u + 0x10414u;
            break;
        case 4u:
            values[1] = input * 5u + 0x13e16u;
            break;
        case 5u:
            values[1] = input * 3u + 0x483au;
            break;
        case 6u:
            values[1] = (input ^ 0xae1du) + (input >> 1u);
            break;
        case 7u:
            values[1] = (input ^ 0x8e18u) + (input >> 1u);
            break;
        default:
            return 1;
    }

    return write_answer(out, cap, values, count);
}

static uint32_t p4_walk(uint32_t target, uint32_t low,
                        uint32_t high, uint32_t mix) {
    uint32_t mid = low + ((high - low) >> 1u);
    if (target == mid) {
        return (mid + 0x48u) ^ mix;
    }
    if (target < mid) {
        uint32_t child = p4_walk(target, low, mid - 1u, mix);
        return (child * 5u) ^ (mid + mix) ^ 10u;
    }

    uint32_t child = p4_walk(target, mid + 1u, high, mix);
    return (child * 5u) ^ (mid + mix) ^ 7u;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t target = (uint32_t)(challenge & 127u);
    uint32_t mix = (uint32_t)(challenge >> 7u)
                 ^ (uint32_t)(challenge >> 32u);
    values[0] = p4_walk(target, 0u, 127u, mix);

    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    struct node_data {
        uint32_t value;
        uint32_t tag;
        uint32_t next;
    };
    /* Recovered node data; next is an array index (12 denotes the end). */
    static const struct node_data nodes[12] = {
        {0xbcd3u, 0x441u, 3u},
        {0xc7f1u, 0x33u, 5u},
        {0xaba1u, 0x56cu, 1u},
        {0xc772u, 0xa88u, 7u},
        {0xfd52u, 0x60bu, 0u},
        {0x464au, 0x778u, 6u},
        {0x19fdu, 0xb63u, 10u},
        {0xca61u, 0x227u, 11u},
        {0x2acfu, 0x359u, 9u},
        {0xd968u, 0x841u, 2u},
        {0x14c2u, 0x10au, 12u},
        {0x101au, 0x944u, 8u}
    };
    static const uint32_t heads[8] = {4u, 0u, 3u, 7u, 11u, 8u, 9u, 2u};

    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t start = (uint32_t)(challenge & 7u);
    uint32_t hops = (uint32_t)((challenge >> 3u) & 3u);
    uint32_t acc = (uint32_t)(challenge >> 32u)
                 ^ (uint32_t)(challenge >> 6u)
                 ^ 0x2e28u;
    uint32_t last_tag = 0u;
    uint32_t index = heads[start];

    for (uint32_t step = 0u; step <= hops; ++step) {
        if (step > 0u) {
            index = nodes[index].next;
        }
        uint32_t value = nodes[index].value;
        uint32_t tag = nodes[index].tag;
        acc = (acc * 3u + value) ^ tag;
        last_tag = tag;
    }

    values[0] = acc;
    values[1] = last_tag;
    return write_answer(out, cap, values, count);
}

static uint32_t p6_op_0(uint32_t x) {
    return ((x << 5u) + 0x2548u) ^ (x * 3u);
}

static uint32_t p6_op_1(uint32_t x) {
    return ((x >> 3u) ^ x) + 0x6e01u;
}

static uint32_t p6_op_2(uint32_t x) {
    return (x + 0xa91cu) ^ (x >> 3u);
}

static uint32_t p6_op_3(uint32_t x) {
    uint32_t rotated = (x << 2u) | (x >> 30u);
    return rotated ^ 0xcb4fu;
}

static uint32_t p6_op_4(uint32_t x) {
    return (x ^ 0xf1bdu) + (x << 7u);
}

static uint32_t p6_op_5(uint32_t x) {
    return (x >> 2u) | (x << 30u);
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    static uint32_t (*const ops[8])(uint32_t) = {
        p6_op_0, p6_op_2, p6_op_2, p6_op_3,
        p6_op_5, p6_op_4, p6_op_4, p6_op_1
    };
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t input = (uint32_t)(challenge >> 6u)
                   ^ (uint32_t)(challenge >> 32u);
    uint32_t first = (uint32_t)(challenge & 7u);
    uint32_t second = (uint32_t)((challenge >> 3u) & 7u);
    uint32_t result = ops[first](input);
    result ^= 0xd8a8u;
    values[0] = ops[second](result) + 0xcb03u;

    return write_answer(out, cap, values, count);
}

static uint32_t secret_mix(uint32_t x) {
    return ((x >> 3u) ^ 0x9aa7u) + (x ^ 0x7129u);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    static const char prefix[] =
        "AAAAAAAA" "AAAAAAAA" "AAAAAAAA" "AAAAAAAA" "OPEN";

    uint32_t first = (uint32_t)(context & 3u);
    uint32_t cfold = (uint32_t)challenge
                   ^ (uint32_t)(challenge >> 32u);
    uint32_t kfold = (uint32_t)context
                   ^ (uint32_t)(context >> 32u);
    uint32_t adjust = (first * 0x7129u)
                    ^ (uint32_t)(context >> 16u);
    uint32_t second = secret_mix(cfold ^ kfold) ^ adjust;
    uint32_t sum = kfold + first + 0x9aa7u + (cfold >> 3u);
    uint32_t third = second ^ sum;

    int size = snprintf(out, cap, "%s%" PRIu32 " %" PRIu32 " %" PRIu32,
                        prefix, first, second, third);
    return size < 0 || (size_t)size >= cap;
}
