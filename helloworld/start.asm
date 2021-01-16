.global _start

.text
_start:
	xor %rax, %rax
	call main

	# exit(0)
	mov $60, %rax
	xor %rdi, %rdi
	syscall

