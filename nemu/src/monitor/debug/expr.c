#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ

  /* TODO: Add more token types */
  TK_NUM,
  TK_REGISTER,
  TK_HEX,
  TK_NOTEQ,
  TK_OR,
  TK_AND,
  TK_NEG
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},
  {"\\*", '*'},
  {"\\/", '/'},

  {"\\$[a-z]+", TK_REGISTER},
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_NUM},

  {"==", TK_EQ}         // equal
  {"!=", TK_NOTEQ},

  {"\\(", '('},
  {"\\)", ')'},
  {"\\|\\|", TK_OR},
  {"&&", TK_AND},
  {"!", '!'},
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
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

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
	case TK_NUM:
		tokens[nr_token].type = TK_NUM;
		strncpy(tokens[nr_token].str, substr_start,substr_len);
		nr_token++;
		break;
	case TK_REGISTER:
		tokens[nr_token].type = TK_REGISTER;
		strncpy(tokens[nr_token].str, substr_start, substr_len);
		nr_token++;
		break;
	case TK_HEX:
		tokens[nr_token].type = TK_HEX;
		strncpy(tokens[nr_token].str, substr_start, substr_len);
		nr_token++;
		break;
	case TK_EQ:
		tokens[nr_token].type = TK_EQ;
		strcpy(tokens[nr_token].str, "==");
		nr_token++;
		break;
	case TK_NOTEQ:
		tokens[nr_token].type = TK_NOTEQ;
		strcpy(tokens[nr_token].str, "!=");
		nr_token++;
		break;
	case TK_OR:
		tokens[nr_token].type = TK_OR;
		strcpy(tokens[nr_token].str, "||");
		nr_token++;
		break;
	case TK_AND:
		tokens[nr_token].type = TK_AND;
		strcpy(tokens[nr_token].str, "&&");
		nr_token++;
		break;
	case '+':
		tokens[nr_token].type = '+';
		nr_token++;
		break;
	case '-':
		tokens[nr_token].type = '-';
		nr_token++;
		break;
	case '*':
		tokens[nr_token].type = '*';
		nr_token++;
		break;
	case '/':
		tokens[nr_token].type = '/';
		nr_token++;
		break;
	case '!':
		tokens[nr_token].type = '!';
		nr_token++;
		break;
	case '(':
		tokens[nr_token].type = '(';
		nr_token++;
		break;
	case ')':
		tokens[nr_token].type = ')';
		nr_token++;
		break;
	default: 
		assert(0);
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

bool check_parentheses(int p, int q){
  int unmatched = 0;
  if (tokens[p].type == '(' || tokens[q].type == ')'){
    for (int i = p; i <= q; i++){
      if (tokens[i].type == '('){
        unmatched++;
      }
      if (tokens[i].type == ')'){
        unmatched--;
      }
      if (a != q && unmatched == 0){
        return false;
      }
    }
    if (unmatched == 0){
      return true;
    } else {
       return false;
    }
  }
  return false;
}


int dominant_operator(int p, int q){
  int unmatched = 0;
  int op_index = -1;
  int priority = 0;
  for (int i = p; i <= q; i++){
    if (tokens[i].type == '('){
      unmatched++;
    }
    if (tokens[i].type == ')'){
      unmatched--;
    }	
    if (unmatched == 0){
    if (tokens[i].type == TK_OR){
      if (priority < 5){
        op_index = i;
        priority = 5;
      }
    } else if (tokens[i].type == TK_AND){
      if (priority < 4){    
        op_index = i;
        priority = 4;
      }
    } else if (tokens[i].type == TK_EQ || tokens[i].type == TK_NOTEQ){
      if (priority < 3){
        op_index = i;
        priority = 3;
      }
    } else if (tokens[i].type == '+' || tokens[i].type == '-'){
      if (priority < 2){
        op_index = i;
        priority = 2;
    }
    } else if (tokens[i].type == '*' || tokens[i].type == '/'){
      if (priority < 1){
        op_index = i;
        priority = 1;
      }
    }
    else if (unmatched < 0){
      return -2;
    }
  }
  }
  return op_index;
}
uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
