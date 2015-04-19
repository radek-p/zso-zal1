

.text
    # pushl PLT1
    # pushl PLT2
    pushl %eax              # Preserve registers otherwise clobbered.
    pushl %ecx
    pushl %edx

    movl 16(%esp), %edx
    movl 12(%esp), %eax

    pushl %edx
    pushl %eax

    call lazyResolve        # Call resolver.

    movl %eax, 24(%esp)

    movl 20(%esp), %eax
    movl 16(%esp), %ecx
    movl 12(%esp), %edx

    ret $24                 # Jump to function address. 20 28?
