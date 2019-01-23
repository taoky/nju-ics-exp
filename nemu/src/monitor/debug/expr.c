#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_DNUM, TK_HNUM, TK_REG, TK_DREF, TK_NEQ, TK_LAND,
  TK_GE, TK_LE,

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
  {"0x[0-9a-f]+", TK_HNUM}, // hex number
  {"[0-9]+", TK_DNUM},     // decimal number (digits)
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ah|dh|ch|bh|ax|dx|cx|bx|bp|si|di|sp|al|dl|cl|bl)", TK_REG},        // registers
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply or dereference
  {"/", '/'},           // division
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {">=", TK_GE},
  {"<=", TK_LE},
  {">", '>'},
  {"<", '<'},
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_LAND},      // logical 'and'
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

#define TK_LENS 65536

Token tokens[TK_LENS];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position]  != '\0') {
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
        if (nr_token >= TK_LENS) {
            printf("tokens[%d] array overflow: too many tokens.\n", TK_LENS);
            return false;
        }
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case '+': case '-': case '*': case '/': case '(': case ')': case TK_EQ: case TK_NEQ: case TK_LAND: case TK_GE: case TK_LE: case '<': case '>':
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token++].str[0] = '\0';
            break;
          case TK_DNUM: case TK_HNUM:
            tokens[nr_token].type = rules[i].token_type;
            if (substr_len >= 32) {
                printf("The length of the number is too long.\n");
                return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token++].str[substr_len] = '\0';
            Log("Number token %d: %s", nr_token - 1, tokens[nr_token - 1].str);
            break;
          case TK_REG:
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1);
            tokens[nr_token++].str[substr_len] = '\0';
            Log("Reg token %d: %s", nr_token - 1, tokens[nr_token - 1].str);
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

  for (int i = 0; i < nr_token; i++) {
      if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_DNUM && tokens[i - 1].type != TK_HNUM && tokens[i - 1].type != TK_REG && tokens[i - 1].type != ')')))
          tokens[i].type = TK_DREF;
  }

  *success = true;
  uint32_t res = eval(0, nr_token - 1, success);

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
    // init priority array
    static int priority[512] = {};
    priority[TK_DREF] = 2;
    priority['*'] = priority['/'] = 3;
    priority['+'] = priority['-'] = 4;
    priority['<'] = priority['>'] = priority[TK_LE] = priority[TK_GE] = 6;
    priority[TK_EQ] = priority[TK_NEQ] = 7;
    priority[TK_LAND] = 11;
    // larger, less priority

    Assert(!(x == y && x == -1), "priority_cmp()'s x == y == -1");
    if (x == -1) return -1;
    else if (y == -1) return 1;
    else {
        int px, py;
        px = priority[tokens[x].type];
        py = priority[tokens[y].type];
        Assert(px != 0, "priority_cmp() x wrong arg");
        Assert(py != 0, "priority_cmp() y wrong arg");
        if (px > py) return 1;
        else if (px == py) return 0;
        else return -1;
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
                ret = -1;
            lp_cnt--;
        }
        else if (tokens[i].type != TK_DNUM && tokens[i].type != TK_HNUM && tokens[i].type != TK_REG) {
            if (lp_cnt != 0)
                continue;
            if (priority_cmp(ret, i) != 1) { // priority ret <= i
                ret = i;
            }
        }
    }
    if (ret == -1)
        Log("find_main_op() failed");
    return ret;
}

static uint32_t eval(int p, int q, bool *success) {
    if (p > q) { 
        *success = false;
        return 0;
    }
    else if (p == q) {
        if (tokens[p].type == TK_DNUM || tokens[p].type == TK_HNUM)
        {
            *success = true;
            return (uint32_t)strtoul(tokens[p].str, NULL, tokens[p].type == TK_DNUM ? 10 : 16);
        } 
        else if (tokens[p].type == TK_REG) {
            *success = true;
            if (strcmp(tokens[p].str, "eax") == 0)
                return cpu.eax;
            else if (strcmp(tokens[p].str, "edx") == 0)
                return cpu.edx;
            else if (strcmp(tokens[p].str, "ecx") == 0)
                return cpu.ecx;
            else if (strcmp(tokens[p].str, "ebx") == 0)
                return cpu.ebx;
            else if (strcmp(tokens[p].str, "ebp") == 0)
                return cpu.ebp;
            else if (strcmp(tokens[p].str, "esi") == 0)
                return cpu.esi;
            else if (strcmp(tokens[p].str, "edi") == 0)
                return cpu.edi;
            else if (strcmp(tokens[p].str, "esp") == 0)
                return cpu.esp;
            else if (strcmp(tokens[p].str, "ax") == 0)
                return reg_w(R_AX);
            else if (strcmp(tokens[p].str, "dx") == 0)
                return reg_w(R_DX);
            else if (strcmp(tokens[p].str, "cx") == 0)
                return reg_w(R_CX);
            else if (strcmp(tokens[p].str, "bx") == 0)
                return reg_w(R_BX);
            else if (strcmp(tokens[p].str, "bp") == 0)
                return reg_w(R_BP);
            else if (strcmp(tokens[p].str, "si") == 0)
                return reg_w(R_SI);
            else if (strcmp(tokens[p].str, "di") == 0)
                return reg_w(R_DI);
            else if (strcmp(tokens[p].str, "sp") == 0)
                return reg_w(R_SP);
            else if (strcmp(tokens[p].str, "al") == 0)
                return reg_b(R_AL);
            else if (strcmp(tokens[p].str, "dl") == 0)
                return reg_b(R_DL);
            else if (strcmp(tokens[p].str, "cl") == 0)
                return reg_b(R_CL);
            else if (strcmp(tokens[p].str, "bl") == 0)
                return reg_b(R_BL);
            else if (strcmp(tokens[p].str, "ah") == 0)
                return reg_b(R_AH);
            else if (strcmp(tokens[p].str, "dh") == 0)
                return reg_b(R_DH);
            else if (strcmp(tokens[p].str, "ch") == 0)
                return reg_b(R_CH);
            else if (strcmp(tokens[p].str, "bh") == 0)
                return reg_b(R_BH);
            else {
                Log("Unrecognized reg name");
                *success = false;
                return 0;
            }
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
        Log("op: %d", op);
        if (tokens[op].type != TK_DREF) {
            uint32_t val1 = eval(p, op - 1, success);
            if (!(*success))
                return 0;
            Log("val1: %u", val1);
            uint32_t val2 = eval(op + 1, q, success);
            if (!(*success))
                return 0;
            Log("val2: %u", val2);
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
                case '<':
                    return val1 < val2;
                case '>':
                    return val1 > val2;
                case TK_LE:
                    return val1 <= val2;
                case TK_GE:
                    return val1 >= val2;
                case TK_LAND:
                    return val1 && val2;
                case TK_EQ:
                    return val1 == val2;
                case TK_NEQ:
                    return val1 != val2;
                default: Assert(0, "Unrecognized type %d", tokens[op].type);
            }
        }
        else {
            // TK_DREF
            // Log("DREF");
            uint32_t val = eval(op + 1, q, success);
            if (!(*success)) return 0;
            Log("val: %u", val);
            if (val < 0 || val >= (128 * 1024 * 1024)) {
                Log("mem[%u] out of bound", val);
                *success = false;
                return 0;
            }
            return vaddr_read(val, sizeof(uint32_t));
        }
    }
}
