#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  if (!free_) assert(0);
  WP* next_free = free_->next;
  if (!head) {
      head = free_;
      head->next = NULL;
  }
  else {
    free_->next = head;
    head = free_;
  }
  free_ = next_free;
  return head;
}

void free_wp(WP *wp) {
  Assert(wp, "Function free_wp() receives NULL.");
  WP *p = head;
  if (wp->exp) {
    free(wp->exp);
    wp->exp = NULL;
  }
  if (p != wp)
    while (p->next != wp) 
        p = p->next;
  else // only one element inside
    head = NULL;
  p->next = wp->next;
  if (!free_) {
    free_ = wp;
    free_->next = NULL;
  }
  else {
    wp->next = free_;
    free_ = wp;
  }
}

WP* wp_get_head() {
    return head;
}
