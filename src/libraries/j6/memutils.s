global memcpy: function (memcpy.end - memcpy)
memcpy:
    mov rax, rdi
    mov rcx, rdx
    rep movsb
    ret
memcpy.end: