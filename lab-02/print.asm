section .data
RED:			db 1bh, "[31m", 0
RESET:          db 1bh, "[0m", 0

section .text
	global PRINT_RED
	global PRINT_WHITE

PRINT_RED:
    mov eax, RED

    call sprint
    mov eax, [esp+4]
    call sprint

    mov eax, RESET
    call sprint
    ret

PRINT_WHITE:
    mov eax, [esp+4]
    call sprint
    ret

;向标准输出输出字符串的内容，并输出换行符
;默认字符串首地址存放在eax
sprintln:
    call    sprint

    push    eax
    mov eax, 0Ah
    push    eax
    mov eax, esp
    call    sprint
    pop eax
    pop eax
    ret

;向标准输出输出字符串
;默认字符串首地址存放在eax
sprint:
    push    edx
    push    ecx
    push    ebx
    push    eax
    call    strlen

    mov edx, eax
    pop eax

    mov ecx, eax
    mov ebx, 1
    mov eax, 4
    int 80h

    pop ebx
    pop ecx
    pop edx
    ret

;计算字符串长度
;默认字符串首地址存放在eax
strlen:
    push    ebx
    mov ebx, eax

nextchar:
    cmp byte[eax], 0
    jz  finished
    inc eax
    jmp nextchar

finished:
    sub eax, ebx
    pop ebx
    ret

