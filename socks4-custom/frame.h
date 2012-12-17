#ifndef __FRAME_H__
#define __FRAME_H__

#ifdef __cplusplus
extern "C" {
#endif

  struct frame_t {
    unsigned char *buf;
    int l;
  };
  
  struct frame_queue_t {
    struct frame_t f;
    int fixup_seqno; /* Do we need to do seqno fixup? */
    struct frame_queue_t *next;
  };

  struct frame_queue_t *frame_enqueue(struct frame_queue_t *, unsigned char *, int, int);
  struct frame_queue_t *frame_enqueue_dupfrag(struct frame_queue_t *, unsigned char *, int, int, int);
  struct frame_queue_t *frame_enqueue_append(struct frame_queue_t *, struct frame_queue_t *);

  int cmp_frames(struct frame_t *f, struct frame_t *g);
  int cmp_frame_queues(struct frame_queue_t *q, struct frame_queue_t *p, int include_next);

#ifdef __cplusplus
}
#endif

#endif /* ndef __FRAME_H__ */
