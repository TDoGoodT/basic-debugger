.global main

.data
helloworld: .ascii "Hello ATAM\n"
helloworldend:
goodluck: .asciz "Good Luck!\n"
goodluckend:

.text
main:
    # printf(helloworld)
    movq $1, %rax
    movq $1, %rdi
    movq $helloworld, %rsi
    movq $helloworldend-helloworld, %rdx
    syscall

    # printf(goodluck)
    movq $1, %rax
    movq $1, %rdi
    movq $goodluck, %rsi
    movq $goodluckend-goodluck, %rdx
    syscall

    xorq %rax, %rax
    ret
