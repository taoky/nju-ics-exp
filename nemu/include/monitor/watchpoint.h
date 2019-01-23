#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"
#include <stdlib.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char *exp;
  uint32_t old_value;

} WP;

WP* new_wp();
void free_wp(WP *wp);
WP* wp_get_head();

#endif
