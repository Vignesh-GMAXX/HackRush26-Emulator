.text
.set noreorder

# MIPS32 assembly kernels for the Part 1 runtime.
# These are ISA-level equivalents of core C logic in main/tasks.
#
# Calling convention: o32 ABI
# - a0-a3: args
# - v0-v1: return values
#
# -----------------------------------------------------------------------------
# uint32_t mips_scheduler_flags(int32_t now_s)
# Returns task-release flags for the current second:
# bit0 (1): run T1 orbit task      when now_s % 5  == 0
# bit1 (2): run T2/T3 risk+maneuver when now_s % 10 == 0
# bit2 (4): run T4/T6 telem+tx      when now_s % 20 == 0
# -----------------------------------------------------------------------------
.globl mips_scheduler_flags
mips_scheduler_flags:
    addiu   $sp, $sp, -16
    sw      $ra, 12($sp)
    sw      $s0, 8($sp)

    move    $s0, $a0
    move    $v0, $zero

    # if (now_s % 5 == 0) flags |= 1
    li      $t0, 5
    div     $s0, $t0
    mfhi    $t1
    bne     $t1, $zero, 1f
    nop
    ori     $v0, $v0, 1
1:

    # if (now_s % 10 == 0) flags |= 2
    li      $t0, 10
    div     $s0, $t0
    mfhi    $t1
    bne     $t1, $zero, 2f
    nop
    ori     $v0, $v0, 2
2:

    # if (now_s % 20 == 0) flags |= 4
    li      $t0, 20
    div     $s0, $t0
    mfhi    $t1
    bne     $t1, $zero, 3f
    nop
    ori     $v0, $v0, 4
3:

    lw      $s0, 8($sp)
    lw      $ra, 12($sp)
    addiu   $sp, $sp, 16
    jr      $ra
    nop

# -----------------------------------------------------------------------------
# int32_t mips_abs_i32(int32_t x)
# Returns absolute value of x.
# -----------------------------------------------------------------------------
.globl mips_abs_i32
mips_abs_i32:
    bgez    $a0, 4f
    nop
    subu    $v0, $zero, $a0
    jr      $ra
    nop
4:
    move    $v0, $a0
    jr      $ra
    nop

# -----------------------------------------------------------------------------
# int32_t mips_risk_classify(int32_t dr, int32_t dtheta)
# proximity = abs(dr) + abs(dtheta) / 200
# return: 2 -> HIGH-RISK (proximity < 60)
#         1 -> WATCH     (proximity < 220)
#         0 -> SAFE
# -----------------------------------------------------------------------------
.globl mips_risk_classify
mips_risk_classify:
    addiu   $sp, $sp, -24
    sw      $ra, 20($sp)
    sw      $s0, 16($sp)
    sw      $s1, 12($sp)

    move    $s0, $a0      # dr
    move    $s1, $a1      # dtheta

    # abs(dr)
    move    $a0, $s0
    jal     mips_abs_i32
    nop
    move    $t0, $v0

    # abs(dtheta)
    move    $a0, $s1
    jal     mips_abs_i32
    nop
    move    $t1, $v0

    # dtheta_term = abs(dtheta) / 200
    li      $t2, 200
    div     $t1, $t2
    mflo    $t3

    # proximity = abs(dr) + dtheta_term
    addu    $t4, $t0, $t3

    # if (proximity < 60) return 2
    li      $t5, 60
    slt     $t6, $t4, $t5
    beq     $t6, $zero, 5f
    nop
    li      $v0, 2
    b       7f
    nop

5:
    # if (proximity < 220) return 1
    li      $t5, 220
    slt     $t6, $t4, $t5
    beq     $t6, $zero, 6f
    nop
    li      $v0, 1
    b       7f
    nop

6:
    # SAFE
    move    $v0, $zero

7:
    lw      $s1, 12($sp)
    lw      $s0, 16($sp)
    lw      $ra, 20($sp)
    addiu   $sp, $sp, 24
    jr      $ra
    nop

# -----------------------------------------------------------------------------
# int32_t mips_should_maneuver(int32_t high_risk_count)
# return 1 if high_risk_count > 0 else 0
# -----------------------------------------------------------------------------
.globl mips_should_maneuver
mips_should_maneuver:
    slt     $t0, $zero, $a0
    move    $v0, $t0
    jr      $ra
    nop
