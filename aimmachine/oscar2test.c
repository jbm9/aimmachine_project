#include <string.h>

#define __OSCAR2_TEST__
#include "oscar2.h"



/**********************************************************************
 *************** TEST CASES *******************************************
 *********************************************************************/

/* This is a kludge for this file only, since we use static strings as
   queue bodies and don't necessarily consume the output frames */
struct frame_queue_t * free_frame_queue(struct frame_queue_t *root, int free_bodies) {
  struct frame_queue_t *cursor;

  for(cursor = root; cursor; /**/) {
    root = cursor->next;
    if(free_bodies)
      free(cursor->f.buf);
    free(cursor);
    cursor = root;
  }
}

int test_frame_enqueue() {
  struct frame_queue_t *root = NULL;
  struct frame_queue_t *cur;

  struct frame_queue_t expected;
  struct frame_queue_t expected2;

  int errs = 0;

  expected.f.buf = "hi mom";
  expected.f.l = strlen("hi mom");
  expected.fixup_seqno = NO_SEQNO_FIXUP;
  expected.next = NULL;

  cur = frame_enqueue(root, "hi mom", strlen("hi mom"), NO_SEQNO_FIXUP);

  expected.f.buf = "wrong";
  if(0 == cmp_frame_queues(cur, &expected, 0)) {
    FAIL("frame_enqueue: cmp sanity check");
    errs++;
  }

  expected.f.buf = "hi mom";
  if(0 != cmp_frame_queues(cur, &expected, 0)) {
    FAIL("frame_enqueue: root null");
    errs++;
  }


  expected2.f.buf = "hi mom";
  expected2.f.l = strlen("hi mom");
  expected2.fixup_seqno = NO_SEQNO_FIXUP;
  expected2.next = NULL;

  expected.next = &expected2;


  cur = frame_enqueue(cur, "hi mom", strlen("hi mom"), NO_SEQNO_FIXUP);

  if(0 != cmp_frame_queues(cur, &expected, 2)) {
    FAIL("frame_enqueue: root not-null");
    errs++;
  }

  free_frame_queue(cur, 0);


  return errs;
}


int test_frame_enqueue_dupfrag() {
  int errs = 0;
  struct frame_queue_t *cur;

  struct frame_queue_t expected;

  expected.f.buf = "mom";
  expected.f.l = strlen("mom");
  expected.fixup_seqno = NO_SEQNO_FIXUP;
  expected.next = NULL;

  cur = frame_enqueue_dupfrag(NULL, "hi mom", 3, strlen("hi mom"), NO_SEQNO_FIXUP);

  if(0 != cmp_frame_queues(cur, &expected, 0)) {
    FAIL("frame_enqueue_dupfrag");
    errs++;
  }
  cur = free_frame_queue(cur, 1);

  expected.f.buf = NULL;
  expected.f.l = 0;

  cur = frame_enqueue_dupfrag(NULL, "hi mom", 37, strlen("hi mom"), NO_SEQNO_FIXUP);
  
  cur = free_frame_queue(cur, 1);

  return errs;
}

int test_frame_enqueue_append() {
  struct frame_queue_t *root;
  struct frame_queue_t *cur;
  struct frame_queue_t *cur2;

  struct frame_queue_t expected;
  struct frame_queue_t expected2;
  struct frame_queue_t expected3;

  struct frame_queue_t *empty_root = NULL;

  int errs = 0;

  expected.f.buf = "hi mom";
  expected.f.l = strlen("hi mom");
  expected.fixup_seqno = NO_SEQNO_FIXUP;
  expected.next = NULL;

  expected2.f.buf = "hi mom2";
  expected2.f.l = strlen("hi mom2");
  expected2.fixup_seqno = NO_SEQNO_FIXUP;
  expected2.next = NULL;

  expected3.f.buf = "hi mom3";
  expected3.f.l = strlen("hi mom3");
  expected3.fixup_seqno = NO_SEQNO_FIXUP;
  expected3.next = NULL;

  /* concatenate */
  expected.next = &expected2;

  root = frame_enqueue(NULL, "hi mom", strlen("hi mom"), NO_SEQNO_FIXUP);
  cur = frame_enqueue(NULL, "hi mom2", strlen("hi mom2"), NO_SEQNO_FIXUP);
  cur2 = frame_enqueue(NULL, "hi mom3", strlen("hi mom3"), NO_SEQNO_FIXUP);

  if(0 != cmp_frame_queues(root, &expected, 0))
    FAIL("frame_enqueue_append: data: root != expected");
  
  if(0 != cmp_frame_queues(cur, &expected2, 0))
    FAIL("frame_enqueue_append: data: cur != expected2");
  
  if(0 != cmp_frame_queues(cur2, &expected3, 0))
    FAIL("frame_enqueue_append: data: cur2 != expected3");


  root = frame_enqueue_append(root, cur);

  if(0 != cmp_frame_queues(root, &expected, 2)) {
    FAIL("frame_enqueue_append: root/expected (1)");
    errs++;
  }

  expected2.next = &expected3;
  root = frame_enqueue_append(root, cur2);

  if(0 != cmp_frame_queues(root, &expected, 2)) {
    FAIL("frame_enqueue_append: root/expected (2)");
    errs++;
  }

  empty_root = frame_enqueue_append(empty_root, root);
  if(0 != cmp_frame_queues(empty_root, &expected, 2)) {
    FAIL("frame_enqueue_append: empty_root");
    errs++;
  }
  
  free_frame_queue(empty_root, 0);

  return errs;
  
}

/******************/


int test_am_init() {
  int errs = 0;

  struct aim_machine s;

  am_init(&s);

  if(s.flow_state != DEAD_AIR) {
    FAIL("test_am_init: flow_state != DEAD_AIR");
    errs++;
  }

  if(s.seqno != BASE_SEQNO) {
    FAIL("test_am_init: seqno != BASE_SEQNO");
    errs++;
  }

  if(s.body_len != 0) {
    FAIL("test_am_init: body_len != 0");
    errs++;
  }

  if(s.body_pos != 0) {
    FAIL("test_am_init: body_pos != 0");
    errs++;
  }

  if(s.body_buf != NULL) {
    FAIL("test_am_init: body_buf != NULL");
    errs++;
  }


  if(s.to_inject != NULL) {
    FAIL("test_am_init: to_inject != NULL");
    errs++;
  }

  if(0 != 0) {
    FAIL("test_am_init:");
    errs++;
  }

  return errs;
}


int test_am_frame_inject() {
  int errs = 0;
  struct aim_machine s;
  struct frame_queue_t *queue = NULL;
  struct frame_queue_t *inject = NULL;
  
  inject = frame_enqueue(inject, "hi mom", strlen("hi mom"), NO_SEQNO_FIXUP);

  s.to_inject = inject;

  queue = am_frame_inject(&s, queue);

  if(0 != cmp_frame_queues(queue, inject, 0)) {
    FAIL("test_am_frame_inject");
    errs++;
  }

  if(0 != 0) {
    FAIL("test_am_frame_inject:");
    errs++;
  }

  free_frame_queue(queue, 0);

  return errs;
}

int test_am_attempt_header_parse() {
  int errs = 0;

  unsigned char header[6] = { 0x2a, 0x01, 0x02,0x03, 0x04,0x05 };

  unsigned char embedded_header[10] = { 'b','l','a','h', 0x2a, 0x01, 0x02,0x03, 0x04,0x05 };
  int embedded_pos = 4;
  unsigned short e_len = HEADER_LEN + 0x0405;


  unsigned char bogus_magic[6] = { 0x22, 0x01, 0x02,0x03, 0x04,0x05 };
  unsigned char bogus_chan[6] = { 0x2a, 0x10, 0x02,0x03, 0x04,0x05 };

  struct aim_machine s;

  am_init(&s);

  if(-1 != am_attempt_header_parse(NULL, bogus_magic, 0)) {
    FAIL("test_am_attempt_header_parse: bogus_magic wrong errno");
    errs++;
  }

  if(-2 != am_attempt_header_parse(NULL, bogus_chan, 0)) {
    FAIL("test_am_attempt_header_parse: bogus_chan wrong errno");
    errs++;
  }


  if(1 != am_attempt_header_parse(&s, header, 0)) {
    FAIL("test_am_attempt_header_parse: header/0 failed");
    errs++;
  }

  if(s.body_pos != HEADER_LEN) {
    FAIL("test_am_attempt_header_parse: header body_pos");
    errs++;
  }

  if(s.body_len != e_len) {
    FAIL("test_am_attempt_header_parse: header body_len");
    errs++;
  }

  if(s.body_buf == NULL) {
    FAIL("test_am_attempt_header_parse: header NULL body_buf");
    errs++;
  }

  am_finish(&s);

  am_init(&s);


  if(1 != am_attempt_header_parse(&s, embedded_header, embedded_pos)) {
    FAIL("test_am_attempt_header_parse: embedded_header/0 failed");
    errs++;
  }

  if(s.body_pos != HEADER_LEN) {
    FAIL("test_am_attempt_header_parse: embedded_header body_pos");
    errs++;
  }

  if(s.body_len != e_len) {
    FAIL("test_am_attempt_header_parse: embedded_header body_len");
    errs++;
  }

  if(s.body_buf == NULL) {
    FAIL("test_am_attempt_header_parse: embedded_header NULL body_buf");
    errs++;
  }

  am_finish(&s);

  return errs;
}

int test_scan_to_header() {
  int errs = 0;

  int i;

  unsigned char bufs[4][64] = {
    { 0x2a, 0x01, 0x02,0x03, 0x04,0x05, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, }, /* 0 */
    { 'b','l','a','h', 0x2a, 0x01, 0x02,0x03, 0x04,0x05, 0x00,0x00,0x00,0x00,0x00,0x00, }, /* 4 */
    "Ceci n'est pas une ICBM" , /* -1 */
    { 0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00, 0x00,0x00, 0x2a,0x01, 0x02,0x03, 0x04,0x05, 0x00,0x00,0x00,0x00,0x00, }, /* -1: straddles the 24 limit below */
  };

  int poses[] = { 0, 4, -1, -1, -2/* guard */ };

  for(i = 0; bufs[i] != NULL && poses[i] != -2; i++) {
    struct frame_t f;
    int pos;

    f.buf = bufs[i];
    f.l = 24;

    pos = am_scan_to_header(&f, 0);


    if(0 != 0) {
      FAIL("test_scan_to_header:");
      errs++;
    }


  }

  return errs;
}


/******************/
int test_run_passthrough() {
  int errs = 0;

  unsigned char *buf = "*Ceci n'est pas une ICBM";

  struct frame_t f;
  struct frame_queue_t expected;
  struct frame_queue_t *result;

  struct aim_machine s;

  am_init(&s);

  f.buf = buf;
  f.l = strlen(buf);

  expected.f.buf = buf;
  expected.f.l = strlen(buf);
  expected.fixup_seqno = NO_SEQNO_FIXUP;
  expected.next = NULL;
  
  result = am_run(&s, &f);

  if(0 != cmp_frame_queues(&expected, result, 2)) {
    FAIL("test_run_passthrough");
    errs++;
  }

  if(0 != 0) {
    FAIL("test_run_passthrough:");
    errs++;
  }

  free_frame_queue(result, 1);

  am_finish(&s);

  return errs;
}

int test_run_icbmonly() {
  int errs = 0;

  unsigned char buf[] = { 0x2a, 0x00, 0x12,0x23, 0x00,0x04, 0x10, 0x09, 0x08, 0x07 };

  struct frame_t f;
  struct frame_queue_t expected;
  struct frame_queue_t *result = NULL;

  struct aim_machine s;

  f.buf = buf;
  f.l = 10;

  am_init(&s);

  expected.f.buf = (unsigned char *)malloc(10);
  memcpy(expected.f.buf, f.buf, 10);
  am_apply_seqno(&s, expected.f.buf);

  expected.f.l = 10;
  expected.fixup_seqno = SEQNO_FIXUP_YES;
  expected.next = NULL;

  result = am_run(&s, &f);

  if(0 != cmp_frame_queues(&expected, result, 2)) {
    FAIL("test_run_icbmonly");
    errs++;
  }

  free(expected.f.buf);
  free_frame_queue(result, 1);

  am_finish(&s);

  return errs;
}

int test_run_mixed() {
  int errs = 0;

  unsigned char *nonicbm = "*Ceci n'est pas une ICBM";
  int len_nonicbm = strlen(nonicbm);

  unsigned char icbm[] = { 0x2a, 0x00, 0x12,0x23, 0x00,0x04, 0x10, 0x09, 0x08, 0x07 };
  int len_icbm = 10;

  unsigned char *nonicbm2 = "Still not a pipe";
  int len_nonicbm2 = strlen(nonicbm2);


  unsigned char *combined;

  struct frame_t f;
  struct frame_queue_t expected_nonicbm;
  struct frame_queue_t expected_icbm;
  struct frame_queue_t expected_nonicbm2;
  struct frame_queue_t *result = NULL;

  struct aim_machine s;

  combined = (unsigned char*)malloc(len_nonicbm + len_icbm + len_nonicbm2);
  memcpy(combined, nonicbm, len_nonicbm);
  memcpy(combined+len_nonicbm, icbm, len_icbm);
  memcpy(combined+len_nonicbm+len_icbm, nonicbm2, len_nonicbm2);

  f.buf = combined;
  f.l = len_nonicbm + len_icbm + len_nonicbm2;

  am_init(&s);

  expected_nonicbm.f.buf = nonicbm;
  expected_nonicbm.f.l = len_nonicbm;
  expected_nonicbm.fixup_seqno = NO_SEQNO_FIXUP;
  expected_nonicbm.next = NULL;

  expected_icbm.f.buf = icbm;
  am_apply_seqno(&s, expected_icbm.f.buf);
  expected_icbm.f.l = len_icbm;
  expected_icbm.fixup_seqno = SEQNO_FIXUP_YES;
  expected_icbm.next = NULL;

  expected_nonicbm2.f.buf = nonicbm2;
  expected_nonicbm2.f.l = len_nonicbm2;
  expected_nonicbm2.fixup_seqno = NO_SEQNO_FIXUP;
  expected_nonicbm2.next = NULL;


  expected_nonicbm.next = &expected_icbm;
  expected_icbm.next = &expected_nonicbm2;
  
  result = am_run(&s, &f);

  if(0 != cmp_frame_queues(&expected_nonicbm, result, 2)) {
    FAIL("test_run_mixed");
    errs++;
  }

  if(0 != 0) {
    FAIL("test_run_mixed:");
    errs++;
  }

  free(combined);
  result = free_frame_queue(result, 1);

  return errs;
}

/******************/
int test_am_inject_frame() {
  int errs = 0;

  struct aim_machine s;

  unsigned char *buf = "hi";
  int l = strlen(buf);
  int fixup = NO_SEQNO_FIXUP;

  struct frame_t f;
  struct frame_queue_t *queue = NULL;

  am_init(&s);

  queue = frame_enqueue(queue, buf, l, fixup);

  f.buf = strdup(buf);
  f.l = l;

  am_inject_frame(&s, &f, fixup);

  if(0 != cmp_frame_queues(queue, s.to_inject, 2)) {
    FAIL("test_am_inject_frame");
    errs++;
  }

  queue = free_frame_queue(queue, 0);

  am_finish(&s);

  return errs;
}

int test_am_inject_with_icbm() {
  int errs = 0;

  struct aim_machine s;

  unsigned char buf[] =  { 0x2a, 0x00, 0x22,0x22, 0x00,0x04, 0x20, 0x19, 0x18, 0x17 };
  int l = 10;
  int fixup = SEQNO_FIXUP_YES;
  struct frame_t f_inj;

  unsigned char icbm[] = { 0x2a, 0x00, 0x12,0x23, 0x00,0x04, 0x10, 0x09, 0x08, 0x07 };
  int len_icbm = 10;

  unsigned char *combined;

  struct frame_t f;
  struct frame_queue_t expected_injected;
  struct frame_queue_t expected_icbm;
  struct frame_queue_t *result = NULL;

  f_inj.buf = strdup(buf);
  f_inj.l = l;

  am_init(&s);
  am_inject_frame(&s, &f_inj, fixup);

  combined = (unsigned char*)malloc(len_icbm);
  memcpy(combined, icbm, len_icbm);
  f.buf = combined;
  f.l = len_icbm;

  expected_injected.f.buf = (unsigned char *)malloc(l);
  memcpy(expected_injected.f.buf, buf, l);
  am_apply_seqno(&s, expected_injected.f.buf);
  s.seqno++;
  expected_injected.f.l = l;
  expected_injected.fixup_seqno = SEQNO_FIXUP_YES;
  expected_injected.next = NULL;

  expected_icbm.f.buf = icbm;
  am_apply_seqno(&s, expected_icbm.f.buf);
  s.seqno--; /* Restore state! */
  expected_icbm.f.l = len_icbm;
  expected_icbm.fixup_seqno = SEQNO_FIXUP_YES;
  expected_icbm.next = NULL;

  expected_injected.next = &expected_icbm;
  
  result = am_run(&s, &f);

  if(0 != cmp_frame_queues(&expected_injected, result, 2)) {
    FAIL("test_am_inject_with_icbm:");

    if(0 != cmp_frames(&(expected_injected.f), &(result->next->f)))
      FAIL("test_am_inject_with_icbm: injected/result2");

    if(0 != cmp_frames(&(expected_icbm.f), &(result->next->next->f)))
      FAIL("test_am_inject_with_icbm: icbm/result3");


    errs++;
  }

  if(0 != 0) {
    FAIL("test_am_inject_with_icbm:");
    errs++;
  }

  free(combined);
  free_frame_queue(result, 1);
  free(expected_injected.f.buf);

  am_finish(&s);

  return errs;
}


int test_am_inject_with_mixed() {
  int errs = 0;

  struct aim_machine s;

  unsigned char buf[] =  { 0x2a, 0x00, 0x22,0x22, 0x00,0x04, 0x20, 0x19, 0x18, 0x17 };
  int l = 10;
  int fixup = SEQNO_FIXUP_YES;
  struct frame_t f_inj;


  unsigned char *nonicbm = "*Ceci n'est pas une ICBM";
  int len_nonicbm = strlen(nonicbm);

  unsigned char icbm[] = { 0x2a, 0x00, 0x12,0x23, 0x00,0x04, 0x10, 0x09, 0x08, 0x07 };
  int len_icbm = 10;

  unsigned char *combined;

  struct frame_t f;
  struct frame_queue_t expected_nonicbm;
  struct frame_queue_t expected_injected;
  struct frame_queue_t expected_icbm;
  struct frame_queue_t *result = NULL;

  f_inj.buf = (unsigned char*)malloc(l);
  memcpy(f_inj.buf, buf, l);
  f_inj.l = l;


  am_init(&s);
  am_inject_frame(&s, &f_inj, fixup);

  combined = (unsigned char*)malloc(len_nonicbm + len_icbm);
  memcpy(combined, nonicbm, len_nonicbm);
  memcpy(combined+len_nonicbm, icbm, len_icbm);

  f.buf = combined;
  f.l = len_nonicbm + len_icbm;

  expected_injected.f.buf = (unsigned char *)malloc(l);
  memcpy(expected_injected.f.buf, buf, l);
  am_apply_seqno(&s, expected_injected.f.buf);
  s.seqno++;
  expected_injected.f.l = l;
  expected_injected.fixup_seqno = SEQNO_FIXUP_YES;
  expected_injected.next = NULL;

  expected_nonicbm.f.buf = nonicbm;
  expected_nonicbm.f.l = len_nonicbm;
  expected_nonicbm.fixup_seqno = NO_SEQNO_FIXUP;
  expected_nonicbm.next = NULL;

  expected_icbm.f.buf = icbm;
  am_apply_seqno(&s, expected_icbm.f.buf);
  s.seqno--; /* Restore state! */
  expected_icbm.f.l = len_icbm;
  expected_icbm.fixup_seqno = SEQNO_FIXUP_YES;
  expected_icbm.next = NULL;

  expected_injected.next = &expected_nonicbm;
  expected_nonicbm.next = &expected_icbm;
  
  result = am_run(&s, &f);

  if(0 != cmp_frame_queues(&expected_injected, result, 2)) {
    FAIL("test_am_inject_with_mixed: frame queues don't match");

    if(0 != cmp_frames(&(expected_injected.f), &(result->f)))
      FAIL("test_am_inject_with_mixed: nonicbm/result");

    if(0 != cmp_frames(&(expected_nonicbm.f), &(result->next->f)))
      FAIL("test_am_inject_with_mixed: injected/result2");

    if(0 != cmp_frames(&(expected_icbm.f), &(result->next->next->f)))
      FAIL("test_am_inject_with_mixed: icbm/result3");

    errs++;
  }

  if(0 != 0) {
    FAIL("test_am_inject_with_mixed:");
    errs++;
  }

  free_frame_queue(result, 1);
  free(combined);
  free(expected_injected.f.buf);

  am_finish(&s);

  return errs;
}


int test_run_fragmented() {
  int errs = 0;

  unsigned char buf[] = { 0x2a, 0x00, 0x12,0x23, 0x00,0x04, 0x10, 0x09, 0x08, 0x07 };

  struct frame_t f0, f1;
  struct frame_queue_t expected;
  struct frame_queue_t *result = NULL;

  struct aim_machine s;

  f0.buf = (unsigned char *)malloc(6);
  memcpy(f0.buf, buf, 6);
  f0.l = 6;

  f1.buf = (unsigned char *)malloc(4);
  memcpy(f1.buf, buf+6, 4);
  f1.l = 4;

  am_init(&s);

  expected.f.buf = (unsigned char *)malloc(10);
  memcpy(expected.f.buf, buf, 10);
  am_apply_seqno(&s, expected.f.buf);

  expected.f.l = 10;
  expected.fixup_seqno = SEQNO_FIXUP_YES;
  expected.next = NULL;
  
  result = am_run(&s, &f0);
  if(NULL != result) {
    FAIL("test_run_fragmented: non-null result for incomplete ICBM");
    errs++;
  }

  result = am_run(&s, &f1);

  if(0 != cmp_frame_queues(&expected, result, 2)) {
    FAIL("test_run_icbmonly: incorrect result after complete body");
    errs++;
  }

  if(0 != 0) {
    FAIL("test_run_fragmented:");
    errs++;
  }

  free(f0.buf);
  free(f1.buf);

  free(expected.f.buf);

  free_frame_queue(result, 1);

  return errs;
}


int test_run_invalid() {
  int errs = 0;

  struct aim_machine s;
  struct frame_t f;

  am_init(&s);
  f.buf = "hi mom";
  f.l = strlen(f.buf);

  if(NULL != am_run(NULL,&f)) {
    FAIL("test_run_invalid: NULL s no error");
    errs++;
  }
  if(NULL != am_run(&s,NULL)) {
    FAIL("test_run_invalid: NULL f no error");
    errs++;
  }

  s.flow_state = 999;
  if(NULL != am_run(&s,&f)) {
    FAIL("test_run_invalid: invalid flow_state no error");
    errs++;
  }


  am_finish(&s);

  return errs;
}


/* a placeholder for ensuring we get called back the right number of
   times */
static int G_dummycount = 0;

static unsigned char G_sn[255];
static unsigned char G_msg[8192];
static int G_oncoming;
static int G_state;
static int G_idletime;


int cb_presence(struct aim_machine *s, ...) {
  va_list ap;
  unsigned char *sn;
  int oncoming;
  int state;
  int idletime;

  va_start(ap, s);
  sn = va_arg(ap, unsigned char *);
  oncoming = va_arg(ap, int);
  state = va_arg(ap, int);
  idletime = va_arg(ap, int);
  va_end(ap);

  strncpy(G_sn, sn, 255);
  G_dummycount++;

  G_oncoming = oncoming;
  G_state = state;
  G_idletime = idletime;

  return 0;
}


int test_parse_oncoming() {
  int errs = 0;
  struct aim_machine s;

  unsigned char *expected = "sharethisdotcom";

  /* This gives us int packet_len and unsigned char packet[] locally... */
#include "packets/oncoming.c"

  am_init(&s);

  am_register_event_cb(&s, 0xDEAD, 0xBEEF, cb_presence);

  G_dummycount = 0;
  am_parse_body(&s, (unsigned char *)packet, packet_len);

  if(0 != G_dummycount) {
    FAIL("test_oncoming: counter was incorrectly incremented");
    errs++;
  }

  am_register_event_cb(&s, 0x0003, 0x000b, cb_presence);

  G_dummycount = 0;
  memset(G_sn, 0x00, 255);
  am_parse_body(&s, (unsigned char *)packet, packet_len);

  if(1 != G_dummycount) {
    FAIL("test_oncoming: counter wasn't incremented");
    errs++;
  }

  if(0 != strcmp(expected, G_sn)) {
    printf("exp='%s', got = '%s'\n", expected, G_sn);
    FAIL("test_oncoming: screenname incorrect");
    errs++;
  }

  am_finish(&s);

  return errs;
}

int test_parse_offgoing() {
  int errs = 0;
  struct aim_machine s;

  unsigned char *expected = "jbmcontract";

  /* This gives us int packet_len and unsigned char packet[] locally... */
#include "packets/offgoing.c"

  am_init(&s);

  am_register_event_cb(&s, 0xDEAD, 0xBEEF, cb_presence);

  G_dummycount = 0;
  am_parse_body(&s, (unsigned char *)packet, packet_len);

  if(0 != G_dummycount) {
    FAIL("test_offgoing: counter was incorrectly incremented");
    errs++;
  }

  am_register_event_cb(&s, 0x0003, 0x000c, cb_presence);

  G_dummycount = 0;
  memset(G_sn, 0x00, 255);
  am_parse_body(&s, (unsigned char *)packet, packet_len);

  if(1 != G_dummycount) {
    FAIL("test_offgoing: counter wasn't incremented");
    errs++;
  }

  if(0 != strcmp(expected, G_sn)) {
    printf("exp='%s', got = '%s'\n", expected, G_sn);
    FAIL("test_offgoing: screenname incorrect");
    errs++;
  }

  am_finish(&s);

  return errs;
}

int cb_msg_outbound(struct aim_machine *s, ...) {
  unsigned char *sn;
  unsigned char *msg;
  va_list ap;

  va_start(ap, s);
  sn = va_arg(ap, unsigned char *);
  msg = va_arg(ap, unsigned char *);
  va_end(ap);

  /* printf("outbound message to %s: %s\n", sn, msg); */

  strncpy(G_sn, sn, 255);
  strncpy(G_msg, msg, 8192);

  return 0;
}




int test_parse_msg() {
  int errs = 0;
  struct aim_machine s;

  /* This gives us int packet_len and unsigned char packet[] locally... */
#include "packets/msg_outbound.c"

  am_init(&s);

  am_register_event_cb(&s, 0x0004, 0x0006, cb_msg_outbound);

  am_parse_body(&s, (unsigned char *)packet, packet_len);

  if(!G_sn || 0 != strcmp(G_sn, "screennamex")) {
    FAIL("test_parse_msg: sn incorrect");
    errs++;
  }

  if(!G_msg || 0 != strcmp(G_msg, "aaaaaaaaaaaaaaaaaaaaaa")) {
    FAIL("test_parse_msg: message incorrect");
    errs++;
  }

  am_finish(&s);

  return errs;
}

int test_format_oft_sendfile_req() {
  int errs = 0;
  struct frame_t expected;
  struct frame_t *got;

  unsigned char ip[4] = { 192, 168, 1, 105 };
  unsigned short port = 2071;

#include "packets/oscar_sendfile_req.c"  

  expected.buf = packet;
  expected.l = packet_len;

  got = am_format_oft_sendfile_req("screennamex", "snac", "mycookie", ip, port, "aaaaaaaaaaaaa.txt.txt", 40);

  if(0 != cmp_frames(&expected, got)) {
    FAIL("test_format_oscar_sendfile_req: frames don't match");
    errs++;
  }

  free(got->buf);
  free(got);

  return errs;
}


int test_sanitize() {
  int errs = 0;

  unsigned char cookie_miss[] = "mycookie";
  unsigned char cookie_hit[] = { 0x6b, 0x25, 0xd5, 0x6c, 0x83, 0x71, 0xe8, 0x73 };

  struct aim_machine s;

#include "packets/oscar_cookie.c"

  am_init(&s);

  if(0 != am_cur_body_sanitized(&s, packet)) {
    FAIL("test_sanitize: got a false positive on empty list");
    errs++;
  }

  am_add_forbidden_cookie(&s, cookie_miss);

  if(0 != am_cur_body_sanitized(&s, packet)) {
    FAIL("test_sanitize: got a false positive with a miss in list");
    errs++;
  }

  am_add_forbidden_cookie(&s, cookie_hit);
  if(1 != am_cur_body_sanitized(&s, packet)) {
    FAIL("test_sanitize: got a false negative");
    errs++;
  }

  if(0 != 0) {
    FAIL("test_sanitize:");
    errs++;
  }

  am_finish(&s);

  return errs;
}


int test_format_message_outbound() {
  int errs = 0;
  char snac[] = "snac";
  char cookie[] = "mycookie";
  char sn[] = "screennamex";
  char *message = "aaaaaaaaaaaaaaaaaaaaaa";

#include "packets/msg_outbound.c"

  struct frame_t expected;
  struct frame_t *got;

  got = am_format_message_outbound(snac, cookie, sn, message);

  expected.buf = packet;
  expected.l = packet_len;

  if(0 != cmp_frames(&expected, got)) {
    FAIL("test_format_message_for_inject: frames not equal");
    errs++;
  }

  free(got->buf);
  free(got);

  return errs;
}


int test_format_message_inbound() {
  int errs = 0;
  char snac[] = "snac";
  char cookie[] = "mycookie";
  char sn[] = "screennamex";
  char *message = "and it still works.  go figure.";

#include "packets/msg_inbound.c"

  struct frame_t expected;
  struct frame_t *got;

  got = am_format_message_inbound(snac, cookie, sn, message);

  expected.buf = packet;
  expected.l = packet_len;

  if(0 != cmp_frames(&expected, got)) {
    FAIL("test_format_message_for_inject: frames not equal");
    errs++;
  }

  free(got->buf);
  free(got);

  return errs;
}


/******************/
int test_() {
  int errs = 0;



  if(0 != 0) {
    FAIL("test_:");
    errs++;
  }

  return errs;
}


/**********************************************************************
 *************** RUNNER ***********************************************
 *********************************************************************/

typedef int(*test_function)();

int main(int argc, char* argv[]) {
  int errors = 0;

  test_function tests[] = {
    test_frame_enqueue,
    test_frame_enqueue_dupfrag,
    test_frame_enqueue_append,
    test_am_init,
    test_am_frame_inject,
    test_am_attempt_header_parse,
    test_scan_to_header,
    test_run_passthrough,
    test_run_icbmonly,
    test_run_mixed,
    test_am_inject_frame,
    test_am_inject_with_mixed,
    test_am_inject_with_icbm,
    test_run_fragmented,
    test_run_invalid,
    test_parse_oncoming,
    test_parse_offgoing,
    test_parse_msg,
    test_format_oft_sendfile_req,
    test_format_message_inbound,
    test_format_message_outbound,
    test_sanitize,
  };

  int i;

  int n_tests;

  test_function curtest;

  n_tests = sizeof(tests)/sizeof(curtest);

  for(i = 0; i < n_tests; i++) {
    curtest = tests[i];
    errors += curtest();
  }


  if(errors) {
    printf("******************************************\n");
    printf("******************************************\n");
    printf("******************************************\n");
  }

  printf("Got %d errors in %d tests\n", errors, n_tests);

  return 0;
}
