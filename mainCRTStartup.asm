_TEXT SEGMENT

extern lotto : proto

PUBLIC mainCRTStartup

mainCRTStartup PROC
    ; check for RDSEED instruction support
    mov     eax, 1
    cpuid
    shr     ecx, 30
    and     ecx, 1
    jz      lotto

_flush_buf: ;Empty the staging buffer
    rdseed  rdx
    jae     _get_seed
    jmp     _flush_buf

_get_seed:  ;Read a freshly minted 64-bit seed
    rdseed  rdx
    jae     _get_seed
    jmp     lotto

mainCRTStartup ENDP

_TEXT ENDS
END