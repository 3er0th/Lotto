_TEXT SEGMENT

extern lotto : proto

PUBLIC mainCRTStartup

mainCRTStartup PROC

_flush_buf: ;Empty the staging buffer
    rdseed  rcx
    jae     _get_seed
    jmp     _flush_buf

_get_seed:  ;Read a freshly minted 64-bit seed
    rdseed  rcx
    jae     _get_seed
    jmp     lotto

mainCRTStartup ENDP

_TEXT ENDS
END