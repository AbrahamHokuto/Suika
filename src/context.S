.text

.global swap_context
.type swap_context, @function
.align 16

# std::uint64_t swap_context(rdi first_arg, rsi ret_or_second_arg, rdx target_sp, rcx *self_sp)
swap_context:
	leaq -0x38(%rsp), %rsp

	stmxcsr (%rsp)
	fnstcw  0x4(%rsp)
	
	movq %r12, 0x8(%rsp)
	movq %r13, 0x10(%rsp)
	movq %r14, 0x18(%rsp)
	movq %r15, 0x20(%rsp)
	movq %rbx, 0x28(%rsp)
	movq %rbp, 0x30(%rsp)

	movq %rsp, (%rcx)
	movq %rdx, %rsp

	movq 0x38(%rsp), %r8

	ldmxcsr   (%rsp)
	fldcw  0x4(%rsp)

	movq 0x8(%rsp),  %r12
	movq 0x10(%rsp), %r13
	movq 0x18(%rsp), %r14
	movq 0x20(%rsp), %r15
	movq 0x28(%rsp), %rbx
	movq 0x30(%rsp), %rbp

	leaq 0x40(%rsp), %rsp

	movq %rsi, %rax

	jmp *%r8

.global make_context
.type make_context, @function
.align 16

# void* make_context(rdi sp, rsi entry_addr)
make_context:
	movq %rdi, %rax
	andq $-16, %rax

	leaq -0x40(%rax), %rax

	movq %rsi, 0x28(%rax)

	stmxcsr (%rax)
	fnstcw 0x4(%rax)

	leaq trampoline(%rip), %rcx
	movq %rcx, 0x38(%rax)

	leaq finish(%rip), %rcx
	movq %rcx, 0x30(%rax)

	ret

trampoline:
	push %rbp
	jmp *%rbx

finish:
	xorq %rdi, %rdi
	call _exit@PLT
	hlt
