#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#define FAULT 0X80000000
enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_DEC,//Decimal Number
  TK_PLUS,//"+"
  TK_SUB,//"-"
  TK_MULTI,//"*"
  TK_DIVI,//"/"
  TK_LPA,//"("
  TK_RPA,//")"
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal
  //my rules
  {"\\-",TK_SUB},       // sub
  {"\\*",TK_MULTI},     // multiply
  {"/",TK_DIVI},        // Devision
  {"\\(",TK_LPA},       // left parenthesis
  {"\\)",TK_RPA},       // right parenthesis
  {"[0-9]+",TK_DEC},    // decimal number
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0; int i; regmatch_t pmatch; nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
	*/
        switch (rules[i].token_type) {
          case TK_NOTYPE: {
	    printf("space\n");
	    break;
	  }
	  case TK_PLUS: case TK_EQ: case TK_MULTI: case TK_SUB: 
	  case TK_DIVI: case TK_LPA: case TK_RPA:
	  case TK_DEC: {
	    tokens[nr_token].type = rules[i].token_type;
	    strncpy(tokens[nr_token].str,substr_start,substr_len);
	    tokens[nr_token].str[substr_len] = '\0';
	    nr_token++;
	    break;
	  }
          default: TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

#define STACKSIZE 31 //the max size of stack
typedef struct Stack{
  char stack[STACKSIZE];
  int top;
}my_stack;
static int push(my_stack S, char ch);
static int pop(my_stack S);
void my_printf(int p,int q);


//Is the stack empty?
static bool is_empty(my_stack S){
  if(S.top == 0){
    return true;
  }
  else{
    return false;
  }
}

//push
static int push(my_stack S, char ch){
  if(S.top == STACKSIZE){
    panic("Overflow\n");
    return -1;
  }
  S.stack[S.top] = ch;
  S.top++;
  return 0;
}

//pop
static int pop(my_stack S){
  if(is_empty(S)){
    panic("Error,Stack is empty\n");
  }
  S.top--;
  return 0;
}

//printf
void my_printf(int p,int q){
  int count = p;
  for(;count <= q;count++){
    printf("%s",tokens[count].str);
  }
  printf("\n");
}

//check parentheses
bool check_parenthese(uint32_t p,uint32_t q){
  int value = 0;
  if(tokens[p].type != TK_LPA){
    return false;
  }
  int count = p;
  for(;count <= q;count++){
    if(tokens[count].type == TK_LPA){
      value++;
    }
    else if(tokens[count].type == TK_RPA){
      if(value == 0){
        panic("the expression is wrong\n");
	return false;
      }
      value--;
      if(value == 0){
        if(count == q){
	  return true;
	}
	return false;
      }
    }
  }
  return false;
}

#include<stdlib.h>

uint32_t eval(uint32_t p,uint32_t q){
  my_stack S;
  S.top = 0;
  int op_type = 0x7fffffff;
  int op = 0;
  int count = p;
  if(p > q){
    panic("expression is wrong\n");
  }
  else if(p == q){
    if(tokens[p].type == TK_DEC){
      return atoi(tokens[p].str);
    }
    else{
      printf("expression is wrong\n");
      assert(0);
    }
  }
  else if(check_parenthese(p, q) == true){
    return eval(p + 1,q - 1);
  }
  else{
    for(;count <= q;count++){
      switch(tokens[count].type){
        case TK_PLUS:{
	  if(is_empty(S) == false){
	    break;
	  }
	  op = count;
	  op_type = TK_PLUS;
	  break;
	}
	case TK_SUB:{
	  if(is_empty(S) == false){
	     break;
	  }
	  op = count;
	  op_type = TK_SUB;
          break;		      
        }
	case TK_MULTI:{
	  if(is_empty(S) == false){
	    break;
	  }
	  if(op_type < TK_MULTI){
	    break;
	  }
	  op = count;
	  op_type = TK_MULTI;
	  break;
	}
	case TK_DIVI:{
	  if(is_empty(S) == false){
	    break;
	  }
	  if(op_type < TK_MULTI){
	    break;
	  }
	  op = count;
	  op_type = TK_DIVI;
	  break;
	}
	case TK_LPA:{
	  push(S, '1');
	  break;
	}
	case TK_RPA:{
	  if(is_empty(S) == true){
	    panic("exp is not the BNF\n");
	  }
	  pop(S);
	  break;
	}
      }
    }
  }
  uint32_t val1 = eval(p, op - 1);
  uint32_t val2 = eval(op + 1, q);
  switch(op_type){
    case TK_PLUS: return val1 + val2;
    case TK_SUB:  return val1 - val2;
    case TK_MULTI: return val1 * val2;
    case TK_DIVI: return val1 / val2;
    default: {
	printf("exp is wrong\n");
	assert(0);
    }
  }
  return -1;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  int re = eval(0, nr_token - 1);
  if(re == 0x80000000){
    *success = false;
  }
  return re;
}
