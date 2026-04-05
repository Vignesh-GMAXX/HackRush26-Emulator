.text
.align 2

# Sort 10 constants using register-only compare-swap passes.
# Final sorted order is kept in s0-s9 (ascending).
# For easy inspection, also copied to a0-a7, t0, t1 before return.
.globl sort_10_registers
sort_10_registers:
    # Unsorted input data in registers.
    li      s0, 42
    li      s1, 7
    li      s2, 99
    li      s3, 13
    li      s4, 5
    li      s5, 61
    li      s6, 28
    li      s7, 1
    li      s8, 77
    li      s9, 34

    # 9 passes are enough for 10 elements.
    li      t6, 9

1:
    # Even phase: (0,1), (2,3), (4,5), (6,7), (8,9)
    bge     s1, s0, 2f
    mv      t2, s0
    mv      s0, s1
    mv      s1, t2
2:
    bge     s3, s2, 3f
    mv      t2, s2
    mv      s2, s3
    mv      s3, t2
3:
    bge     s5, s4, 4f
    mv      t2, s4
    mv      s4, s5
    mv      s5, t2
4:
    bge     s7, s6, 5f
    mv      t2, s6
    mv      s6, s7
    mv      s7, t2
5:
    bge     s9, s8, 6f
    mv      t2, s8
    mv      s8, s9
    mv      s9, t2

    # Odd phase: (1,2), (3,4), (5,6), (7,8)
6:
    bge     s2, s1, 7f
    mv      t2, s1
    mv      s1, s2
    mv      s2, t2
7:
    bge     s4, s3, 8f
    mv      t2, s3
    mv      s3, s4
    mv      s4, t2
8:
    bge     s6, s5, 9f
    mv      t2, s5
    mv      s5, s6
    mv      s6, t2
9:
    bge     s8, s7, 10f
    mv      t2, s7
    mv      s7, s8
    mv      s8, t2

10:
    addi    t6, t6, -1
    bnez    t6, 1b

    # Copy sorted values to argument/temp registers for quick viewing.
    mv      a0, s0
    mv      a1, s1
    mv      a2, s2
    mv      a3, s3
    mv      a4, s4
    mv      a5, s5
    mv      a6, s6
    mv      a7, s7
    mv      t0, s8
    mv      t1, s9

    ret
