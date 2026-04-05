.text
.align 2

# Simple scenario for emulator validation:
# - input:  a0 = now_s
# - output: a0 = (scheduler_flags + 1234)
# - memory side effects (stack frame):
#   [sp+0] = scheduler_flags
#   [sp+4] = 1234
#   [sp+8] = scheduler_flags + 1234
.globl simple_scenario_store
simple_scenario_store:
    addi    sp, sp, -32
    sw      ra, 28(sp)
    sw      s0, 24(sp)

    mv      s0, a0

    mv      a0, s0
    jal     rv_scheduler_flags
    sw      a0, 0(sp)

    li      t0, 1234
    sw      t0, 4(sp)

    add     t1, a0, t0
    sw      t1, 8(sp)

    lw      a0, 8(sp)

    lw      s0, 24(sp)
    lw      ra, 28(sp)
    addi    sp, sp, 32
    ret

# Helper copied for standalone execution in this file.
.globl rv_scheduler_flags
rv_scheduler_flags:
    addi    sp, sp, -16
    sw      ra, 12(sp)
    sw      s0, 8(sp)

    mv      s0, a0
    li      a0, 0

    li      t0, 5
    rem     t1, s0, t0
    bnez    t1, 1f
    ori     a0, a0, 1
1:
    li      t0, 10
    rem     t1, s0, t0
    bnez    t1, 2f
    ori     a0, a0, 2
2:
    li      t0, 20
    rem     t1, s0, t0
    bnez    t1, 3f
    ori     a0, a0, 4
3:
    lw      s0, 8(sp)
    lw      ra, 12(sp)
    addi    sp, sp, 16
    ret
