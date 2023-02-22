# trap.S

.macro reg_save base
	sd ra,	  0(\base)
	sd sp,	  8(\base)
	sd gp,	 16(\base)
	sd tp,	 24(\base)
	sd t0,	 32(\base)
	sd t1,	 40(\base)
	sd t2,	 48(\base)
	sd s0,	 56(\base)
	sd s1,	 64(\base)
	sd a0,	 72(\base)
	sd a1,	 80(\base)
	sd a2,	 88(\base)
	sd a3,	 96(\base)
	sd a4,	104(\base)
	sd a5,	112(\base)
	sd a6,	120(\base)
	sd a7,	128(\base)
	sd s2,	136(\base)
	sd s3,	144(\base)
	sd s4,	152(\base)
	sd s5,	160(\base)
	sd s6,	168(\base)
	sd s7,	176(\base)
	sd s8,	184(\base)
	sd s9,	192(\base)
	sd s10,	200(\base)
	sd s11,	208(\base)
	sd t3,	216(\base)
	sd t4,	224(\base)
	sd t5,	232(\base)
	sd t6,	240(\base)
.endm

.macro reg_restore base
	ld ra,	  0(\base)
	ld sp,	  8(\base)
	ld gp,	 16(\base)
	ld tp,	 24(\base)
	ld t0,	 32(\base)
	ld t1,	 40(\base)
	ld t2,	 48(\base)
	ld s0,	 56(\base)
	ld s1,	 64(\base)
	ld a0,	 72(\base)
	ld a1,	 80(\base)
	ld a2,	 88(\base)
	ld a3,	 96(\base)
	ld a4,	104(\base)
	ld a5,	112(\base)
	ld a6,	120(\base)
	ld a7,	128(\base)
	ld s2,	136(\base)
	ld s3,	144(\base)
	ld s4,	152(\base)
	ld s5,	160(\base)
	ld s6,	168(\base)
	ld s7,	176(\base)
	ld s8,	184(\base)
	ld s9,	192(\base)
	ld s10,	200(\base)
	ld s11,	208(\base)
	ld t3,	216(\base)
	ld t4,	224(\base)
	ld t5,	232(\base)
	ld t6,	240(\base)
.endm

.global trap_vector

.section .text
# the trap vector base address must always be aligned on a 4-byte boundary
.align 4
trap_vector:
	# save context.
	addi sp, sp, -248	# make room to save registers.
	reg_save sp

	# prepare the argu of C function trap_handler
	csrr	a0, sepc
	csrr	a1, scause
	call	trap_handler

	# trap_handler will return the return address via a0.
	csrw	sepc, a0

	reg_restore sp
	addi sp, sp, 248

	sret