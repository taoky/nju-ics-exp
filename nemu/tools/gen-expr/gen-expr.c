#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

static inline uint32_t choose(uint32_t n) {
    return rand() % n;
}

#define INC 65536
static char *buf;
static int bufpos = 0;
static int bufsize;

static inline void buf_enlarge();

static inline int num_len(int num) {
    return snprintf(NULL, 0, "%u", num);
}

static inline void gen_num() {
    uint32_t num = rand();
    int nlen = num_len(num);
    if (bufpos + nlen >= bufsize - 1)
        buf_enlarge();
    sprintf(buf + bufpos, "%u", num);
    bufpos += nlen;
}

static inline void gen(char x) {
    if (bufpos >= bufsize - 1) buf_enlarge();
    buf[bufpos++] = x;
}

static inline void gen_rand_op() {
    if (bufpos >= bufsize - 1) buf_enlarge();
    char x;
    switch (choose(4)) {
        case 0: x = '+'; break;
        case 1: x = '-'; break;
        case 2: x = '*'; break;
        default: x = '/'; break;
    }
    buf[bufpos++] = x;
}

static inline void gen_rand_space() {
    if (bufpos >= bufsize - 1) buf_enlarge();
    switch (choose(4)) {
        case 0: buf[bufpos++] = ' ';
        default: break;
    }
}

static inline void gen_rand_expr() {
  switch (choose(3)) {
      case 0: gen_rand_space(); gen_num(); gen_rand_space(); break;
      case 1: gen('('); gen_rand_space(); gen_rand_expr(); gen_rand_space(); gen(')'); break;
      default: gen_rand_expr(); gen_rand_space(); gen_rand_op(); gen_rand_space(); gen_rand_expr(); break;
  }
  buf[bufpos] = '\0';
}

static inline void gen_init() {
    buf = malloc(INC);
    bufsize = INC;
    bufpos = 0;
    assert(buf);
}

static inline void buf_enlarge() {
    char *newbase = realloc(buf, bufsize + INC);
    assert(newbase);
    buf = newbase;
    bufsize += INC;
}

static inline void gen_cleanup() {
    free(buf);
    buf = NULL;
}

static char code_buf[65536];
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_init();
    gen_rand_expr();
    if (bufsize != INC) continue; // lazy
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen(".code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Wno-overflow -Wno-div-by-zero .code.c -o .expr");
    if (ret != 0) continue;

    fp = popen("./.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    int code = pclose(fp);
    if (code == 0)
        printf("%u %s\n", result, buf);
    gen_cleanup();
  }
  return 0;
}
