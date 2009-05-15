#include <string.h>

#define __OSCAR2_TEST__
#include "oscar2.h"

int cmp_frames(struct frame_t *f, struct frame_t *g) {
  if(!f && !g) return 0;

  if(!f) return -1;

  if(!g) return 1;

  if(f->l < g->l) return -1;
  if(f->l > g->l) return 1;

  return memcmp(f->buf, g->buf, f->l);
}

int cmp_frame_queues(struct frame_queue_t *q, struct frame_queue_t *p, int include_next) {
  int i;

  i = cmp_frames(&q->f, &p->f);
  if(0 != i)
    return i;

  if(include_next == 1 && q->next != p->next) return -1;
  if(include_next == 2) {
    int cmp_next = cmp_frame_queues(q->next, p->next, include_next);
    if(cmp_next != 0)
      return cmp_next;
  }

  return 0;
}

void fail(char *file, int line, char *msg) {
  printf("FAIL: %s:%d: %s\n", file, line, msg);
}

#define FAIL(msg) fail(__FILE__, __LINE__, (msg))

int test_frame_enqueue() {
  struct frame_queue_t *root = NULL;
  struct frame_queue_t *cur;

  struct frame_queue_t expected;

  int errs = 0;

  expected.f.buf = "hi mom";
  expected.f.l = strlen("hi mom");
  expected.fixup_seqno = NO_SEQNO_FIXUP;
  expected.next = NULL;

  cur = frame_enqueue(root, "hi mom", strlen("hi mom"), NO_SEQNO_FIXUP);

  if(0 != cmp_frame_queues(cur, &expected, 0)) {
    FAIL("failed cmp:case=0");
    errs++;
  }

  expected.f.buf = "wrong";
  if(0 == cmp_frame_queues(cur, &expected, 0)) {
    FAIL("failed cmp:case=wrong");
    errs++;
  }
  

  return errs;
}

typedef int(*test_function)();

int main(int argc, char* argv[]) {
  int errors = 0;

  test_function tests[] = {
    test_frame_enqueue,
  };

  int i;

  int n_tests;

  test_function curtest;


  n_tests = sizeof(tests)/sizeof(curtest);

  for(i = 0; i < n_tests; i++) {
    curtest = tests[i];
    errors += curtest();
  }

  
  printf("Got %d errors in %d tests\n", errors, n_tests);
  
  return 0;
}
