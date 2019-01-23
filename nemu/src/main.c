int init_monitor(int, char *[]);
void ui_mainloop(int);

#include "nemu.h"
#include "../include/monitor/expr.h"

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  int is_batch_mode = init_monitor(argc, argv);

  /* PA1 Phase 2 Test
  char x;
  printf("Run PA1 Phase2 test? (y/...)");
  x = getchar();
  if (x == 'y') {
      FILE* test_file = fopen("tools/gen-expr/input", "r");
      if (!test_file) {
          Log("Cannot find PA1 Phase2 test file!");
      }
      else {
          uint32_t res;
          char buf[65536];
          while (fscanf(test_file, "%u %[^\n]", &res, buf) == 2) {
              bool success;
              uint32_t expr_res = expr(buf, &success);
              Assert(success && res == expr_res, "failed at %s, expected %u, got %u, success %d", buf, res, expr_res, success);
              Log("%s=%d success", buf, res);
          }
      }
  } */

  /* Receive commands from user. */
  ui_mainloop(is_batch_mode);

  return 0;
}
