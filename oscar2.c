#include "oscar2.h"

void tlv_print(struct am_tlv *tlv, int dump_value);


void tlv_print(struct am_tlv *tlv, int dump_value) {
  printf("TLV: %04x/%04x", tlv->type, tlv->length);
  if(dump_value && tlv->length) {
    int i;
    for(i = 0; i < tlv->length; i++)
      printf(" %02x", (unsigned char)tlv->value[i]);
  }
  printf("\n");
}

struct am_tlv_list *parse_tlvs(struct am_tlv_list *root, unsigned char *body, int len) {
  struct am_tlv_list *cur = root;
  struct am_tlv_list *newroot = NULL;

  int pos = 0;

  FUNC("parse_tlvs");

  if(!root)
    newroot = (struct am_tlv_list *)malloc(sizeof(struct am_tlv_list));

  while(pos < len) {
    unsigned short type = aimutil_get16(body+pos);
    unsigned short tlv_len = aimutil_get16(body+pos+2);

    unsigned char *data = body+pos+4;

    if(cur) {
      cur->next = (struct am_tlv_list *)malloc(sizeof(struct am_tlv_list));
      cur = cur->next;
    } else
      cur = newroot;

    cur->next = NULL;
    cur->tlv.type = type;
    cur->tlv.length = tlv_len;
    cur->tlv.value = data;

    pos += 2+2+tlv_len;
  }

  return (root ? root : newroot);
}

struct am_tlv_list *tlv_first_by_type(struct am_tlv_list *root, unsigned short type) {
  struct am_tlv_list *cur = root;

  FUNC("am_tlv_list");

  while(cur) {
    if(cur->tlv.type == type)
      return cur;
    cur = cur->next;
  }
  return NULL;
}

void tlv_list_free(struct am_tlv_list *root) {
  struct am_tlv_list *t;

  for(t = root; t; t = root) {
    root = t->next;
    /* values are typically left in-situ */
    free(t);
  }
}



void am_init(struct aim_machine *s) {
  memset(s, 0x00, sizeof(struct aim_machine));

  s->flow_state = DEAD_AIR;
  s->seqno = BASE_SEQNO;

  memset(s->header, 0x00, HEADER_LEN);

  s->body_len = 0;
  s->body_buf = NULL;
  s->body_pos = 0;

  s->to_inject = NULL;

  s->callbacks = NULL;

  s->partner = NULL;

  s->forbidden_cookies = NULL;
}

void am_finish(struct aim_machine *s) {
  struct frame_queue_t *f1, *f2;
  struct am_cb_entry_t *c1, *c2;

  if(s->body_buf)
    free(s->body_buf);

  s->body_buf = NULL;

  for(f1 = s->to_inject; f1; f1 = f2) {
    f2 = f1->next;
    free(f1->f.buf);
    free(f1);
  }
  s->to_inject = NULL;

  for(f1 = s->forbidden_cookies; f1; f1 = f2) {
    f2 = f1->next;
    free(f1->f.buf);
    free(f1);
  }
  s->forbidden_cookies = NULL;

  for(c1 = s->callbacks; c1; c1 = c2){
    c2 = c1->next;
    free(c1);
  }
  s->callbacks = NULL;
}

void am_init_pair(struct aim_machine *in, struct aim_machine *out) {
  am_init(in);
  am_init(out);

  in->partner = out;
  out->partner = in;

}

struct frame_queue_t *am_frame_inject(struct aim_machine *s, struct frame_queue_t *queue) {
  FUNC("am_frame_inject");

  queue = frame_enqueue_append(queue, s->to_inject);

  /* Clear the internal inject queue */
  s->to_inject = NULL;

  /* Return the filled frame queue */
  return queue;
}

void am_load_header(struct aim_machine *s, unsigned char *orig_header, unsigned char chan, unsigned short seqno, unsigned short len) {
  memset(s->header, 0x00, HEADER_LEN);
  s->body_len = HEADER_LEN+len;
  if(s->body_buf)
    free(s->body_buf);
  s->body_buf = (unsigned char *)malloc(len+HEADER_LEN);
  s->body_pos = HEADER_LEN;

  memcpy(s->body_buf, orig_header, HEADER_LEN);
}



int am_attempt_header_parse(struct aim_machine *s, unsigned char *buf, int pos) {
  unsigned char chan;
  unsigned short seqno;
  unsigned short len;
 
  FUNC("am_attempt_header_parse");

  if(buf[pos] != 0x2A)
    return -1;

  chan = buf[pos+1];

  if(chan > 5)
    return -2;

  seqno = aimutil_get16(buf+pos+1+1);
  len = aimutil_get16(buf+pos+1+1+2);

  if(s)
    am_load_header(s, buf+pos, chan, seqno, len);
  
  return 1;
}

int am_scan_to_header(struct frame_t *f, int pos) {
  int i;

  FUNC("am_scan_to_header");

  for(i = pos; (i+HEADER_LEN) <= f->l; i++) {
    if(*(f->buf+i) == 0x2A) {
      /* got magic, try to parse header */
      if(1 == am_attempt_header_parse(NULL, f->buf, i))
	return i;
    }
  }

  /* none found */
  return -1;
}


/* 

   OSCAR Outbound Messages (0x0004/0x0006) are somewhat ugly, so we
   just fudge their parsing here.  We can always do better later...
*/
unsigned char *am_extract_message(unsigned char *buf, int l) {
  unsigned short pos = 0;

  while(pos < l) {
    unsigned short len = aimutil_get16(buf+pos+2);
    if(buf[pos] == 0x01) { /* looking for message fragment */
      unsigned char *msg;
      int msglen = len-4;
      msg = (unsigned char *)malloc(msglen+1);
      memset(msg, 0, msglen+1);

      memcpy(msg, buf+pos+1+1+2+2+2, msglen);
      msg[msglen] = 0x00;

      return msg;
    }
    pos += 2+2+len;
  }
  return NULL;
}

void parse_msg_outbound(struct aim_machine *s, unsigned char *buf, int l, am_event_cb cb) {
  unsigned char cookie[8];
  unsigned short msg_channel;
  unsigned char snlen;
  unsigned char sn[255];

  struct am_tlv_list *tlvs;

  struct am_tlv_list *msgbody;
#if DEBUGMSG
  struct am_tlv_list *curtlv;
#endif

  unsigned char *message;

  int base_pos = HEADER_LEN+2+2+2+4;

  FUNC("parse_msg_outbound");

  memcpy(cookie, buf+base_pos, 8);
  msg_channel = aimutil_get16(buf+base_pos+8);

#if DEBUGMSG
  printf("MSG: msg_channel: %04x\n", msg_channel);
#endif

  /* Grab SN */
  snlen = (unsigned char)buf[base_pos+8+2];

#if DEBUGMSG
  printf("MSG: snlen: %02x\n", snlen);
#endif

  memset(sn, 0, 255);
  memcpy(sn, buf+base_pos+8+2+1, snlen);
#if DEBUGMSG
  printf("MSG: sn: %s\n", sn);
#endif

  tlvs = NULL;
  tlvs = parse_tlvs(tlvs, buf+base_pos+8+2+1+snlen, l-(base_pos+8+2+1+snlen));

#if DEBUGMSG
  for(curtlv = tlvs; curtlv; curtlv = curtlv->next)
    tlv_print(&(curtlv->tlv), 1);
#endif

  msgbody = tlv_first_by_type(tlvs, 0x0002);
    
  if(msg_channel == 1) {
    message = am_extract_message(msgbody->tlv.value, msgbody->tlv.length);
    cb(s, sn, message);
    free(message);
    tlv_list_free(tlvs);
    tlvs = NULL;
    /* sn is a static buffer */
  } else
    printf("DRAT!  Don't know how to parse channels other than 1: %d tx\n", msg_channel);
}

void parse_msg_inbound(struct aim_machine *s, unsigned char *buf, int l, am_event_cb cb) {
  unsigned char cookie[8];
  unsigned short msg_channel;
  unsigned char snlen;
  unsigned char sn[255];

  unsigned char warning;
  unsigned char n_tlvs;

  int i;

  int pos = HEADER_LEN+2+2+2+4;

  FUNC("parse_msg_outbound");

  memcpy(cookie, buf+pos, 8);
  pos += 8;
  msg_channel = aimutil_get16(buf+pos);
  pos += 2;

  printf("MSG: msg_channel: %04x\n", msg_channel);

  /* Grab SN */
  snlen = (unsigned char)buf[pos];
  pos++;

  printf("MSG: snlen: %02x\n", snlen);

  memset(sn, 0, 255);
  memcpy(sn, buf+pos, snlen);
  pos += snlen;
  printf("MSG: sn: %s\n", sn);

  warning = aimutil_get16(buf+pos);
  pos += 2;

  printf("MSG: warning: %04x\n", warning);

  n_tlvs = aimutil_get16(buf+pos);
  pos += 2;

  printf("MSG: n_tlvs: %04x\n", n_tlvs);

  for(i = 0; i < n_tlvs; i++) {
    unsigned short c_t, c_l;
    int j;

    c_t = aimutil_get16(buf+pos);
    pos += 2;
    c_l = aimutil_get16(buf+pos);
    pos += 2;

    printf("MSG: TLV FIRST: %04x/%04x: ", c_t, c_l);
    for(j = 0; j < c_l; j++)
      printf("%02x", (unsigned char)buf[pos+j]);
    printf("\n");

    if(c_t == 0x0002) {
      pos += c_l;
    } else { 
      pos += c_l;
    }
  }

  while(pos < l) {
    unsigned short c_t, c_l;
    int j;

    c_t = aimutil_get16(buf+pos);
    pos += 2;
    c_l = aimutil_get16(buf+pos);
    pos += 2;

    if(c_t == 0x0002) {
      int pos2;
      unsigned short d_t, d_l;

      for(pos2 = 0; pos2 < c_l; ) {
	d_t = aimutil_get16(buf+pos+pos2);
	pos2 += 2;
	d_l = aimutil_get16(buf+pos+pos2);
	pos2 += 2;

	printf("MSG: TLV MSG: %04x/%04x: ", d_t, d_l);
	if(d_t == 0x0101) {
	  unsigned short a, b;
	  unsigned char msg[4092];

	  memset(msg, 0, 4092);

	  a = aimutil_get16(buf+pos+pos2);
	  pos2 += 2;
	  b = aimutil_get16(buf+pos+pos2);
	  pos2 += 2;

	  memcpy(msg, buf+pos+pos2, d_l-4);

	  printf(" BODY %04x/%04x: %s", a, b, msg);
	  pos2 += d_l-4;

	} else {
	  int k;
	  for(k = 0; k < d_l; k++) {
	    printf("%02x", (unsigned char)buf[pos+pos2+k]);
	  }	
	  pos2 += d_l;
	}
	printf("\n");
      }
    }

    printf("MSG: TLV FREE: %04x/%04x: ", c_t, c_l);
    for(j = 0; j < c_l; j++)
      printf("%02x", (unsigned char)buf[pos+j]);
    printf("\n");

    pos += c_l;
  }

  if(cb)
    cb(s, buf, l);

}

void parse_presence(struct aim_machine *s, unsigned char *buf, int l, am_event_cb cb) {
  int pos = HEADER_LEN+2+2+2+4;
  int i;

  int state_aim = -1;
  int state_icq = -1;
  int idletime = -1;

  int oncoming = -1;

  /* family 0x0003 */
  if(0x0003 == aimutil_get16(buf+HEADER_LEN)) {
    unsigned short subtype = aimutil_get16(buf+HEADER_LEN+2);
    if(0x000b == subtype) {
      oncoming = 1;
    } else if(0x000c == subtype) {
      oncoming = 0;
    } 
  }

  while(pos < l) {
    int namelen;

    unsigned char name[255];
    unsigned short warning;
    unsigned short n_tlv;

    namelen = buf[pos];
    pos++;
    memset(&name, 0, 255);
    memcpy(&name, buf+pos, namelen);
    pos += namelen;
      
    warning = aimutil_get16(buf+pos);
    pos += 2;
    n_tlv = aimutil_get16(buf+pos);
    pos += 2;

    for(i = 0; i < n_tlv; i ++) {
      unsigned short tlv_t = aimutil_get16(buf+pos);
      pos += 2;
      unsigned short tlv_l = aimutil_get16(buf+pos);
      pos += 2;

      if(tlv_t == 0x0001) { /* user flags */
	state_aim = aimutil_get16(buf+pos);
      } else if(tlv_t == 0x0004) { /* idle time */
	idletime = aimutil_get16(buf+pos);
      } else if(tlv_t == 0x0006) { /* ICQ state */
	state_icq = aimutil_get16(buf+pos);
      }

      pos += tlv_l;
    }

    if(-1 == oncoming) {
      /* Not really sure what the state is, unfortunately */
    } else if(1 == oncoming) {
      if(-1 == state_icq) {
	if(state_aim & 0x0020)
	  state_icq = AIM_PRES_AWAY;
	else
	  state_icq = AIM_PRES_ONLINE;
      }
    }

    if(namelen)
      cb(s, name, oncoming, state_icq, idletime);

    pos += 1 + namelen + 2 + 2 + i;
  }
}

int am_cur_body_sanitized(struct aim_machine *s, unsigned char *buf) {
  struct frame_queue_t *cur_fc;

  FUNC("am_cur_body_sanitized");

  if(!buf)
    return 0;

  /* If we don't have any forbidden cookies, pass everything */
  if(!s->forbidden_cookies)
    return 0;

  /* We only care about channel 2 */
  if(buf[1] != 0x02)
    return 0;

  /* too short to contain snac... */
  if(aimutil_get16(buf+4) < 18)
    return 0;

  for(cur_fc = s->forbidden_cookies; cur_fc; cur_fc = cur_fc->next)
    if(0 == memcmp(buf+HEADER_LEN+2+2+2+4, cur_fc->f.buf, 8))
       return 1;
  

  /* XXX TODO Implement me... */
  return 0;
}


am_event_cb am_cb_find(struct am_cb_entry_t *root, unsigned short family, unsigned short subtype) {
  struct am_cb_entry_t *cur;
  FUNC("am_cb_find");

  cur = root;

  while(cur) {
    if(family == cur->family && subtype == cur->subtype)
      return cur->f;
    cur = cur->next;
  }

  return NULL;
}

void am_parse_body(struct aim_machine *s, unsigned char *buf, int l) {
  unsigned short family;
  unsigned short subtype;

  am_event_cb callback;


  FUNC("am_parse_body");
    
  if(!s)
    return;

  /* Check the channel first */
  if(buf[1] != 0x02)
    return;

  family  = aimutil_get16(buf+HEADER_LEN);
  subtype = aimutil_get16(buf+HEADER_LEN+2);

  if(s->nukebyte && family == 0x0004 && subtype == 0x0007) {
    buf[s->nukebyte] = 0xff;
    printf("***** NUKED %d\n", s->nukebyte);
    s->nukebyte = 0x00;
  }
  fflush(stdout);

  callback = am_cb_find(s->callbacks, family, subtype);
  if(!callback)
    return;

  switch(family) {
  case 0x0003: {
    switch(subtype) {
    case 0x000b: /* fallthrough */
    case 0x000c:
      parse_presence(s, buf,l, callback);
      return;
    }
  }
    
  case 0x0004:
    switch(subtype) {
    case 0x0006:
      parse_msg_outbound(s, buf, l, callback);
      return;
      break;
    case 0x0007:
      callback(s, buf, l);
      return;
      break;
    }
    break;
  }
  callback(s);
}


/* Note that queue_p here is a pointer to a pointer! */
int am_load_body(struct aim_machine *s, struct frame_queue_t **queue_p, unsigned char *buf, int len, int pos) {
  int buf_left = len - pos;
  int body_left = s->body_len - s->body_pos;
  
  int copy_chunk = buf_left > body_left ? body_left : buf_left;

  FUNC("am_load_body");

  /* Copy all the data we can in */
  memcpy(s->body_buf+s->body_pos, buf+pos, copy_chunk);
  s->body_pos += copy_chunk;

  pos += copy_chunk;

  if(s->body_pos >= s->body_len) {
    am_parse_body(s, s->body_buf, s->body_len);

    if(!am_cur_body_sanitized(s,s->body_buf))
      *queue_p = frame_enqueue(*queue_p, s->body_buf, s->body_len, SEQNO_FIXUP_YES);
    else
      free(s->body_buf); /* If sanitized, the consumer can't free... */


    s->body_buf = NULL;
    s->body_len = 0;
    s->body_pos = 0;

    s->flow_state = DEAD_AIR;
  }

  return pos;
}

int am_inject_frame(struct aim_machine *s, struct frame_t *f, int fixup_seqno) {
  FUNC("am_inject_frame");

  s->to_inject = frame_enqueue(s->to_inject, f->buf, f->l, fixup_seqno);

  /* we can transmit immediately */
  if(DEAD_AIR == s->flow_state)
    return 1;

  return 0;
}

void am_apply_seqno(struct aim_machine *s, unsigned char *buf) {

  unsigned char exceptions[][HEADER_LEN] = { 
    { 0x2a, 0x01, 0x87, 0x68, 0x01, 0x00 },
  };

  int n_exceptions = 1;

  int i;

  FUNC("am_inject_frame");

  for(i = 0; i < n_exceptions; i++) {
    if(0 == memcmp(buf, exceptions[i], HEADER_LEN))
      return;
  }

  aimutil_put16(buf+2, s->seqno);
}

void am_fixup_seqnos(struct aim_machine *s, struct frame_queue_t *queue) {
  struct frame_queue_t *cur;

  FUNC("am_fixup_seqnos");

  if(!queue)
    return;

  for(cur = queue; cur; cur = cur->next) {
    if(cur->fixup_seqno) {
      am_apply_seqno(s, cur->f.buf);
      s->seqno++;
    }
  }
}

struct am_cb_entry_t *am_cb_insert(struct am_cb_entry_t *root, unsigned short family, unsigned short subtype, am_event_cb cb) {
  struct am_cb_entry_t *retval;

  FUNC("am_cb_insert");

  retval = (struct am_cb_entry_t *)malloc(sizeof(struct am_cb_entry_t));
  retval->family = family;
  retval->subtype = subtype;
  retval->f = cb;
  retval->next = root;

  return retval;
}

int am_register_event_cb(struct aim_machine *s, unsigned short family, unsigned short subtype, am_event_cb cb) {

  s->callbacks = am_cb_insert(s->callbacks, family, subtype, cb);

  return 0;
}

struct frame_t *am_format_oft_sendfile_req(const unsigned char *sn, const unsigned char *snac, const unsigned char *cookie, const unsigned char *ip, const unsigned short port, const unsigned char *filename,  int filesize ) {
  unsigned char buf[1024];
  int i = 0;

  unsigned char sn_len = strlen(sn);
  unsigned short filename_len = strlen(filename);

  struct frame_t *retval;

  int pos_tlv5len = 0;

  unsigned char capstr[16] =  {0x09, 0x46, 0x13, 0x43, 0x4c, 0x7f, 0x11, 0xd1,
			       0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

  unsigned char local_ip[4] = { 0, 0, 0, 0 };

  for(i = 0; i < 4; i++)
    local_ip[i] = 255 - ip[i]; /* What. The. Hell. */

  retval = (struct frame_t *)malloc(sizeof(struct frame_t));

  i = 0;

  i += aimutil_put8(buf+i, 0x2a); /* magic */
  i += aimutil_put8(buf+i, 0x02); /* channel 2 */
  i += aimutil_put16(buf+i, 0x1111); /* seqno, to be overwritten by _run() */
  i += aimutil_put16(buf+i, 0x7777); /* len, to be overwritten */
  
  i += aimutil_put16(buf+i, 0x0004); /* family 0x0004 */
  i += aimutil_put16(buf+i, 0x0006); /* subtype 0x0006 */
  i += aimutil_put16(buf+i, 0x0000); /* no snac flags */

  memcpy(buf+i, snac, 4); /* add snac id */
  i += 4;

  memcpy(buf+i, cookie, 8); /* cookie */
  i += 8;

  i += aimutil_put16(buf+i, 0x0002); /* channel 2 */

  i += aimutil_put8(buf+i, sn_len); /* len of SN */
  memcpy(buf+i, sn, sn_len); /* copy in SN */
  i += sn_len;

  i += aimutil_put16(buf+i, 0x0005); /* TLV 5: wrapper */
  pos_tlv5len = i; /* note position for later.. */
  i += aimutil_put16(buf+i, 0x3333); /* len, to be overwritten */


  i += aimutil_put16(buf+i, 0x0000); /* rendezvous propose */
  memcpy(buf+i, cookie, 8); /* copy of cookie for rendezvous */
  i += 8;
  memcpy(buf+i, capstr, 16); /* sendfile cap: what rendezvous we're proposing */
  i += 16;
  
  /* request number: 0x000a/0x0002 */
  i += aimutil_put16(buf+i, 0x000a);
  i += aimutil_put16(buf+i, 0x0002);
  i += aimutil_put16(buf+i, 0x0001);
  
  /* unknown but needed: 0x000f/0x0000 */
  i += aimutil_put16(buf+i, 0x000f);
  i += aimutil_put16(buf+i, 0x0000);

  /* IP address: 0x0002/0x0004 */
  i += aimutil_put16(buf+i, 0x0002);
  i += aimutil_put16(buf+i, 0x0004);
  memcpy(buf+i, ip, 4);
  i += 4;

  /* IP address(again): 0x0003/0x0004 */
  i += aimutil_put16(buf+i, 0x0003);
  i += aimutil_put16(buf+i, 0x0004);
  memcpy(buf+i, ip, 4);
  i += 4;

  /* Proxy IP 0x0016/0x0004 */
  i += aimutil_put16(buf+i, 0x0016);
  i += aimutil_put16(buf+i, 0x0004);
  memcpy(buf+i, local_ip, 4);
  i += 4;

  /* Proxy Port: 0x0017/0x0002 */
  i += aimutil_put16(buf+i, 0x0017);
  i += aimutil_put16(buf+i, 0x0002);
  i += aimutil_put16(buf+i, port);

  /* Port: 0x0005/0x0002 */
  i += aimutil_put16(buf+i, 0x0005);
  i += aimutil_put16(buf+i, 0x0002);
  i += aimutil_put16(buf+i, port);
  
  /* File Info: 0x2711/2+2+4+strlen(filename)+1 */
  i += aimutil_put16(buf+i, 0x2711);
  i += aimutil_put16(buf+i, 2+2+4+filename_len+1);
  i += aimutil_put16(buf+i, 0x0001); /* # files: 1 single file, 2 multiple files */
  i += aimutil_put16(buf+i, 0x0001); /* number of files: always 1 for us */
  i += aimutil_put32(buf+i, filesize); /* file size */
  memcpy(buf+i, filename, filename_len);
  i += strlen(filename);
  i += aimutil_put8(buf+i, 0x00); /* null terminator on string */

  /* end of packet.  fix up lengths. */
  aimutil_put16(buf+4, i-HEADER_LEN); /* frame length */
  aimutil_put16(buf+pos_tlv5len, i-(pos_tlv5len+2)); /* len of TLV 0x0005/len */

  retval->l = i;
  retval->buf = (unsigned char *)malloc(retval->l);
  memcpy(retval->buf, buf, i);

  return retval;
}

void am_add_forbidden_cookie(struct aim_machine *s, unsigned char *cookie) {
  s->forbidden_cookies = frame_enqueue_dupfrag(s->forbidden_cookies, cookie, 0, 8, NO_SEQNO_FIXUP);
}

void mk_cookie(unsigned char *cookie) {
  int i;
  for(i = 0; i < 8; i++)
    cookie[i] = 0x30 + (rand()%10);
}


int am_inject_sendfile(struct aim_machine *s, char *sn, char *ip, short port, char *filename, int filesize) {
  char snac[4];
  char cookie[8];
  
  struct frame_t *frame;

  mk_cookie(cookie);
  memcpy(cookie+2, snac, 4);
  
  frame = am_format_oft_sendfile_req(sn, snac, cookie, ip, port, filename, filesize);

  am_add_forbidden_cookie(s, cookie);
  am_add_forbidden_cookie(s->partner, cookie);

  return am_inject_frame(s, frame, SEQNO_FIXUP_YES);
}

struct frame_t *am_format_message_outbound(unsigned char *snac, unsigned char *cookie, unsigned char *sn, unsigned char *message) {
  struct frame_t *retval;

  char *outbuf, buf[8192];
  int pos, pos_tlv5len, n_tlvs, pos_n_tlvs;

  int sn_len;

  sn_len = strlen(sn);

  pos = 0;

  pos += aimutil_put8(buf+pos, 0x2a);
  pos += aimutil_put8(buf+pos, 0x02);
  pos += aimutil_put16(buf+pos, 0x1111);
  pos += aimutil_put16(buf+pos, 0x7777);
  
  pos += aimutil_put16(buf+pos, 0x0004); /* family 0x0004 */
  pos += aimutil_put16(buf+pos, 0x0006); /* subtype 0x0006 */
  pos += aimutil_put16(buf+pos, 0x0000); /* no snac flags */

  memcpy(buf+pos, snac, 4);
  pos += 4;

  memcpy(buf+pos, cookie, 8);
  pos += 8;


  pos += aimutil_put16(buf+pos, 0x0001); /* channel 1 */

  pos += aimutil_put8(buf+pos, sn_len); /* len of SN */
  memcpy(buf+pos, sn, sn_len); /* copy in SN */
  pos += sn_len;


  pos += aimutil_put16(buf+pos, 0x0002);
  pos += aimutil_put16(buf+pos, 0x0d+strlen(message));

  pos += aimutil_put16(buf+pos, 0x0501);
  pos += aimutil_put16(buf+pos, 0x0001);
  pos += aimutil_put16(buf+pos, 0x0101);
  pos += aimutil_put8(buf+pos, 0x01);

  pos += aimutil_put16(buf+pos, 4+strlen(message));
  pos += aimutil_put16(buf+pos, 0x0000);
  pos += aimutil_put16(buf+pos, 0x0000);

  memcpy(buf+pos, message, strlen(message));
  pos += strlen(message);


  /* fixup body length */
  aimutil_put16(buf+4, pos-HEADER_LEN);

  /* Copy output */
  outbuf = (char *)malloc(pos);
  memcpy(outbuf, buf, pos);
  retval = (struct frame_t *)malloc(sizeof(struct frame_t));
  retval->buf = outbuf;
  retval->l = pos;

  return retval;
}


int am_inject_message_outbound(struct aim_machine *s, char *sn, char *message) {
  char snac[4];
  char cookie[8];
  
  struct frame_t *frame;

  mk_cookie(cookie);
  memcpy(cookie+2, snac, 4);
  
  frame = am_format_message_outbound(snac, cookie, sn, message);

  am_add_forbidden_cookie(s, cookie);
  am_add_forbidden_cookie(s->partner, cookie);

  return am_inject_frame(s, frame, SEQNO_FIXUP_YES);
  
}


/* this really is the worst possible way to do this, but it works */
struct frame_t *am_toy_format_message_orig(unsigned char *cookie, unsigned char *sn, unsigned char *msg) {

  unsigned char header[] = {
    0x2a, 0x02, 0x11, 0x11, 0x88, 0x88, 
    0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 's', 'n', 'a', 'c',
    'm', 'y', 'C', 'o', 'o', 'k', 'i', 'e',
    0x00, 0x01, 
  };


/* screenname */
/*
  unsigned char sn_dummy[] = {
    0x0b, 
    's', 'c', 'r', 'e', 'e', 'n','n','a','m','e','X',
  };
*/

    /* post-screename */

  unsigned char post_sn[] = {
    0x00, 0x00,

    0x00, 0x08, 

    0x00, 0x01, 0x00, 0x02, 
    0x00, 0x31,


    0x00, 0x05, 0x00, 0x04,
    0x47, 0x34, 0x05, 0x72,

    0x00, 0x1d, 0x00, 0x12,
    0x00, 0x01, 0x00, 0x05, 0x02, 0x01, 0xd2, 0x04,
    0x72, 0x00, 0x81, 0x00, 0x05, 0x2b, 0x00, 0x00,
    0x14, 0x4f,

    0x00, 0x27, 0x00, 0x04,
    0x47, 0x35, 0x4e, 0xb1,

    0x00, 0x04, 0x00, 0x02,
    0x05, 0xab,            

    0x00, 0x0f, 0x00, 0x04,
    0x00, 0x01, 0x54, 0x84,

    0x00, 0x03, 0x00, 0x04,
    0x47, 0x35, 0x4d, 0x48,

    0x00, 0x29, 0x00, 0x04, 
    0x47, 0x35, 0x4e, 0xb1,

    0x00, 0x04, 0x00, 0x00,

    0x00, 0x02,
    0x00, 0x26,
    0x05,
    0x01,
    0x00, 0x04,
    0x01, 0x01, 0x01, 0x02,
    0x01, 
    0x01, 
  };

    /* message len */
  /*
    unsigned char msg_dummy_len[] = {
    0x00, 0x1a,
  };
*/

  unsigned char msg_format[] = {
    0x00, 0x00,
    0x00, 0x00,
  };

  /*
  unsigned char msg_dummy[] = {
    0x49, 0x27, 0x6d, 0x20, 0x6e, 0x6f, 0x74, 0x20,
    0x68, 0x65, 0x72, 0x65, 0x20, 0x72, 0x69, 0x67,
    0x68, 0x74, 0x20, 0x6e, 0x6f, 0x77,
  };
  */

  unsigned char post_msg[] = {
    /* post-message */

    0x00, 0x0b, 0x00, 0x00,

    0x00, 0x16, 0x00, 0x04, 
    0x47, 0x36, 0xa1, 0xcc, 

    0x00, 0x13, 0x00, 0x01,
    0x0f
  };


  unsigned char *buf;
  int len, len_sn, len_msg;
  int pos = 0;

  struct frame_t *retval;

  FUNC("am_toy_format_message");

  len_sn = strlen(sn);
  len_msg = strlen(msg);

  len = sizeof(header) + 1 + len_sn + sizeof(post_sn) + 2 + sizeof(msg_format) + len_msg + sizeof(post_msg);
  buf = (unsigned char *)malloc(len);
  
  pos = 0;
  memcpy(buf+pos, header, sizeof(header));
  aimutil_put16((buf+1+1+2), (len-HEADER_LEN));
  memcpy(buf+HEADER_LEN + 2+2+2, cookie+2, 4); /* snac */
  memcpy(buf+HEADER_LEN + 2+2+2+4, cookie, 8);
  pos += sizeof(header);

  buf[pos] = (unsigned char)(len_sn & 0xff);
  pos++;
  memcpy(buf+pos, sn, len_sn);
  pos += len_sn;

  memcpy(buf+pos, post_sn, sizeof(post_sn));
  aimutil_put16(buf+pos+86, len_msg+0x26-0x1a);
  pos += sizeof(post_sn);

  aimutil_put16(buf+pos-2, ((len_msg+sizeof(msg_format)) & 0xffff));
  pos += 2;
  memcpy(buf+pos, msg_format, sizeof(msg_format));
  pos += sizeof(msg_format);
  memcpy(buf+pos, msg, len_msg);
  pos += len_msg;

  memcpy(buf+pos, post_msg, sizeof(post_msg));
  pos += sizeof(post_msg);

  if(pos != len) {
    printf("!!!!!!!!!!!!!!pos=%d != len=%d\n", pos, len);
  }

  retval = (struct frame_t *)malloc(sizeof(struct frame_t));

  retval->buf = buf;
  retval->l = len;

  return retval;
}


void aimutil_put_tlv(unsigned char *buf, unsigned short t, unsigned short l, unsigned char *v) {
  aimutil_put16(buf, t);
  aimutil_put16(buf+2, l);
  memcpy(buf+4, v, l);
}


int format_message_02(unsigned char *buf, int pos, unsigned char *msg, unsigned short msglen) {
  unsigned char buf0501[] = { 0x01, 0x01, 0x01, 0x02 };
  aimutil_put_tlv(buf+pos, 0x0501, 0x0004, buf0501);
  pos += 4+4;

  aimutil_put16(buf+pos, 0x0101);
  pos += 2;
  aimutil_put16(buf+pos, msglen + 2+2);
  pos += 2;
  aimutil_put16(buf+pos, 0x0000);
  pos += 2;
  aimutil_put16(buf+pos, 0x0000);
  pos += 2;
  memcpy(buf+pos, msg, msglen);
  pos += msglen;
  
  return pos;
}

/* this really is the worst possible way to do this, but it works */
struct frame_t *am_format_message_inbound(unsigned char *snac, unsigned char *cookie, unsigned char *sn, unsigned char *msg) {
  unsigned char tmp[8192];
  int pos;

  struct frame_t *retval;

  int sublen1;

  unsigned char len_sn;
  unsigned short len_msg;

  unsigned short user_class = 0x0410;

  unsigned short n_tlvs = 0;
  unsigned short tlv_cursor = 0;

  unsigned char v_0x001d[] = {
    0x00, 0x01, 0x00, 0x05, 0x02, 0x01, 0xd2, 0x04,
    0x72, 0x00, 0x81, 0x00, 0x05, 0x2b, 0x00, 0x00,
    0x14, 0x4f,
  };


  len_sn = strlen(sn);
  len_msg = strlen(msg);

  pos = 0;
  tmp[pos] = 0x2a;
  pos++;
  tmp[pos] = 0x02;
  pos++;
  
  pos += aimutil_put16(tmp+pos, 0x1111); /* seqno filled in for us */
  pos += aimutil_put16(tmp+pos, 0x7777); /* len filled in later */

  pos += aimutil_put16(tmp+pos, 0x0004);
  pos += aimutil_put16(tmp+pos, 0x0007);
  pos += aimutil_put16(tmp+pos, 0x0000);

  memcpy(tmp+pos, snac, 4);
  pos += 4;

  memcpy(tmp+pos, cookie, 8);
  pos += 8;

  pos += aimutil_put16(tmp+pos, 0x0001); /* channel 1 */

  tmp[pos] = len_sn;
  pos++;
  memcpy(tmp+pos, sn, len_sn);
  pos += len_sn;

  pos += aimutil_put16(tmp+pos, 0x0000);

  /* n_tlvs */
  tlv_cursor = pos;
  pos += aimutil_put16(tmp+pos, 0x0007);

  pos += aimutil_put16(tmp+pos, 0x0001);
  pos += aimutil_put16(tmp+pos, 0x0002);
  pos += aimutil_put16(tmp+pos, user_class);
  n_tlvs++;
  pos += aimutil_put16(tmp+pos, 0x0002);
  pos += aimutil_put16(tmp+pos, 16+len_msg);
  
  pos += aimutil_put16(tmp+pos, 0x0501);
  pos += aimutil_put16(tmp+pos, 0x0004);
  pos += aimutil_put32(tmp+pos, 0x01010102);

  pos += aimutil_put16(tmp+pos, 0x0101);
  pos += aimutil_put16(tmp+pos, 0x04+len_msg);
  pos += aimutil_put16(tmp+pos, 0x0000);
  pos += aimutil_put16(tmp+pos, 0x0000);
  memcpy(tmp+pos, msg, len_msg);
  pos += len_msg;

  aimutil_put16(tmp+tlv_cursor, n_tlvs);
  /* fixup, so no need to update pos */

  /* 0x000b/0x0000: typing notification */

  pos += aimutil_put16(tmp+pos, 0x000b);
  pos += aimutil_put16(tmp+pos, 0x0000);



  /* fixup body length */
  aimutil_put16(tmp+4, pos-HEADER_LEN);

  retval = (struct frame_t *)malloc(sizeof(struct frame_t));

  retval->buf = (unsigned char *)malloc(pos);
  memcpy(retval->buf, tmp, pos);
  retval->l = pos;

  return retval;
}


int am_inject_message_inbound(struct aim_machine *s, char *sn, char *message) {
  char snac[4];
  char cookie[8];
  
  struct frame_t *frame;

  int i;

  mk_cookie(cookie);
  memcpy(cookie+2, snac, 4);
  
  frame = am_format_message_inbound(snac, cookie, sn, message);

  am_add_forbidden_cookie(s, cookie);
  am_add_forbidden_cookie(s->partner, cookie);

  i = am_inject_frame(s, frame, SEQNO_FIXUP_YES);

  return 0;
}


/* never frees its input, and you need to free all frames it
   returns */
struct frame_queue_t * am_run(struct aim_machine *s, struct frame_t *f) {
  int pos = 0;
  struct frame_queue_t *retval = NULL;

  FUNC("am_run");

  if(!s || !f) /* XXX TODO add errno/errmsg here */
    return NULL;

  while(pos < f->l) {
    if(!(s->flow_state == DEAD_AIR || s->flow_state == PREHEADER || s->flow_state == INHEADER || s->flow_state == INBODY)) {
      /* XXX TODO add errno/errmsg here */
      return NULL;
    }

    if(DEAD_AIR == s->flow_state) {
      /* inject pending frames */
      retval = am_frame_inject(s, retval);

      if((f->l-pos) >= HEADER_LEN) {
	/* attempt to get header */
	if(0 == am_attempt_header_parse(NULL, f->buf, pos))
	  s->flow_state = INHEADER;
	else
	  s->flow_state = PREHEADER;
      }
    }

    if(PREHEADER == s->flow_state) {
      /* scan for the next OSCAR HEADER */
      int next_header = am_scan_to_header(f, pos);
      if(-1 == next_header) {
	/* No header found in this buffer, just queue it out */
        retval = frame_enqueue_dupfrag(retval, f->buf, pos, f->l, NO_SEQNO_FIXUP);
	pos = f->l;
      } else {
	if(next_header > pos) {
	  /* enqueue up to next_header */
	  retval = frame_enqueue_dupfrag(retval, f->buf, pos, next_header-pos, NO_SEQNO_FIXUP);
	}

	/* inject pending frames */
	retval = am_frame_inject(s, retval);

	/* update machine state */
	pos = next_header;
	s->flow_state = INHEADER;
      }
    }

    if(INHEADER == s->flow_state) {
      am_attempt_header_parse(s, f->buf, pos);
      pos += HEADER_LEN;
      s->flow_state = INBODY;
    }

    if(INBODY == s->flow_state) {
      /* fill up body buf */

      /* am_load_body updates the state and queue for us */
      pos = am_load_body(s, &retval, f->buf, f->l, pos);
    }
  }

  am_fixup_seqnos(s, retval);
  return retval;
}

