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
  TK_NEGA,TK_DEREF,//symbol
  TK_REG,//register
  TK_NEQ,// "!="
  TK_OR,TK_AND,TK_NOT,// || && !
  TK_HEX,
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
  {"\\$[A-Za-z]+",TK_REG}, // register
  {"!=",TK_NEQ},         //!=
  {"!",TK_NOT},          // !
  {"\\|\\|",TK_OR},      // ||
  {"&&",TK_AND},
  {"Ox[A-Za-z0-9]+",TK_HEX},
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
	  case TK_SUB: {
	    if(nr_token == 0 || tokens[nr_token - 1].type != TK_DEC){
	      tokens[nr_token].type = TK_NEGA;
	    }
	    else{
	      tokens[nr_token].type = TK_SUB;
	    }
	    strncpy(tokens[nr_token].str, substr_start, substr_len);
	    tokens[nr_token].str[substr_len] = '\0';
	    nr_token++;
	    break;
	  }
	  case TK_MULTI: {
	    if(nr_token == 0 || (tokens[nr_token-1].type != TK_DEC && tokens[nr_token-1].type != TK_HEX && tokens[nr_token-1].type!= TK_REG && tokens[nr_token-1].type != TK_RPA)){
	      tokens[nr_token].type = TK_DEREF;
	    }
	    else{
	      tokens[nr_token].type = TK_MULTI;
	    }
	    strncpy(tokens[nr_token].str, substr_start, substr_len);
	    tokens[nr_token].str[substr_len] = '\0';
	    nr_token++;
	    break;
	  }
	  case TK_OR: case TK_AND:case TK_NOT:
	  case TK_NEQ:
	  case TK_PLUS: case TK_EQ:  
	  case TK_DIVI: case TK_LPA: case TK_RPA:
	  case TK_DEC: case TK_HEX: case TK_REG: {
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
void my_printf(int p,int q);


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
  int value = 0;
  if(p > q){
    panic("expression is wrong\n");
  }
  else if(p == q){
    if(tokens[p].type == TK_DEC){
      return atoi(tokens[p].str);
    }
    else if(tokens[p].type == TK_HEX){
      return strtol(tokens[p].str, NULL, 16);
    }
    else if(tokens[p].type == TK_REG){
      int count = 0;
      char str[10];
      for(count = 1;count < strlen(tokens[p].str);count++){
        str[count-1] = tokens[p].str[count];
      }
      str[count-1] = '\0';
      for(count = 0;count < 8;count++){
        if(strcmp(str, regsl[count]) == 0){
	  return reg_l(count);
	}
	else if(strcmp(str, regsw[count]) == 0){
	  return reg_w(count);
	}
	else if(strcmp(str, regsb[count]) == 0){
	  return reg_b(count);
	}
      }
      if(strcmp(str,"eip") == 0){
        return cpu.eip;
      }
      printf("wrong register\n");
      return FAULT;
    }
    else{
      printf("expression is wrong\n");
      return FAULT;
    }
  }
  else if(check_parenthese(p, q) == true){
    return eval(p + 1,q - 1);
  }
  else{
    int op_type = 99999;
    int op = 0;
    int count = p;
    for(;count <= q;count++){
      bool flag = false;
      if(tokens[count].type == TK_OR){
        if(value != 0){
	  continue;
	}
	flag = true;
      }
      if(tokens[count].type == TK_AND){
        if(value != 0 || op_type < TK_AND){
	  continue;
	}
	flag = true;
      }
      if(tokens[count].type == TK_EQ || tokens[count].type == TK_NEQ){
        if(value != 0 || op_type < TK_EQ){
	  continue;
	}
	flag = true;
      }
      if(tokens[count].type == TK_PLUS || tokens[count].type == TK_SUB){
        if(value != 0 || op_type < TK_PLUS){
	  continue;
	}
	flag = true;
      }  
      if(tokens[count].type == TK_MULTI || tokens[count].type == TK_DIVI){
        if(value != 0  || op_type < TK_MULTI){
	  continue;
	}
	flag = true;
      }
      if(tokens[count].type == TK_LPA){
        value++;
	continue;
      }
      if(tokens[count].type == TK_RPA){
        if(value == 0){
	  printf("exp is wrong\n");
	  return FAULT;
	}
	value--;
	continue;
      }
      if(flag == true){
        op = count;
	op_type = tokens[count].type;
      }
    }
    if(op == 0){
      if(tokens[p].type == TK_NEGA){
        return 0 - eval(p + 1, q);
      }
      else if(tokens[p].type == TK_NOT){
        return !(eval(p+1, q));
      }
      else if(tokens[p].type == TK_DEREF){
        return vaddr_read(eval(p+1, q),4);
      }
      else{
        panic("Something seems wrong\n");
      }
    }
    uint32_t val1 = eval(p, op - 1);
    uint32_t val2 = eval(op + 1, q);
    if(val1 == FAULT || val2 == FAULT){
      return FAULT;
    }
    switch(op_type){
      case TK_PLUS: return val1 + val2;
      case TK_SUB:  return val1 - val2;
      case TK_MULTI: return val1 * val2;
      case TK_DIVI: return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR:  return val1 || val2;
      default: {
	printf("exp is wrong\n");
	assert(0);
      }
    }
  }
  return FAULT;
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
