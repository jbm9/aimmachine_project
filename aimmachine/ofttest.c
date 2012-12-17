#define __OFT_TEST__
#include "oft.h"

/* This is a kludge for this file only, since we use static strings as
   queue bodies and don't necessarily consume the output frames */
static struct frame_queue_t * free_frame_queue(struct frame_queue_t *root, int free_bodies) {
  struct frame_queue_t *cursor;

  for(cursor = root; cursor; /**/) {
    root = cursor->next;
    if(free_bodies)
      free(cursor->f.buf);
    free(cursor);
    cursor = root;
  }

  return NULL;
}


static int G_closed = 0;
int close_conn(int token) { G_closed++; return 0; }

static unsigned char G_filecontent[8192];
static int G_filepos = 0;
int append_to_file(int token, unsigned char *buf, int len) {
  memcpy(G_filecontent+G_filepos, buf, len);
  G_filepos += len;
  return len;
}



/******************/
int test_rx_machine() {
  int errs = 0;

#include "packets/oft_rx_trans.c"

  struct oft_rx_machine m;
  struct aim_fileheader_t fh;

  struct frame_queue_t *queue;
  
  unsigned char expected_ack[512];
  unsigned char expected_termack[512];

  struct frame_t expected_frame;

  G_closed = 0;
  memset(G_filecontent, 0, 8192);
  G_filepos = 0;

  memcpy(expected_ack, packet, packet_len);
  aimutil_put16(expected_ack+6, 0x0202);

  memcpy(expected_termack, packet, packet_len);
  aimutil_put16(expected_termack+6, 0x0204);

  aim_oft_getfh(&fh, (unsigned char*)packet);

  memset(cookie, 0x00, 8);
  oft_rx_init(&m, cookie, close_conn, append_to_file, 0x9981);

  queue = oft_rx_run(&m, (unsigned char*)packet, packet_len);

  if(!queue) {
    FAIL("test_rx_machine: queue empty after PROMPT");
    errs++;
  }

  expected_frame.buf = expected_ack;
  expected_frame.l = packet_len;

  if(0 != cmp_frames(&expected_frame, &queue->f)) {
    FAIL("test_rx_machine: ACK frames not equal");
    errs++;
  }

  queue = free_frame_queue(queue, 1);

  queue = oft_rx_run(&m, (unsigned char *)file, file_len);

  if(!queue) {
    FAIL("test_rx_machine: queue empty after FILE");
    errs++;
  }

  expected_frame.buf = expected_termack;
  expected_frame.l = packet_len;

  if(0 != cmp_frames(&expected_frame, &queue->f)) {
    FAIL("test_rx_machine: TERMACK frames not equal");
    errs++;
  }

  if(file_len != G_filepos) {
    FAIL("test_rx_machine: file length differs");
    errs++;
  }

  if(0 != strcmp(file, G_filecontent)) {
    FAIL("test_rx_machine: file content differs");
    errs++;
  }

  /* I'm not certain how to handle the close case here just yet.
  queue = oft_rx_run(&m, NULL, 0);

    
  if(queue){
    FAIL("test_rx_machine: got a queued message after close(?)");
    errs++;
  }

  if(1 != G_closed) {
    FAIL("test_rx_machine: closed number isn't one after close");
    errs++;
    }*/

  free_frame_queue(queue, 1);

  return errs;
}

static int G_began_xfer = 0;
int begin_xfer(struct oft_tx_machine *m, int token) {
  G_began_xfer++;
  return 0;
}

static int G_file_pos;
int resume_offset(struct oft_tx_machine *m, int token, int pos) {
  G_file_pos = pos;
  return 0;
}


int test_tx_machine() {
  int errs = 0;

  struct oft_tx_machine m;
  struct aim_fileheader_t fh;

  struct frame_queue_t *queue;

#include "packets/oft_tx_trans.c"

  struct frame_t expected_frame_termack;
  struct frame_t expected_frame_ack;
  struct frame_t expected_frame_prompt;
  
  expected_frame_termack.buf = (unsigned char *)packet_termack;
  expected_frame_ack.buf = (unsigned char *)packet_ack;
  expected_frame_prompt.buf = (unsigned char *)packet_prompt;

  expected_frame_termack.l = packet_len;  
  expected_frame_ack.l = packet_len;  
  expected_frame_prompt.l = packet_len;


  /* this is technically cheating, but we do trust our
     de/serialization to be idempotent after the checks above. 
     Ideally, we'd have a dummy fh here instead of a parsed one.
  */
  aim_oft_getfh(&fh, packet_prompt);
  
  oft_tx_init(&m, &fh, close_conn, begin_xfer, resume_offset, 0x9981);

  if(0 != oft_tx_accepted(&m)) {
    FAIL("test_tx_machine: got non-zero reply from accepted-without-oob");
    errs++;
  }
  if(1 != oft_tx_got_ooback(&m)) {
    FAIL("test_tx_machine: got_ooback: not qualified to send here(?)");
    errs++;
  }

  queue = oft_tx_run(&m, NULL, 0);

  if(!queue) {
    FAIL("test_tx_machine: no queue after request for prompt");
    errs++;
  }

  if(0 != cmp_frames(&expected_frame_prompt, &(queue->f))) {
    FAIL("test_tx_machine: frames unequal");
    errs++;
  }

  queue = free_frame_queue(queue, 1);

  /* send prompt, get ack... */

  G_began_xfer = 0;
  queue = oft_tx_run(&m, (unsigned char *)packet_ack, packet_len);

  if(queue) {
    FAIL("test_tx_machine: got buffers in response to the ACK read!");
    errs++;
  }

  if(1 != G_began_xfer) {
    FAIL("test_tx_machine: G_began_xfer not 1 after ACK");
    errs++;
  }

  queue = free_frame_queue(queue, 1);


  /* Send the file */
  oft_tx_sent_bytes(&m, m.fh.size);

  /* Get back the TERMACK from the remote side, close conn */
  G_closed = 0;

  queue = oft_tx_run(&m, (unsigned char *)packet_termack, packet_len);

  if(queue) {
    FAIL("test_tx_machine: got queue in TERMACK read");
    errs++;
  }

  if(1 != G_closed) {
    FAIL("test_tx_machine: TERMACK closed count incorrect");
    errs++;
  }


  if(0 != 0) {
    FAIL("test_tx_machine:");
    errs++;
  }

  free_frame_queue(queue, 1);

  return errs;
}


/******************/
int test_roundtrip() {
  int errs = 0;
  struct aim_fileheader_t fh_tx, fh_rx;

  struct oft_rx_machine rx;
  struct oft_tx_machine tx;

  struct frame_queue_t *queue_rx, *queue_tx;

#include "packets/oft_tx_trans.c"

  G_closed = 0;
  memset(G_filecontent, 0, 8192);
  G_filepos = 0;

  G_began_xfer = 0;
  G_closed = 0;


  /* SEND choose the file to send, set up fileheader */
  aim_oft_getfh(&fh_tx, (unsigned char*)packet_prompt);

  /* SEND OFT->  establish listener, attach tx machine to it */
  oft_tx_init(&tx, &fh_tx, close_conn, begin_xfer, resume_offset, 0x9981);

  /* SEND OSCAR-> send invitation, including cookie */
  /* DEST <-OSCAR get invitation, including cookie */

  /* DEST OFT-> connect to remote side, attach rx machine */
  oft_rx_init(&rx, cookie, close_conn, append_to_file, 0x9981);

  /* SEND <-OFT get connection */
  if(1 == oft_tx_accepted(&tx)) {
    FAIL("test_roundtrip: oft_tx_accept has queue: got queue too early");
    errs++;
  }

  /* DEST OSCAR-> send OOB ack */
  /* SEND <-OSCAR get OOB ack */

  if(1 == oft_tx_got_ooback(&tx)) {
    queue_tx = oft_tx_run(&tx, NULL, 0);

    if(queue_tx->next) {
      FAIL("test_roundtrip: oft_tx_got_ooback: queue_tx more than one element");
      errs++;
    }
    
    if(queue_rx) {
      FAIL("test_roundtrip: oft_tx_got_ooback: queue_rx not-empty!");
      errs++;
    }
  } else {
    FAIL("test_roundtrip: oft_tx_accept has queue: got queue too early");
    errs++;
  }

  /* DEST <-OFT get OFT PROMPT */
  if(queue_tx) {
    queue_rx = oft_rx_run(&rx, queue_tx->f.buf, queue_tx->f.l);
    
    queue_tx = free_frame_queue(queue_tx, 1);
  }

  /* DEST OFT-> send OFT ACK */
  /* SEND <-OFT get OFT ACK */
  if(!queue_rx) {
    FAIL("test_roundtrip: oft_rx_run after PROMPT empty queue");
    errs++;
  } else {
    queue_tx = oft_tx_run(&tx, queue_rx->f.buf, queue_rx->f.l);
    queue_rx = free_frame_queue(queue_rx, 1);
    
  }

  /* SEND OFT-> send file content */
  if(1 != G_began_xfer) {
    FAIL("test_roundtrip: sender didn't begin transfer on cue");
    errs++;
  }
  oft_tx_sent_bytes(&tx, file_len);

  /* DEST <-OFT get file content */
  queue_rx = oft_rx_run(&rx, file, file_len);
  /* DEST OFT-> send OFT TERM ACK */

  if(file_len != G_filepos) {
    FAIL("test_roundtrip: file length differs");
    errs++;
  }

  if(0 != strcmp(file, G_filecontent)) {
    FAIL("test_roundtrip: file content differs");
    errs++;
  }

  if(!queue_rx) {
    FAIL("test_roundtrip: receiver didn't enqueue TERMACK");
    errs++;
  } else {
    /* SEND <-OFT get OFT TERM ACK */
    queue_tx = oft_tx_run(&tx, queue_rx->f.buf, queue_rx->f.l);
    queue_rx = free_frame_queue(queue_rx, 1);
    queue_tx = free_frame_queue(queue_tx, 1);
    /* SEND +OFT+ close OFT conn */
  }

  if(1 != G_closed) {
    FAIL("test_roundtrip: closed != 1 ");
    errs++;
  }


  if(0 != 0) {
    FAIL("test_roundtrip:");
    errs++;
  }


  if(0 != 0) {
    FAIL("test_roundtrip:");
    errs++;
  }

  free_frame_queue(queue_tx, 1);
  free_frame_queue(queue_rx, 1);

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
    test_roundtrip,
    test_rx_machine,
    test_tx_machine,
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
