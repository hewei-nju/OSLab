;变量区
SECTION .bss
str:    resb    50              ;存储输入
addRes: resb    42              ;加法结果
mulRes: resb    42              ;乘法结果
tmpMul: resb    42              ;乘法中间结果

;数据区
SECTION .data
msg db  "Please input x and y:", 0Ah, 0 ;0Ah是换行符的ASCII码的16进制表示，0的目的是避免两个字符串同时输出
bugs    db  "Bugs occured!", 0Ah, 0     ;作为调试信息
addrX   dd  0               ;X的地址
lenX    dd  0               ;X的长度
addrY   dd  0               ;Y的地址
lenY    dd  0               ;Y的长度
addrZ   dd  0               ;保存部分积的首地址
addrW   dd  0               ;保存上次部分和的计算结果的首地址
lenADD  dd  41              ;加法结果可用长度
lenMUL  dd  41              ;乘法结果可用长度
carry   db  0               ;进位
numZero dd  0               ;乘法中间结果末尾要补充的0的个数
lenT    dd  41              ;乘法中间结果可用长度
;===============说明========================
;lenADD = 41，加法从最低位开始进行，并且末尾有'\0'；
;lenMUL = 41，同理！


;主函数
SECTION .text
global _start

_start:
;===============输出提示====================    OK
    mov eax, msg
    call    sprint

;===============获取输入====================    OK
    mov eax, str
    mov ebx, 50
    call    getline

;===============格式化输入================== OK
    mov eax, str
    call    format
    
;===============进行加法、输出加法结果======   OK
    call    ADD
    call    sprintln
    
;===============将addRes内容全部恢复为最初模样  OK
    mov eax, dword[lenADD]

recover:
    mov ebx, addRes
    cmp eax, -1
    je  fine
    
    mov byte[eax + ebx], 0
    dec eax
    jmp recover
fine:
    
;=======恢复在计算过程中改变的数值'\001'->'1'    OK
    mov ebx, dword[addrX]
    mov ecx, dword[lenX]
    dec ecx
RecoverX:
    cmp ecx, -1
    je  OkX
    add byte[ebx + ecx], 48
    dec ecx
    jmp RecoverX
OkX:
    mov ebx, dword[addrY]
    mov ecx, dword[lenY]
    dec ecx
RecoverY:
    cmp ecx, -1
    je  OkY
    add byte[ebx + ecx], 48
    dec ecx
    jmp RecoverY
    
OkY:
;===============进行乘法、输出乘法结果======   OK
    call    MUL
    call    sprintln
;===============正常退出=================== OK
    call    exit
    

;进行两个正数的乘法 a * b
MUL:    

    ;=======获取lenX=========
    mov eax, dword[addrX]
    call    strlen
    mov ebx, lenX
    mov dword[ebx], eax
    
    
    ;====edit----------
    cmp eax, 1
    jne CONTINUE
    mov eax, dword[addrX]
    cmp byte[eax], '0'
    jne CONTINUE
    call    sprintln
    call    exit
    

CONTINUE:   
    
    ;-------------------

    ;=======获取lenY=========
    mov eax, dword[addrY]
    call    strlen
    mov ebx, lenY
    mov dword[ebx], eax
    
    ;=======测试lenMUL是否长为41
    cmp dword[lenMUL], 41
    jne BUGS
    
    ;=======addrX保存X的起始位置
    ;=======addrY保存Y的起始位置
    ;=======addrZ保存中间结果的起始位置
    ;=======addrW保存上次加法结果的起始位置
    ;=======给addrw赋值"0\0"，便于后续计算
    ;让addrW保存
    mov eax, mulRes
    mov ebx, dword[lenMUL]
    add eax, ebx
    mov byte[eax], 0
    dec eax
    mov byte[eax], '0'
    mov ecx, addrW
    mov dword[ecx], eax
    
    ;edx保存lenY
    mov edx, lenY
    mov edi, dword[edx]
    dec edi
    mov dword[edx], edi         ;指向Y的最低有效位

MulLoop:
    ;=======如果lenY >= 0，继续进行乘法
    push    edx
    
    mov edx, dword[lenY]
    cmp edx, -1
    je  MulOver
    
    pop edx
    
    pusha
    ;=======addrX保存X的起始位置
    ;=======addrY保存Y的起始位置
    ;=======addrZ保存中间结果的起始位置
    ;=======addrW保存上次加法结果的起始位置
    
    ;将中间乘积结果的首地址保存在addrZ
    call    GetTmpMulRes
    
    
    ;将原先的X、lenX，Y和lenY保存
    mov eax, dword[addrX]
    push    eax
    mov ebx, dword[lenX]
    push    ebx
    mov ecx, dword[addrY]
    push    ecx
    mov edx, dword[lenY]
    push    edx
    ;===================================
    
    
    ;进行加法计算中间和
    ;================ERROR============== FIX
    ;{mov   dword[addrX], dword[addrZ]}
    ;{mov   dword[addrY], dword[addrW]}
    push    eax
    push    ebx
    
    mov eax, addrX
    mov ebx, dword[addrZ]
    mov dword[eax], ebx
    
    mov eax, addrY
    mov ebx, dword[addrW]
    mov dword[eax], ebx
    
    pop ebx
    pop eax
    ;===================================
    pusha
    call    ADD
    ;addrX与addrY的计算结果的起始位置保存在eax中
    ;=======使用addrW保存加法中间和的起始地址值
    mov ebx, addrW
    mov dword[ebx], eax
    popa
    
    ;还原lenY,addrY,lenX,addrX
    pop edx
    mov dword[lenY], edx
    pop ecx
    mov dword[addrY], ecx
    pop ebx
    mov dword[lenX], ebx
    pop eax
    mov dword[addrX], eax
    ;====================================
    popa
    jmp MulLoop
    
GetTmpMulRes:
    ;=======获取一次中间乘积，将结果的起始位置保存在addrZ
    ;tmpMul用来存储中间结果
    mov eax, tmpMul
    add eax, dword[lenT]
    ;给tmpMul末尾添加'\0'
    mov byte[eax], 0
    dec eax
    
    ;判断Y的当前位是否是0
    mov ecx, dword[addrY]
    add ecx, dword[lenY]
    cmp byte[ecx], '0'
    je  YBitIsZero
    
    mov ebx, dword[numZero]
    cmp ebx, 0
    jne InitZeroLoop
    jmp FinishedInit

YBitIsZero:
    ;下次末尾置0的数量需要+1
    mov ebx, dword[numZero]
    inc ebx
    mov edx, numZero
    mov dword[edx], ebx
    
    ;将lenY移向前一个有效数NONONO
    mov edx, dword[lenY]
    dec edx
    mov dword[lenY], edx
    
    inc eax
    mov ebx, addrZ
    mov dword[ebx], eax
    
    ret

InitZeroLoop:
    ;=======给中间结果末尾添加0
    cmp ebx, 0
    je  FinishedInit
    mov byte[eax], '0'
    dec eax
    dec ebx
    jmp InitZeroLoop
    
FinishedInit:
    ;=======完成末尾置0操作
    ;下次末尾置0的数量要+1
    pusha
    mov ebx, dword[numZero]
    inc ebx 
    mov dword[numZero], ebx
    popa
    
    ;使用ebx存储X的可计算长度(从最低位->最高位)
    mov ebx, dword[lenX]
    dec ebx             ;指向最低位
    
    ;使用ecx存储当前Y需要计算部分积的位的地址
    mov ecx, dword[addrY]
    add ecx, dword[lenY]
    
    ;将lenY移向前一个有效数NONONO
    mov edx, dword[lenY]
    dec edx
    mov dword[lenY], edx
    
    ;使用esi保存进位
    mov esi, 0
    
TmpMulLoop:
    cmp ebx, -1
    je  CheckCarryMul

    movzx   edi, byte[ecx]
    sub edi, 48
    
    ;=======获取当前X的位的地址=====
    push    edi
    mov edi, dword[addrX]
    add ebx, edi
    pop edi
    ;========================
    
    ;将X的当前位存储在edx中
    ;===================ERROR========== FIX
    ;{mov   byte[edx], byte[ebx]}
    movzx   edx, byte[ebx]
    ;==================================
    ;=======还原lenX=========
    dec ebx
    sub ebx, dword[addrX]
    ;========================
    
    sub edx, 48
    ;============乘法{mul edx, edi}====  SIGSEGV
    push    eax
    mov al, dl
    ;mov    al, [edx]
    
    ;===================
    ;mul    byte[edi]
    push    edx
    mov edx, edi
    mul dl
    pop edx
    ;====================
    
    mov dx, ax
    pop eax
    ;====================================
    add edx, esi
    cmp edx, 10
    jnb MulCarry
    add edx, 48
    ;=================ERROR============= FIX
    ;{mov   byte[eax], byte[edx]}
    mov byte[eax], dl
    ;===================================
    dec eax
    mov esi, 0
    jmp TmpMulLoop
    
    
MulCarry:
    ;存在进位
    ;push   edx
    push    ecx
    push    ebx
    
    push    eax
    
    ;mov    eax, esi换成了下面一行
    mov eax, edx
    mov edx, 0
    
    mov ebx, 10
    div ebx
    ;mov    esi, eax;目的是保存进位
    movzx   esi, al
    add edx, 48
    
    pop eax
    ;=================ERROR============= FIX
    ;{mov   byte[eax], byte[edx]}
    mov byte[eax], dl
    ;===================================
    dec eax
    
    pop ebx
    pop ecx
    ;pop    edx
    jmp TmpMulLoop
    

CheckCarryMul:
    ;移动到Y的下一位
    dec ecx
    ;=======此时已经计算完，判断是否还有进位
    cmp esi, 0
    je  TmpMulOver
    add esi, 48
    ;=================ERROR================
    ;{mov   byte[eax], byte[esi]}
    push    ebx
    
    mov ebx, esi
    mov byte[eax], bl
    
    pop ebx
    ;======================================
    dec eax
    mov esi, 0
    jmp TmpMulOver
    
TmpMulOver:
    ;将部分积的首地址保存到addrZ
    inc eax
    
    push    esi
    mov esi, addrZ
    mov dword[esi], eax
    pop esi
    
    ret 
    

    
MulOver:
    pop edx
    ;检查是否有进位
    cmp esi, 0
    jne lastStep
    inc edi
    mov eax, dword[addrW]
    ret
    
lastStep:
    ;=================ERROR================
    ;{mov   byte[eax], byte[esi]}
    push    ebx
    
    mov ebx, esi
    mov byte[eax], bl
    
    pop ebx
    ;======================================
    inc eax
    mov esi, 0
    jmp MulOver



;进行两个正数的加法*addrX + *addrY
ADD:
    ;eax用来作为返回值
    push    esi
    push    edi
    push    edx
    push    ecx
    push    ebx
    ;=======获取lenX=========
    mov eax, dword[addrX]
    call    strlen
    mov ebx, lenX
    mov dword[ebx], eax
    ;=======获取lenY=========
    mov eax, dword[addrY]
    call    strlen
    mov ebx, lenY
    mov dword[ebx], eax
    ;=======测试lenADD是否长为41
    cmp dword[lenADD], 41
    jne BUGS
    
    ;=======edi保存addRes的最后一个位置
    ;=======esi保存carry
    ;=======eax保存X的起始地址
    ;=======ebx保存lenX
    ;=======ecx保存Y的起始地址
    ;=======edx保存lenY的起始地址
    ;=======给addRes最末尾添加'\0'    
    
    mov eax, dword[addrX]
    mov ebx, dword[lenX]
    dec ebx             ;指向X的最低有效位
    mov ecx, dword[addrY]
    mov edx, dword[lenY]
    dec edx             ;指向Y的最低有效位
    movzx   esi, byte[carry]        ;无符号扩展
    mov edi, addRes
    add edi, dword[lenADD]
    mov byte[edi], 0
    dec edi
    
    
AddLoop:
    ;=======48->'0',57->'9'
    cmp ebx, -1             ;X已经计算完
    je  GoY
    cmp edx, -1             ;Y已经计算完
    je  GoX
    sub byte[eax + ebx], 48
    sub byte[ecx + edx], 48
    ;===================================================
    ;代码块功能为{add byte[esi], byte[eax + ebx]}
    push    eax
    movzx   eax, byte[eax + ebx]
    add esi, eax
    pop eax
    ;===================================================
    ;代码块功能为{add byte[esi], byte[ecx + edx]}
    push    eax
    movzx   eax, byte[ecx + edx]
    add esi, eax
    pop eax
    ;===================================================
    dec ebx
    dec edx
    cmp esi, 10
    jnb CarryOne
    ;=======恢复正常ASCII码表示
    add esi, 48
    ;====================================================
    ;代码块功能为{mov byte[edi], byte[esi]}
    push    eax
    mov eax, esi
    mov byte[edi], al
    pop eax
    ;====================================================
    dec edi
    mov esi, 0
    jmp AddLoop
    
CarryOne:
    ;=======进位为1
    sub esi, 10
    add esi, 48
    
    ;call   BUGS
    ;====================================================
    ;代码块功能为{mov byte[edi], byte[esi]}
    push    eax
    mov eax, esi
    mov byte[edi], al
    pop eax
    ;====================================================
    dec edi
    mov esi, 1
    jmp AddLoop
    
GoX:    
    ;=======Y已经计算完，计算X和carry
    cmp ebx, -1
    je  AddCheckCarry
    sub byte[eax + ebx], 48
    ;===================================================
    ;代码块功能为{add byte[esi], byte[eax + ebx]}
    push    eax
    movzx   eax, byte[eax + ebx]
    add esi, eax
    pop eax
    ;===================================================
    dec ebx
    cmp esi, 10
    jnb CarryOneX
    add esi, 48
    ;===================================================
    ;代码块功能为{mov byte[edi], byte[esi]}
    push    eax
    mov eax, esi
    mov byte[edi], al
    pop eax
    ;===================================================
    dec edi
    mov esi, 0
    jmp GoX
    
CarryOneX:
    ;=======进位为1,且仅有X还没计算完
    sub esi, 10
    add esi, 48
    ;===================================================
    ;代码块功能为{mov byte[edi], byte[esi]}
    push    eax
    mov eax, esi
    mov byte[edi], al
    pop eax
    ;===================================================
    dec edi
    mov esi, 1
    jmp GoX

GoY:
    ;=======X已计算完，计算Y和carry
    cmp edx, -1
    je  AddCheckCarry
    sub byte[ecx + edx], 48
    ;====================================================
    ;代码块功能为{add byte[esi], byte[ecx + edx]}
    push    eax
    movzx   eax, byte[ecx + edx]
    add esi, eax
    pop eax
    ;====================================================
    dec edx
    cmp esi, 10
    jnb CarryOneY
    add esi, 48
    ;====================================================
    ;代码块功能为{mov byte[edi], byte[esi]}
    push    eax
    mov eax, esi
    mov byte[edi], al
    pop eax
    ;====================================================
    dec edi
    mov esi, 0
    jmp GoY
    
CarryOneY:
    ;=======进位为1,且仅有Y还没计算完
    sub esi, 10
    add esi, 48
    ;======================================================
    ;代码块功能为{mov byte[edi], byte[esi]}
    push    eax
    mov eax, esi
    mov byte[edi], al
    pop eax
    ;======================================================
    dec edi
    mov esi, 1
    jmp GoY
    
    
AddCheckCarry:
    cmp esi, 0
    jz  AddOver
    mov byte[edi], '1'
    dec edi
    jz  AddOver

AddOver:
    inc edi
    mov eax, edi
    
    pop ebx
    pop ecx
    pop edx
    pop edi
    pop esi
    
    ret


;用来报告哪块代码区出现了bug
BUGS:
    mov eax, bugs
    call    sprintln
    call    exit


;格式化输入：将str="a b" ===> addrX=&"a", addrY=&"b"
format:
    mov edi, str
    mov dword[addrX], edi   ;addrX = X的首地址

loop1:  

    cmp byte[edi], 32       ;' '
    je  next
    cmp byte[edi], 0Ah      ;'LF'
    je  ok
    inc edi
    jmp loop1
    
next:   
    mov byte[edi], 0
    inc edi         ;指向Y的首地址
    mov dword[addrY], edi
    jmp loop1

ok:
    mov byte[edi], 0        ;'LF' --> '\0'  
    ret


;从标准输入中读取一行
;edx:存放变量大小
;ecx:存放变量地址
;ebx:存放获取输入的文件描述符 1表示STDIO
;eax:存放系统调用号，3表示SYS_READ
;默认字符串首地址存放在eax
;变量大小存放在ebx中
getline:
    push    edx
    push    ecx
    push    ebx
    push    eax
    
    mov edx, ebx
    mov ecx, eax
    mov ebx, 1
    mov eax, 3
    int 80h
    
    pop eax
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

;退出程序
exit:
    mov ebx, 0      ;0是_start函数的返回值
    mov eax, 1      ;1是SYS_EXIT的系统调用号
    int 80h     ;中断
    ret
