_TEXT SEGMENT

extern lotto : proto

PUBLIC mainCRTStartup

mainCRTStartup PROC
    mov     eax, 1      ; check for RDSEED instruction support
    cpuid
    shr     ecx, 30
    and     ecx, 1
    jz      lotto

_flush_buf:             ; empty the staging buffer
    rdseed  rdx
    jae     _get_seed   ; buffer is empty, proceed to _get_seed
    jmp     _flush_buf  ; not empty, try again

_get_seed:              ; read a freshly-minted 64-bit seed
    rdseed  rdx
    jae     _get_seed   ; seed unavailable, try again
    jmp     lotto

mainCRTStartup ENDP

_TEXT ENDS
END