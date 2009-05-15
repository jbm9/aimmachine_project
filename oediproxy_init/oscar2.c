#include "oscar2.h"

struct frame_queue_t *frame_enqueue(struct frame_queue_t *cur, char *buf, int l, int fixup_seqno) {
  struct frame_queue_t *my_frame = (struct frame_queue_t *)malloc(sizeof(struct frame_queue_t));

  my_frame->f.buf = buf;
  my_frame->f.l = l;
  my_frame->fixup_seqno = fixup_seqno;
  my_frame->next = NULL;

  if(cur) {
    cur->next = my_frame;
    return cur;
  }

  return my_frame;
}

struct frame_queue_t *frame_enqueue_dupfrag(struct frame_queue_t *cur, char *buf, int offset, int l, int fixup_seqno) {
  char *my_buf;
  int my_len;

  my_len = l - offset;

  if(0 >= my_len) {
    my_len = 0;
    my_buf = NULL;
  } else {
    my_buf = (char *)malloc(my_len);
    memcpy(my_buf, buf+offset, my_len);
  }

  return frame_enqueue(cur, my_buf, my_len, fixup_seqno);
}

struct frame_queue_t *frame_enqueue_append(struct frame_queue_t *queue, struct frame_queue_t *tail) {

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

void of_init(struct oedifilter_state *s) {
  s->flow_state = PREHEADER;
  s->seqno = 0x1899;

  memset(s->header, 0x00, HEADER_LEN);

  s->body_len = 0;
  s->body_buf = NULL;
  s->body_pos = 0;

  s->to_inject = NULL;
  s->to_sanitize = NULL;
}

struct frame_queue_t *of_frame_inject(struct oedifilter_state *s, struct frame_queue_t *queue) {

  queue = frame_enqueue_append(queue, s->to_inject);

  /* Clear the internal inject queue */
  s->to_inject = NULL;

  /* Return the filled frame queue */
  return queue;
}

void of_load_header(struct oedifilter_state *s, char *orig_header, char chan, unsigned short seqno, unsigned short len) {
  memset(s->header, 0x00, HEADER_LEN);
  s->body_len = HEADER_LEN+len;
  s->body_buf = (char *)malloc(len+HEADER_LEN);
  s->body_pos = HEADER_LEN;

  memcpy(s->body_buf, orig_header, HEADER_LEN);
}



int of_attempt_header_parse(struct oedifilter_state *s, char *buf, int pos) {
  unsigned char chan;
  signed short seqno;
  unsigned short len;
 
  if(buf[pos] != 0x2A)
    return -1;

  chan = buf[pos+1];

  if(chan > 5)
    return -2;

  seqno = aimutil_get16(buf+pos+1+1);
  len = aimutil_get16(buf+pos+1+1+2);

  if(s)
    of_load_header(s, buf+pos, chan, seqno, len);
  
  return 1;
}

int of_scan_to_header(struct frame_t *f, int pos) {
  int i;

  for(i = pos; (i+HEADER_LEN) < f->l; i++) {
    if(*(f->buf+i) == 0x2A) {
      /* got magic, try to parse header */
      if(1 == of_attempt_header_parse(NULL, f->buf, i))
	return i;
    }
  }

  /* none found */
  return -1;
}

/* Note that queue_p here is a pointer to a pointer! */
int of_load_body(struct oedifilter_state *s, struct frame_queue_t **queue_p, char *buf, int len, int pos) {
  int buf_left = len - pos;
  int body_left = s->body_len - s->body_pos;
  
  int copy_chunk = buf_left > body_left ? body_left : buf_left;

  /* Copy all the data we can in */
  memcpy(s->body_buf+s->body_pos, buf+pos, copy_chunk);
  s->body_pos += copy_chunk;

  pos += copy_chunk;

  if(s->body_pos >= s->body_len) {
    /* parse body if full:
       - extract events
       - queue for xmit unless sanitized
       reset state to PREHEADER
    */

  }

  return pos;
}

/* never frees its input, and you need to free all frames it
   returns */
struct frame_queue_t * of_run(struct oedifilter_state *s, struct frame_t *f) {
  int pos = 0;
  struct frame_queue_t *retval = NULL;

  if(!s || !f)
    return NULL;

  while(pos < f->l) {
    if(PREHEADER == s->flow_state) {
      /* scan for the next OSCAR HEADER */
      int next_header = of_scan_to_header(f, pos);
      if(-1 == next_header) {
	/* No header found in this buffer, just queue it out */
        retval = frame_enqueue_dupfrag(retval, f->buf, pos, f->l-pos, NO_SEQNO_FIXUP);
	pos = f->l;
      } else {
	/* enqueue up to next_header, change state, and go */
        retval = frame_enqueue_dupfrag(retval, f->buf, pos, next_header-pos, NO_SEQNO_FIXUP);

	/* inject pending frames */
	retval = of_frame_inject(s, retval);
	
	/* update machine state */
	pos = next_header;
	s->flow_state = INHEADER;
      }
    }

    if(INHEADER == s->flow_state) {
      of_attempt_header_parse(s, f->buf, pos);
      pos += HEADER_LEN;
      s->flow_state = INBODY;
    }

    if(INBODY == s->flow_state) {
      /* fill up body buf */


      /* of_load_body updates the state and queue for us */
      pos = of_load_body(s, &retval, f->buf, f->l, pos);
    }
  }

  /* XXX TODO Fixup seqnos */
  return retval;
}
