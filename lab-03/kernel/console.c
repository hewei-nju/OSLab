
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                  console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
    回车键: 把光标移到第一列
    换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PUBLIC void flush(CONSOLE *p_con);

// TODO:
PUBLIC void cleanScreen(TTY *p_tty) {
    u8 *p_vmem = (u8 *)(V_MEM_BASE + p_tty->p_console->original_addr * 2);

    for (int i = 0; i < p_tty->p_console->v_mem_limit; i++) {
        *(p_vmem++) = '\0';
        *(p_vmem++) = DEFAULT_CHAR_COLOR;
    }
    p_tty->p_console->cursor = p_tty->p_console->original_addr;
    set_cursor(p_tty->p_console->cursor);

    p_tty->p_console->cnt = 0;
    p_tty->p_console->lfcnt = 0;
    p_tty->p_console->opcnt = 0;
}

/*======================================================================*
               init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty) {
    int nr_tty = p_tty - tty_table;
    p_tty->p_console = console_table + nr_tty;

    int v_mem_size = V_MEM_SIZE >> 1;  /* 显存总大小 (in WORD) */

    int con_v_mem_size = v_mem_size / NR_CONSOLES;
    p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
    p_tty->p_console->v_mem_limit = con_v_mem_size;
    p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

    /* 默认光标位置在最开始处 */
    p_tty->p_console->cursor = p_tty->p_console->original_addr;

    if (nr_tty == 0) {
        /* 第一个控制台沿用原来的光标位置 */
        p_tty->p_console->cursor = disp_pos / 2;
        disp_pos = 0;
    } else {
        out_char(p_tty->p_console, nr_tty + '0');
        out_char(p_tty->p_console, '#');
    }
    set_cursor(p_tty->p_console->cursor);

    // hewei 记录打印在屏幕上的内容
    p_tty->p_console->cnt = 0;
    p_tty->p_console->lfcnt = 0;
    p_tty->p_console->opcnt = 0;
}

/*======================================================================*
               is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con) {
    return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
               out_char
 *======================================================================*/

/*
+-----------------80--------------------+ '\n'
|                                       |
|                                       |
|                                       |
25                                      |
|                                       |
|                                       |
|                                       |
+---------------------------------------+ '\n'
WIDTH * HEIGHT

所以鼠标的位置最多到：p_con->original_addr + 该终端显存的大小；
如果前已经在最后一行了，则键入换行符不会产生任何效果；
否则则换行；
因此通过 p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH 来作为判断条件
*/

PUBLIC void out_char(CONSOLE *p_con, char ch) {
    u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

    switch (ch) {
    case (ESC - FLAG_EXT):
        if (!searchMode) {
            while (kwlen > 0) {
                if (keyword[kwlen - 1] == '\t') {
                    for (int i = 0; i < 4; i++) {
                        p_con->cursor--;
                        *(--p_vmem) = DEFAULT_CHAR_COLOR;
                        *(--p_vmem) = ' ';
                    }
                } else {
                    p_con->cursor--;
                    *(--p_vmem) = DEFAULT_CHAR_COLOR;
                    *(--p_vmem) = ' ';
                }
                keyword[kwlen--] = '\0';
            }
            u8 *vmem = p_vmem - p_con->cursor * 2 + 1;
            while (vmem < p_vmem) {
                *vmem = DEFAULT_CHAR_COLOR;
                vmem += 2;
            }
            // disp_str("RED");
        }
        break;
    case '\n':
        if (!searchMode) {
            if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
                int n = p_con->cursor;
                p_con->cursor = p_con->original_addr + SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);

                n = p_con->cursor - n;
                // hewei
                p_con->log[p_con->cnt++] = '\n';
                p_con->nums[p_con->lfcnt++] = n;
                p_con->op[p_con->opcnt] = '\n';
                p_con->opcode[p_con->opcnt] = '+';
                p_con->opnums[p_con->opcnt++] = n;
            }
        } else {
            // TODO 进行字符串匹配 操作显存
            for (int i = 0, j = 0, k = 0; i < p_con->cnt && j < kwlen;) {
                if (p_con->log[i] == '\n') {
                    i += 1, j = 0, k = 0;
                    continue;
                }
                if (p_con->log[i + k] == keyword[j]) {
                    k++, j++;
                } else {
                    i += 1, k = 0, j = 0;
                }
                if (j == kwlen) {
                    int pos = 0;
                    for (int idx = 0; idx < i; idx++) {
                        if (p_con->log[idx] == '\n') {
                            pos += SCREEN_WIDTH - (pos - (pos / SCREEN_WIDTH) * SCREEN_WIDTH);
                        } else if (p_con->log[idx] == '\t') {
                            pos += 4;
                        } else {
                            pos += 1;
                        }
                    }
                    pos = pos * 2 + 1;

                    i += k;

                    u8 *vmem = (u8 *)(V_MEM_BASE + p_con->original_addr * 2);
                    j = 0;
                    while (k-- > 0) {
                        if (keyword[j++] == '\t') {
                            for (int idx = 0; idx < 3; idx++) {
                                *(vmem + pos) = RED;
                                pos += 2;
                            }
                        }
                        *(vmem + pos) = RED;
                        pos += 2;
                    }

                    k = 0, j = 0;
                }
            }
        }
        break;
    case '\b':
        if (!searchMode) {
            if (p_con->cursor > p_con->original_addr && p_con->cnt > 0) {
                if (p_con->log[p_con->cnt - 1] == '\n') {
                    // TODO:
                    p_con->opnums[p_con->opcnt] = p_con->nums[p_con->lfcnt - 1];
                    p_con->op[p_con->opcnt] = '\n';
                    p_con->opcode[p_con->opcnt++] = '-';

                    while (p_con->nums[p_con->lfcnt - 1]-- > 0) {
                        p_con->cursor--;
                        *(--p_vmem) = DEFAULT_CHAR_COLOR;
                        *(--p_vmem) = ' ';
                    }
                    p_con->lfcnt -= 1;

                } else if (p_con->log[p_con->cnt - 1] == '\t') {
                    for (int i = 0; i < 4; i++) {
                        p_con->cursor--;
                        *(--p_vmem) = DEFAULT_CHAR_COLOR;
                        *(--p_vmem) = ' ';
                    }

                    p_con->opnums[p_con->opcnt] = 4;
                    p_con->op[p_con->opcnt] = '\t';
                    p_con->opcode[p_con->opcnt++] = '-';
                } else {
                    p_con->cursor--;
                    *(--p_vmem) = DEFAULT_CHAR_COLOR;
                    *(--p_vmem) = ' ';

                    p_con->op[p_con->opcnt] = p_con->log[p_con->cnt - 1];
                    p_con->opcode[p_con->opcnt++] = '-';
                }

                p_con->cnt -= 1;
            }
        } else {
            if (kwlen > 0) {
                if (keyword[kwlen - 1] == '\t') {
                    for (int i = 0; i < 4; i++) {
                        p_con->cursor--;
                        *(--p_vmem) = DEFAULT_CHAR_COLOR;
                        *(--p_vmem) = ' ';
                    }
                } else {
                    p_con->cursor--;
                    *(--p_vmem) = DEFAULT_CHAR_COLOR;
                    *(--p_vmem) = ' ';
                }
                keyword[kwlen--] = '\0';
            }
        }
        break;
    case '\t':
        // TODO tab = 4 blanck
        if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 4) {
            for (int i = 0; i < 4; i++) {
                *p_vmem++ = ' ';
                *p_vmem++ = DEFAULT_CHAR_COLOR;
                p_con->cursor++;
            }

            // hewei
            if (!searchMode) {
                p_con->log[p_con->cnt++] = '\t';

                // TODO:
                p_con->opnums[p_con->opcnt] = 4;
                p_con->op[p_con->opcnt] = '\t';
                p_con->opcode[p_con->opcnt++] = '+';
            } else {
                keyword[kwlen++] = '\t';
            }
        }
        break;
    default:
        if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
            *p_vmem++ = ch;
            *p_vmem++ = DEFAULT_CHAR_COLOR;
            p_con->cursor++;

            // hewei
            if (!searchMode) {
                p_con->log[p_con->cnt++] = ch;

                p_con->opnums[p_con->opcnt] = 1;
                p_con->op[p_con->opcnt] = ch;
                p_con->opcode[p_con->opcnt++] = '+';
            } else if (searchMode) {
                keyword[kwlen++] = ch;
                *(p_vmem - 1) = RED;
            }
        }
        break;
    }

    while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
        scroll_screen(p_con, SCR_DN);
    }

    flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
// PRIVATE void flush(CONSOLE *p_con) {
    PUBLIC void flush(CONSOLE *p_con) {
    set_cursor(p_con->cursor);
    set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
                set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position) {
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, position & 0xFF);
    enable_int();
}

/*======================================================================*
              set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr) {
    disable_int();
    out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, addr & 0xFF);
    enable_int();
}

/*======================================================================*
               select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
        return;
    }

    nr_current_console = nr_console;

    set_cursor(console_table[nr_console].cursor);
    set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
               scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
    SCR_UP	: 向上滚屏
    SCR_DN	: 向下滚屏
    其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction) {
    if (direction == SCR_UP) {
        if (p_con->current_start_addr > p_con->original_addr) {
            p_con->current_start_addr -= SCREEN_WIDTH;
        }
    } else if (direction == SCR_DN) {
        if (p_con->current_start_addr + SCREEN_SIZE < p_con->original_addr + p_con->v_mem_limit) {
            p_con->current_start_addr += SCREEN_WIDTH;
        }
    } else {
    }

    set_video_start_addr(p_con->current_start_addr);
    set_cursor(p_con->cursor);
}
