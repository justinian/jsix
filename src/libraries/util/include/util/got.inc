extern _GLOBAL_OFFSET_TABLE_

; Put the address of the given symbol in rax
%macro lookup_GOT 1
    lea rax, [rel _GLOBAL_OFFSET_TABLE_]
    mov rax, [rax + %1 wrt ..got]
%endmacro