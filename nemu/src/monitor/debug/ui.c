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


static int cmd_si(char *args){
  int steps;
  if (args ==NULL){
    steps=1;
  }
  else { 
    sscanf(args,"%d",&steps);
  }
  cpu_exec(steps);
  return 0;
}
	
static int cmd_info(char *args) {
  if (args[0] == 'r') {
     printf("General reg: ----------------------------------------------------------- \n");
     int i;
     for (i = 0; i < 8 ; i++) {
	printf("$%s\t0x%08x\t%d\n", regsl[i], reg_l(i),reg_l(i) );
     }
     printf("Special reg: ----------------------------------------------------------- \n");	
     printf("$eip\t0x%08x\t%d\n", cpu.eip, cpu.eip);
  }
  if (args[0] == 'w'){
		printf_wp();
		return 0;
  }
  return 0;
}

static int cmd_x(char* args){
  if (args == NULL) {
    printf("ERORR!\n");
    return 0;
  }
  int num;
  char exp[100];
  bool success = true;
  sscanf(args, "%d %s", &num, exp);
  uint32_t addr = expr(exp, &success);
  int j=0;
  for (int i = 0; i < num; i++) {
    if (j % 4 == 0){
      printf("0x%x:\t", addr);
    }
    printf("0x%08x ", vaddr_read(addr, 4));
    addr +=4;
    j++;
    if ( j % 4 == 0){
      printf ("\n");
    }
  }
  printf("\n");
  return 0;
}
static int cmd_p(char *args){
  bool success = true;
  uint32_t result = expr (args, &success);
  if (!success) {
    printf("wrong!\n");
  }
  else {
    printf("%u\n",result);
  }
  return 0;
}
static int cmd_w(char* args) {
  bool success = true;
  uint32_t result = expr(args, &success);
  if(!success) {
    printf("wrong expression!\n");
    return -1;
  }
  WP* wp = new_wp();
  wp->result = result;
  strcpy(wp->expr, args);
  printf("watchpoint %d: %s\n", wp->NO, wp->expr);
  return 0;
}
static int cmd_d(char *args){
	int p;
	bool key = true;
	sscanf(args, "%d", &p);
	WP* q = delete_wp(p, &key);
	if (key){
		printf("Delete watchpoint %d: %s\n", q->NO, q->expr);
		free_wp(q);
		return 0;
	} else {
		printf("No found watchpoint %d\n", p);
		return 0;
	}
	return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  /* TODO: Add more commands */
  { "si", "Pause the execution after stepping N commands", cmd_si},
  { "info", "Print registers status", cmd_info},
  { "x", "Scan memory", cmd_x},
  { "p", "Expression evaluation", cmd_p},
  { "w", "new a watchpoint", cmd_w},
  { "d", "Delete watchpoint", cmd_d},
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

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
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
