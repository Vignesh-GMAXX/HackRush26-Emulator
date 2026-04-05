.text
.align 2

# RISC-V RV32IM assembly kernels for the Part 1 runtime helpers.
#
# Calling convention: standard RISC-V ABI
# - a0-a7: arguments / return values
# - t0-t6: caller-saved temporaries
# - s0-s11: callee-saved registers
#
# These helpers mirror the current C simulator math and can be executed by a
# future Python emulator without depending on the host build.

# -----------------------------------------------------------------------------
# uint32_t rv_scheduler_flags(int32_t now_s)
# Returns task-release flags for the current second:
# bit0 (1): run orbit task       when now_s % 5  == 0
# bit1 (2): run risk + maneuver   when now_s % 10 == 0
# bit2 (4): run telemetry + tx    when now_s % 20 == 0
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# int32_t rv_abs_i32(int32_t x)
# Returns absolute value of x.
# -----------------------------------------------------------------------------
.globl rv_abs_i32
rv_abs_i32:
    bge     a0, zero, 4f
    sub     a0, zero, a0
4:
    ret

# -----------------------------------------------------------------------------
# int32_t rv_wrap_theta_mdeg(int32_t theta_mdeg)
# Normalizes angle to [0, 360000).
# -----------------------------------------------------------------------------
.globl rv_wrap_theta_mdeg
rv_wrap_theta_mdeg:
    li      t0, 360000
5:
    blt     a0, t0, 6f
    addi    a0, a0, -360000
    j       5b
6:
    blt     a0, zero, 7f
    ret
7:
    addi    a0, a0, 360000
    j       5b

# -----------------------------------------------------------------------------
# int32_t rv_signed_delta_theta_mdeg(int32_t debris_theta_mdeg, int32_t sat_theta_mdeg)
# Returns shortest signed angular difference in [-180000, 180000].
# -----------------------------------------------------------------------------
.globl rv_signed_delta_theta_mdeg
rv_signed_delta_theta_mdeg:
    sub     a0, a0, a1
8:
    li      t0, 180000
    blt     t0, a0, 9f
    li      t0, -180000
    blt     a0, t0, 10f
    ret
9:
    addi    a0, a0, -360000
    j       8b
10:
    addi    a0, a0, 360000
    j       8b

# -----------------------------------------------------------------------------
# int32_t rv_proximity_score(int32_t dr_m, int32_t dtheta_mdeg)
# proximity = abs(dr_m) + abs(dtheta_mdeg) / 200
# -----------------------------------------------------------------------------
.globl rv_proximity_score
rv_proximity_score:
    addi    sp, sp, -16
    sw      ra, 12(sp)
    sw      s0, 8(sp)
    sw      s1, 4(sp)
    sw      s2, 0(sp)

    mv      s0, a0
    mv      s1, a1

    mv      a0, s0
    jal     rv_abs_i32
    mv      s2, a0

    mv      a0, s1
    jal     rv_abs_i32
    mv      t1, a0

    li      t2, 200
    div     t3, t1, t2
    add     a0, s2, t3

    lw      s2, 0(sp)
    lw      s1, 4(sp)
    lw      s0, 8(sp)
    lw      ra, 12(sp)
    addi    sp, sp, 16
    ret

# -----------------------------------------------------------------------------
# int32_t rv_risk_classify(int32_t proximity, int32_t size_cm)
# Returns: 2 -> HIGH-RISK, 1 -> WATCH, 0 -> SAFE
# Thresholds mirror the current C task_risk_eval implementation.
# -----------------------------------------------------------------------------
.globl rv_risk_classify
rv_risk_classify:
    mul     t0, a1, 100
    blt     a0, t0, 11f

    mul     t1, a1, 1000
    blt     a0, t1, 12f

    li      a0, 0
    ret

11:
    li      a0, 2
    ret

12:
    li      a0, 1
    ret

# -----------------------------------------------------------------------------
# int32_t rv_should_maneuver(int32_t high_risk_count)
# Returns 1 if high_risk_count > 0 else 0.
# -----------------------------------------------------------------------------
.globl rv_should_maneuver
rv_should_maneuver:
    slt     a0, zero, a0
    ret