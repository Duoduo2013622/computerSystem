#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[100];
  uint32_t result;
} WP;

  WP* new_wp();
  void free_wp(WP * wp);
  WP* delete_wp(int p , bool *key);
  void printf_wp();
  bool checkWP();
#endif
