
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                  console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

/* CONSOLE */
typedef struct s_console {
    unsigned int current_start_addr; /* 当前显示到了什么位置	  */
    unsigned int original_addr;      /* 当前控制台对应显存位置 */
    unsigned int v_mem_limit;        /* 当前控制台占的显存大小 */
    unsigned int cursor;             /* 当前光标位置 */

    // hewei
    // 记录当前屏幕上打印的内容，便于删除
    int  cnt;                                   // log 的数量
    int  lfcnt;                                 // nums 的数量
    char log[(V_MEM_SIZE >> 2) / NR_CONSOLES];  // 字符
    int  nums[(V_MEM_SIZE >> 2) / NR_CONSOLES]; // 换行符对应的空格数
    int  opcnt;                                 // 操作次数
    char op[10000];                             // 对应操作的字符
    char opcode[10000];                         // 对应操作码 '+'表示输出，'-'表示删除
    int  opnums[10000];                         // 记录对应操作的符号的数量
} CONSOLE;

#define SCR_UP 1  /* scroll forward */
#define SCR_DN -1 /* scroll backward */

#define SCREEN_SIZE (80 * 25)
#define SCREEN_WIDTH 80

#define DEFAULT_CHAR_COLOR 0x07 /* 0000 0111 黑底白字 */

#endif /* _ORANGES_CONSOLE_H_ */
