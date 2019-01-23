#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Instruction level single step", cmd_si },
  { "info", "Print program status", cmd_info },
  { "x", "Read from the memory of the current target program", cmd_x },
  { "p", "Print value of expression EXP", cmd_p },
  { "w", "Set a watchpoint for an expression", cmd_w },
  { "d", "Delete a watchpoint", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
    char *arg = strtok(NULL, " ");
    int n;
    
    if (arg == NULL) {
        n = 1;
    }
    else {
        n = atoi(arg);
        if (n <= 0) {
            printf("The 1st argument should be a postive integer rather than '%s'\n", arg);
            return 0;
        }
    }
    cpu_exec(n);
    return 0;
}

static int cmd_info(char *args) {
    char *arg = strtok(NULL, " ");

    if (arg == NULL) {
        printf("Missing subcommand.\nNow supports 'r'.\n");
        return 0;
    }
    else {
        if (strcmp(arg, "r") == 0) {
            // CPU_state cpu;
            printf("eax\t0x%1$x\t%1$d\n"
                    "ecx\t0x%2$x\t%2$d\n"
                    "edx\t0x%3$x\t%3$d\n"
                    "ebx\t0x%4$x\t%4$d\n"
                    "esp\t0x%5$x\t%5$d\n"
                    "ebp\t0x%6$x\t%6$d\n"
                    "esi\t0x%7$x\t%7$d\n"
                    "edi\t0x%8$x\t%8$d\n"
                    "eip\t0x%9$x\t%9$d\n",
                    cpu.eax, cpu.ecx, cpu.edx, cpu.ebx,
                    cpu.esp, cpu.ebp, cpu.esi, cpu.edi,
                    cpu.eip);
        }
        else if (strcmp(arg, "w") == 0) {
            WP* head = wp_get_head();
            for (int i = 0; head != NULL; i++, head = head->next) {
                printf("Watchpoint %d %s=%u\n", i, head->exp, head->old_value);
            }
        }
        else {
            printf("Unsupported command '%s'\n", arg);
        }
    }
    return 0;
}

static int cmd_x(char *args) {
    char *arg1 = strtok(NULL, " ");
    char *arg2 = strtok(NULL, " ");
    int n, esp;
    
    if (arg1 == NULL) {
        printf("Missing subcommand 1\n");
        return 0;
    }
    else {
        n = atoi(arg1);
        if (n <= 0) {
            printf("The 1st argument should be a postive integer rather than '%s'\n", arg1);
            return 0;
        }
    }
    if (arg2 == NULL) {
        printf("Missing subcommand 2\n");
        return 0;
    }
    else {
        // TODO: modify it to expr eval
        esp = strtol(arg2, NULL, 16);
        if (esp < 0 || esp >= (128 * 1024 * 1024)) {
            printf("Wrong memory address!\n");
            return 0;
        }
        for (int i = 0; i < n; i++) {
            printf("0x%x:\t0x%08x\n", esp, 
                    vaddr_read(esp, 4));
            esp += 4;
        }
    }
    return 0;
}

static int cmd_p(char *args) {
    bool success = true;
    uint32_t res = expr(args, &success);
    if (!success) {
        printf("Error in '%s'\n", args);
    }
    else {
        printf("%u\n", res);
    }
    return 0;
}

static int cmd_w(char *args) {
    bool success = true;
    if (!args) {
        printf("No expression inputed.\n");
        return 0;
    }
    WP* wp = new_wp();
    wp->exp = strdup(args);
    wp->old_value = expr(args, &success);
    if (!success) {
        printf("Error in calculating the expression.\n");
        free_wp(wp);
    }
    return 0;
}

static int cmd_d(char *args) {
    WP* head = wp_get_head();
    int num = atoi(args);
    if (num >= 0 && num < 32) {
        for (int i = 0; head != NULL; i++, head = head->next) {
            if (i == num) {
                free_wp(head);
                return 0;
            }
        }
    }
    printf("Watchpoint id out of bound!\n");
    return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
