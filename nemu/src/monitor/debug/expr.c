#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM

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
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},           // division
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {"[0-9]+", TK_NUM},     // number (digits)
  {"==", TK_EQ}         // equal
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
        if (nr_token >= 32) {
            printf("tokens[32] array overflow: too many tokens.\n");
            return false;
        }
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case '+': case '-': case '*': case '/': case '(': case ')':
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token++].str[0] = '\0';
            break;
          case TK_NUM:
            tokens[nr_token].type = rules[i].token_type;
            if (substr_len >= 32) {
                printf("The length of the number is too long.\n");
                return false;
            }
            strncpy(tokens[nr_token++].str, substr_start, substr_len);
            break;
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

static uint32_t eval(int p, int q, bool *success);

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  *success = true;
  uint32_t res = eval(0, nr_token, success);

  return res;
}

static bool check_parentheses(int p, int q) {
    if (tokens[p].type != '(' || tokens[q].type != ')') return false;
    int lp_cnt = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == '(') lp_cnt++;
        else if (tokens[i].type == ')') {
            if (lp_cnt == 0) return false;
            lp_cnt--;
            if (i != q && lp_cnt == 0) return false;
        }
    }
    if (lp_cnt != 0) return false;
    return true;
}

static int priority_cmp(int x, int y) {
    /*
     * Compare two chars by priority.
     * return 1: x > y
     * return 0: x = y
     * return -1: x < y
     */
    Log("priority_cmp(), x=%d, y=%d", x, y);
    Assert(x == y && x == -1, "priority_cmp()'s x == y == -1");
    if (x == -1) return -1;
    else if (y == -1) return 1;
    else {
        int px = -1, py = -1;
        if (tokens[x].type == '+' || tokens[x].type == '-')
            px = 0;
        else if (tokens[x].type == '*' || tokens[x].type == '/')
            px = 1;
        if (tokens[y].type == '+' || tokens[y].type == '-')
            py = 0;
        else if (tokens[y].type == '*' || tokens[y].type == '/')
            py = 1;
        Assert(x == -1, "priority_cmp() x wrong arg");
        Assert(y == -1, "priority_cmp() y wrong arg");
        return px > py;
    }
}

static int find_main_op(int p, int q) {
    int lp_cnt = 0;
    int ret = -1;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == '(')
            lp_cnt++;
        else if (tokens[i].type == ')') {
            if (lp_cnt == 0)
                return -1;
            lp_cnt--;
        }
        else if (tokens[i].type == '+' || tokens[i].type == '-' || tokens[i].type == '*' || tokens[i].type == '/') {
            if (lp_cnt != 0)
                continue;
            if (priority_cmp(ret, i) != -1) {
                ret = i;
            }
        }
    }
    Assert(ret == -1, "find_main_op() does not find the result.");
    return ret;
}

static uint32_t eval(int p, int q, bool *success) {
    if (p > q) {
        *success = false;
        return 0;
    }
    else if (p == q) {
        if (tokens[p].type == TK_NUM)
        {
            *success = true;
            return (uint32_t)strtoul(tokens[p].str, NULL, 16);
        }
        else {
            *success = false;
            return 0;
        }
    }
    else if (check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1, success);
    }
    else {
        Log("p: %d, q: %d", p, q);
        int op = find_main_op(p, q);
        if (op == -1) {
            *success = false;
            return 0;
        }
        uint32_t val1 = eval(p, op - 1, success);
        if (!(*success))
            return 0;
        uint32_t val2 = eval(op + 1, q, success);
        if (!(*success))
            return 0;

        switch (tokens[op].type) {
            case '+': 
                return val1 + val2;
            case '-':
                return val1 - val2;
            case '*':
                return val1 * val2;
            case '/':
                if (val2 == 0) {
                    printf("Divided by 0!\n");
                    *success = false;
                    return 0;
                }
                return val1 / val2;
            default: assert(0);
        }
    }
}
