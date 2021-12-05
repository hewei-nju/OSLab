
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8   in_byte(u16 port);
PUBLIC void disp_str(char *info);
PUBLIC void disp_color_str(char *info, int color);

/* protect.c */
// 初始化保护模式
PUBLIC void init_prot();
// 段地址转物理地址
PUBLIC u32 seg2phys(u16 seg);

/* klib.c */
// 延迟时间
PUBLIC void delay(int time);

/* kernel.asm */
// 初始化进程表后，操作系统重启，也是启动第一个进程的入口
void restart();

/* main.c */
// 三个进程
void TestA();
void TestB();
void TestC();

/* i8259.c */
// 设定中断号为 irq 的中断处理程序为 handler
PUBLIC void put_irq_handler(int irq, irq_handler handler);
// 伪中断，只是打印中断号
PUBLIC void spurious_irq(int irq);

/* clock.c */
// 时钟中断处理程序，irq 相当于一个摆设
PUBLIC void clock_handler(int irq);
// 初始化时钟
PUBLIC void init_clock();

/* keyboard.c */
// 初始化键盘
PUBLIC void init_keyboard();

/* tty.c */
// 执行 tty 的任务
PUBLIC void task_tty();
//
PUBLIC void in_process(TTY *p_tty, u32 key);

/* console.c */
// 在屏幕 p_con 上打印字符 ch
PUBLIC void out_char(CONSOLE *p_con, char ch);
// 华东屏幕
PUBLIC void scroll_screen(CONSOLE *p_con, int direction);

/* 以下是系统调用相关 */

/* proc.c */
PUBLIC int sys_get_ticks(); /* sys_call */

/* syscall.asm */
PUBLIC void sys_call(); /* int_handler */
PUBLIC int  get_ticks();
