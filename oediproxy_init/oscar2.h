#ifndef __OSCAR2_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define aimutil_get16(buf) ((((*(buf))<<8)&0xff00) + ((*((buf)+1)) & 0xff))
#define aimutil_put16(buf, x) *(buf) = (((x)&0xff00) >> 8); *((buf)+1) = x & 0xff;

  struct frame_t {
    char *buf;
    int l;
  };

  struct frame_queue_t {
    struct frame_t f;
    int fixup_seqno; /* Do we need to do seqno fixup? */
    struct frame_queue_t *next;
  };

  struct sanitize_entry {
    unsigned short family;
    unsigned short subtype;
    void *priv_match;
  };

  struct sanitize_list_t {
    struct sanitize_entry se;
    struct sanitize_list_t *next;
  };

  enum { HEADER_LEN = 6 };

  enum { NO_SEQNO_FIXUP = 0, SEQNO_FIXUP_YES = 1 };

  struct oedifilter_state {
    enum flow_state_t { PREHEADER, INHEADER, INBODY } flow_state;
    signed short seqno;
    char header[HEADER_LEN];
    unsigned short body_len;
    char *body_buf;
    unsigned short body_pos;

    struct frame_queue_t *to_inject;
    struct sanitize_list_t *to_sanitize;

  };


  /* exported interfaces */

  struct frame_queue_t * of_run(struct oedifilter_state *, struct frame_t *);


#ifdef __OSCAR2_TEST__
    /* Private/"static" interfaces only exposed for testing */
  struct frame_queue_t *frame_enqueue(struct frame_queue_t *, char *, int, int);
  struct frame_queue_t *frame_enqueue_dupfrag(struct frame_queue_t *, char *, int, int, int);
  struct frame_queue_t *frame_enqueue_append(struct frame_queue_t *, struct frame_queue_t *);

  void of_init(struct oedifilter_state *s);
  
  struct frame_queue_t *of_frame_inject(struct oedifilter_state *, struct frame_queue_t *) ;
  int of_attempt_header_parse(struct oedifilter_state *, char *, int);
  int of_scan_to_header(struct frame_t *f, int);
  int of_load_body(struct oedifilter_state *, struct frame_queue_t **, char *, int, int);
#endif /* def __OSCAR2_TEST__ */



#ifdef __cplusplus
}
#endif

#endif /* ndef __OSCAR2_H__ */
