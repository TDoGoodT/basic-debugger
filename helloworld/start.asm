.global _start

.text
_start:
	call main

	# exit(0)
	mov $60, %rax
	xor %rdi, %rdi
	syscall

