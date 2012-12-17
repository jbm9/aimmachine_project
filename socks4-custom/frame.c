#include "oscar2.h"
#include "frame.h"

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

  if(!q && !p) return 0;
  if(!q) return -1;
  if(!p) return 1;

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
/*
struct frame_queue_t* frame_queue_free(struct frame_queue_t *q) {
  struct frame_queue_t *p;

  while(q) {
    p = q->next;
    free(q->f.buf);
    free(q);
    q = p
  }
  return NULL;
}
*/

struct frame_queue_t *frame_enqueue(struct frame_queue_t *root, unsigned char *buf, int l, int fixup_seqno) {
  struct frame_queue_t *my_frame = (struct frame_queue_t *)malloc(sizeof(struct frame_queue_t));
  struct frame_queue_t *cur;

  FUNC("frame_enqueue");

  my_frame->f.buf = buf;
  my_frame->f.l = l;
  my_frame->fixup_seqno = fixup_seqno;
  my_frame->next = NULL;

  cur = root;

  if(!cur)
    return my_frame;

  while(cur->next)
    cur = cur->next;

  cur->next = my_frame;
  return root;
}

struct frame_queue_t *frame_enqueue_dupfrag(struct frame_queue_t *cur, unsigned char *buf, int start, int end, int fixup_seqno) {
  unsigned char *my_buf;
  int my_len;

  FUNC("frame_enqueue_dupfrag");

  my_len = end - start;

  if(0 >= my_len) {
    my_len = 0;
    my_buf = NULL;
  } else {
    my_buf = (unsigned char *)malloc(my_len);
    memcpy(my_buf, buf+start, my_len);
  }

  return frame_enqueue(cur, my_buf, my_len, fixup_seqno);
}

struct frame_queue_t *frame_enqueue_append(struct frame_queue_t *queue, struct frame_queue_t *tail) {
  FUNC("frame_enqueue_append");

  if(!queue) {
    queue = tail;
  } else { 
    struct frame_queue_t *cursor = queue;
    while(cursor->next)
      cursor = cursor->next;
    cursor->next = tail;
  }

  return queue;
}

