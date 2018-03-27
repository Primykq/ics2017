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
static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Stop after executing N instructions", cmd_si},
  { "info", "Print the value of value", cmd_info},
  { "x","Scan the memory", cmd_x},
  {"p","Expression evaluation",cmd_p}
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

static int cmd_si(char *args){
  char *arg = strtok(NULL," ");
  if(arg == NULL){//no argument
    cpu_exec(1);
  }
  else{
    int i = atoi(arg);
    if(i < 0){
      printf("illegal command '%s'\n", arg);
    }
    if(i == 0){//si 0 = si
      cpu_exec(1);
    }
    cpu_exec(i);
  }

  return 0;
}

static int cmd_info(char *args){
  char *arg = strtok(NULL, " ");
  int i;
  if(arg == NULL){
    printf("Error!\n");
  }
  else{
    if(strcmp(arg, "r") == 0){
      printf("    register        Hex     Decimal\n");
      for(i = 0;i < 8;i++){
	printf("%10s %16x %16d\n",regsl[i],reg_l(i),reg_l(i));
      }
      printf("       eip%16x %16d\n",cpu.eip,cpu.eip);
    }
    else{
      printf("defaultn");
    }
  }

  return 0;
}

static int cmd_x(char *args){
  char *arg_1 = strtok(NULL, " ");
  char *arg_2 = strtok(NULL, " ");
  if(arg_1 == NULL || arg_2 == NULL){
    printf("Illegal Command\n");
    return 0;
  }
  else{
    int n = atoi(arg_1);
    long addr = strtol(arg_2, NULL, 16);
    int i = 0;
    for(;i < n;i++){
      int j = 0;
      char s[20];
      sprintf(s,"%lx: ",addr);
      printf("0x%s",s);
      printf("0x");
      for(;j < 8;j++){
	printf("%02x",pmem[addr+j]);
      }
      printf(" ");
      j = 0;
      if(++i >= n){
	break;
      }
      addr = addr + 8;
      for(;j < 8;j++){
	printf("%02x",pmem[addr+j]);
      }
      printf("\n");
      addr = addr + 8;
    }
  }
  return 0;
}

static int cmd_p(char *args){
  char cmd[81] = "\0";
  while(true){
    char *arg = strtok(NULL, " ");
    if(arg == NULL){
      break;
    } 
    if(strlen(arg) + strlen(cmd) > 80){
      printf("Overflow\n");
      return 0;
    }
    strcat(cmd,arg);
  }
  bool flag;
  uint32_t re = expr(cmd,&flag);
  if(flag == true){
    printf("%s =0x%x(%d)\n",cmd,re,re);
  }
  else{
    printf("Illegal Command\n");
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
    extern void dl_clear_event_queue(void);
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
