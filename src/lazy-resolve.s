
.text
    .global outerLazyResolve

outerLazyResolve:
    # pushl PLT1
    # pushl PLT2
    pushl %eax              # Preserve registers otherwise clobbered.
    pushl %ecx
    pushl %edx

   pushl 16(%esp)
   pushl 12(%esp)

#    pushl %edx
#    pushl %eax

    call lazyResolve        # Call resolver.
    add $8, %esp


    movl %eax, 16(%esp)
    popl %edx
    popl %ecx
    popl %eax 
    add $4, %esp
    ret
