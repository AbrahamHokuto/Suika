.text

.global swap_context
.type swap_context, @function
.align 16

# std::uint64_t swap_context(rdi first_arg, rsi ret_or_second_arg, rdx target_sp, rcx *self_sp)
swap_context:
	subq $48, %rsp
	movq %r12, (%rsp)
	movq %r13, 8(%rsp)
	movq %r14, 16(%rsp)
	movq %r15, 24(%rsp)
	movq %rbx, 32(%rsp)
	movq %rbp, 40(%rsp)

	movq %rsp, (%rcx)
	movq %rdx, %rsp

	movq 40(%rsp), %rbp
	movq 32(%rsp), %rbx
	movq 24(%rsp), %r15
	movq 16(%rsp), %r14
	movq 8(%rsp),  %r13
	movq (%rsp), %r12
	addq $48, %rsp

	movq %rsi, %rax
	retq

.global make_context
.type make_context, @function
.align 16

# void make_context(rdi *sp, rsi entry_addr)
make_context:
	movq (%rdi), %rcx
	subq $56, %rcx
	movq %rsi, 48(%rcx)
	movq %rcx, (%rdi)
	retq
