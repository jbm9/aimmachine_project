/* A lot of the packet-mangling here is from libfaim.  Happily, I
   wrote that code in the first place, so I'm allowed to do that.
   Also, it's fairly trivial, even if it weren't the case.*/

#include "oft.h"
#ifdef __cplusplus
extern "C" {
#endif

  int cmp_fileheader(struct aim_fileheader_t *a, struct aim_fileheader_t *b) {
    if(0 != memcmp(a->bcookie, b->bcookie, 8))
      return -1;

    /*
      Ignored...
      if(0 != strcmp(a->dummy, b->dummy))
      return -1;
    */

    if(a->flags != b->flags)
      return -1;

    if(0 != memcmp(a->idstring, b->idstring, 32))
      return -1;

    if(a->lnameoffset != b->lnameoffset)
      return -1;

    if(a->lsizeoffset != b->lsizeoffset)
      return -1;

    if(0 != memcmp(a->macfileinfo, b->macfileinfo, 16))
      return -1;

    if(0 != memcmp(a->magic, b->magic, 4))
      return -1;

    if(0 != strcmp(a->name, b->name))
      return -1;

    if(a->checksum != b->checksum)
      return -1;

    if(a->cretime != b->cretime)
      return -1;

    if(a->modtime != b->modtime)
      return -1;

    if(a->nrecvd != b->nrecvd)
      return -1;

    if(a->recvcsum != b->recvcsum)
      return -1;

    if(a->rfcsum != b->rfcsum)
      return -1;

    if(a->rfrcsum != b->rfrcsum)
      return -1;

    if(a->rfsize != b->rfsize)
      return -1;

    if(a->size != b->size)
      return -1;

    if(a->totsize != b->totsize)
      return -1;

    if(a->compress != b->compress)
      return -1;

    if(a->encrypt != b->encrypt)
      return -1;

    if(a->filesleft != b->filesleft)
      return -1;

    if(a->hdrlen != b->hdrlen)
      return -1;

    if(a->hdrtype != b->hdrtype)
      return -1;

    if(a->nencode != b->nencode)
      return -1;

    if(a->nlanguage != b->nlanguage)
      return -1;

    if(a->partsleft != b->partsleft)
      return -1;

    if(a->totfiles != b->totfiles)
      return -1;

    if(a->totparts != b->totparts)
      return -1;

    return 0;
  }

  struct aim_fileheader_t *oft_fill_fileheader(struct aim_fileheader_t *fh, char * cookie, const char *filename, int size) {
    int filename_len;

    filename_len = strlen(filename);
    
    memset(fh, 0, sizeof(struct aim_fileheader_t));

    memcpy(fh->magic, "OFT2", 4);
    fh->hdrlen = 256;

    fh->hdrtype = 0x0101;
    
    /* the default packet is zero-padded to a filename length of 64.
       If it's longer, we need to note that */

    if(filename_len > 64) {
      /* don't overshoot our buffer... */
      if(filename_len > 256)
	filename_len = 256;

      fh->hdrlen += (filename_len - 64);
    }

    memcpy(fh->bcookie, cookie, 8);

    fh->totfiles = 1;
    fh->filesleft = 1;
    fh->totparts = 1;
    fh->partsleft = 1;

    fh->totsize = size;
    fh->size = size;

    strncpy((char *)fh->idstring, "Cool FileXfer", 32);
    memcpy(fh->name, filename, filename_len);

    return fh;
  }


  /*
   * aim_oft_getfh - extracts a aim_fileheader_t from buffer hdr.
   * @hdr: buffer to extract header from  
   *
   * returns pointer to struct on success; %NULL on error.  
   *
   */
  struct aim_fileheader_t *aim_oft_getfh(struct aim_fileheader_t *fh, unsigned char *hdr) 
  {
    int i, j;

    i = 0;

    memcpy(fh->magic, hdr, 4);
    i += 4;

    fh->hdrlen = aimutil_get16(hdr+i);
    i += 2;

    fh->hdrtype = aimutil_get16(hdr+i);
    i+= 2;

    for(j = 0; j < 8; j++, i++)
      fh->bcookie[j] = hdr[i];
    fh->encrypt = aimutil_get16(hdr+i);
    i += 2;
    fh->compress = aimutil_get16(hdr+i);
    i += 2;
    fh->totfiles = aimutil_get16(hdr+i);
    i += 2;
    fh->filesleft = aimutil_get16(hdr+i);
    i += 2;
    fh->totparts = aimutil_get16(hdr+i);
    i += 2;
    fh->partsleft = aimutil_get16(hdr+i);
    i += 2;
    fh->totsize = aimutil_get32(hdr+i);
    i += 4;
    fh->size = aimutil_get32(hdr+i);
    i += 4;
    fh->modtime = aimutil_get32(hdr+i);
    i += 4;
    fh->checksum = aimutil_get32(hdr+i);
    i += 4;
    fh->rfrcsum = aimutil_get32(hdr+i);
    i += 4;
    fh->rfsize = aimutil_get32(hdr+i);
    i += 4;
    fh->cretime = aimutil_get32(hdr+i);
    i += 4;
    fh->rfcsum = aimutil_get32(hdr+i);
    i += 4;
    fh->nrecvd = aimutil_get32(hdr+i);
    i += 4;
    fh->recvcsum = aimutil_get32(hdr+i);
    i += 4;
    memcpy(fh->idstring, hdr+i, 32);
    i += 32;
    fh->flags = aimutil_get8(hdr+i);
    i += 1;
    fh->lnameoffset = aimutil_get8(hdr+i);
    i += 1;
    fh->lsizeoffset = aimutil_get8(hdr+i);
    i += 1;
    memcpy(fh->dummy, hdr+i, 69);
    i += 69;
    memcpy(fh->macfileinfo, hdr+i, 16);
    i += 16;
    fh->nencode = aimutil_get16(hdr+i);
    i += 2;
    fh->nlanguage = aimutil_get16(hdr+i);
    i += 2;
    memcpy(fh->name, hdr+i, fh->hdrlen-i);
    i += fh->hdrlen-i;
    return fh;
  }

  int aim_oft_buildheader(unsigned char *dest, struct aim_fileheader_t *fh) 
  { 
    int i, curbyte;
    if(!dest || !fh)
      return -1;
    curbyte = 0;

    memcpy(dest+curbyte, fh->magic, 4);
    curbyte += 4;

    curbyte += aimutil_put16(dest+curbyte, fh->hdrlen);
    curbyte += aimutil_put16(dest+curbyte, fh->hdrtype);

    for(i = 0; i < 8; i++) 
      curbyte += aimutil_put8(dest+curbyte, fh->bcookie[i]);
    curbyte += aimutil_put16(dest+curbyte, fh->encrypt);
    curbyte += aimutil_put16(dest+curbyte, fh->compress);
    curbyte += aimutil_put16(dest+curbyte, fh->totfiles);
    curbyte += aimutil_put16(dest+curbyte, fh->filesleft);
    curbyte += aimutil_put16(dest+curbyte, fh->totparts);
    curbyte += aimutil_put16(dest+curbyte, fh->partsleft);
    curbyte += aimutil_put32(dest+curbyte, fh->totsize);
    curbyte += aimutil_put32(dest+curbyte, fh->size);
    curbyte += aimutil_put32(dest+curbyte, fh->modtime);
    curbyte += aimutil_put32(dest+curbyte, fh->checksum);
    curbyte += aimutil_put32(dest+curbyte, fh->rfrcsum);
    curbyte += aimutil_put32(dest+curbyte, fh->rfsize);
    curbyte += aimutil_put32(dest+curbyte, fh->cretime);
    curbyte += aimutil_put32(dest+curbyte, fh->rfcsum);
    curbyte += aimutil_put32(dest+curbyte, fh->nrecvd);
    curbyte += aimutil_put32(dest+curbyte, fh->recvcsum);
    memcpy(dest+curbyte, fh->idstring, 32);
    curbyte += 32;
    curbyte += aimutil_put8(dest+curbyte, fh->flags);
    curbyte += aimutil_put8(dest+curbyte, fh->lnameoffset);
    curbyte += aimutil_put8(dest+curbyte, fh->lsizeoffset);
    memcpy(dest+curbyte, fh->dummy, 69);
    curbyte += 69;
    memcpy(dest+curbyte, fh->macfileinfo, 16);
    curbyte += 16;
    curbyte += aimutil_put16(dest+curbyte, fh->nencode);
    curbyte += aimutil_put16(dest+curbyte, fh->nlanguage);
    memset(dest+curbyte, 0x00, fh->hdrlen-curbyte);
    memcpy(dest+curbyte, fh->name, fh->hdrlen-curbyte);
    curbyte += fh->hdrlen-curbyte;
    return curbyte;
  }

  void oft_rx_init(struct oft_rx_machine *m, unsigned char *cookie, close_cb_t close_conn, append_cb_t append_to_file, int token) {
    m->state = PENDING_PROMPT;
    memset(m, 0x00, sizeof(struct oft_rx_machine));
    memcpy(m->cookie, cookie, 8);
    m->close_conn = close_conn;
    m->append_to_file = append_to_file;
    m->file_token = token;
  }
 
  int oft_rx_read_prompt(struct oft_rx_machine *m, unsigned char *buf, int pos, int l) {
    int avail_buf = l-pos;

    if(m->header_pos <= 6) { /* we still need length */
      int min_left = 6 - m->header_pos;
      int grab = avail_buf > min_left ? min_left : avail_buf;

      memcpy(m->cur_header+m->header_pos, buf+pos, grab);

      /* consume the input */
      pos += grab;
      avail_buf -= grab;

      /* extend the output */
      m->header_pos += grab;

      if(m->header_pos >= 6) { /* can now parse header length */
	m->header_len = aimutil_get16(m->cur_header+4);
      }
    }

    if(m->header_len) {
      int header_left = m->header_len - m->header_pos;
      int grab = avail_buf > header_left ? header_left : avail_buf;

      memcpy(m->cur_header+m->header_pos, buf+pos, grab);
      m->header_pos += grab;
      pos += grab;
      avail_buf -= grab;
    }

    if(m->header_len && m->header_len == m->header_pos) {
      aim_oft_getfh(&(m->fh), (unsigned char*)m->cur_header);
      memcpy(m->fh.bcookie, m->cookie, 8);
      /* TODO Should we actually sanity-check this? */
      m->state = SEND_ACK;
    }

    return pos;
  }

  void am_rx_gentermack(struct oft_rx_machine *m, struct frame_t *f){
    f->l = m->fh.hdrlen;
    f->buf = (unsigned char *)malloc(f->l);

    aim_oft_buildheader((unsigned char *)f->buf, &(m->fh));
    aimutil_put16(f->buf+6, 0x0204);
  }

  void oft_rx_genack(struct oft_rx_machine *m, struct frame_t *f) {
    f->l = m->fh.hdrlen;
    f->buf = (unsigned char *)malloc(f->l);

    aim_oft_buildheader((unsigned char *)f->buf, &(m->fh));
    aimutil_put16(f->buf+6, 0x0202);
  }

  struct frame_queue_t *oft_rx_run(struct oft_rx_machine *m, unsigned char *buf, int l) {
    struct frame_queue_t *retval = NULL;
    int pos;

    for(pos = 0; pos < l; /**/) {
    
      if(PENDING_PROMPT == m->state) {
	pos = oft_rx_read_prompt(m, buf, pos, l);
      }

      if(SEND_ACK == m->state) {
	struct frame_t f;

	oft_rx_genack(m, &f);
	retval = frame_enqueue(retval, f.buf, f.l, NO_SEQNO_FIXUP);
	m->state = RX_IN_XFER;
      }

      if(RX_IN_XFER == m->state) {
	int file_rem = m->fh.size - m->file_pos;
	int buf_rem = l-pos;

	if(buf_rem > file_rem) {
	  /* XXX What's the recovery here? */
	  buf_rem = file_rem;
	}
	
	if(m->append_to_file)
	  m->append_to_file(m->file_token, buf+pos, buf_rem);

	m->file_pos += buf_rem;
	pos += buf_rem;

	if(m->file_pos >= m->fh.size) {
	  struct frame_t f;
	  am_rx_gentermack(m, &f);

	  retval = frame_enqueue(retval, f.buf, f.l, NO_SEQNO_FIXUP);
	  m->state = TERM_ACK;
	}
      } else {
	if(TERM_ACK == m->state) {
	  m->state = TX_PENDING_CLOSE;
	}
      }

      if(TX_PENDING_CLOSE == m->state) {
	if(m->close_conn)
	  m->close_conn(m->file_token);

	m->state = TX_DONE;
      }
    }
    
    return retval;
  }

  void oft_tx_init(struct oft_tx_machine *m, struct aim_fileheader_t *fh, close_cb_t close_conn, begin_xfer_cb begin_xfer, resume_cb resume_offset, int token) {
    memset(m, 0, sizeof(struct oft_tx_machine));
    memcpy(&m->fh, fh, sizeof(struct aim_fileheader_t));
    m->state = LISTENING;
    m->close_conn = close_conn;

    m->begin_xfer = begin_xfer;
    m->resume_offset = resume_offset;
    m->file_token = token;

    
  }

  int oft_tx_accepted(struct oft_tx_machine *m) {
    switch(m->state) {
    case LISTENING:
      m->state = WAIT_START;
      break;
    case WAIT_CONNECT:
      m->state = SEND_PROMPT;
      break;
    default:
      /* XXX recovery? */
      break;
    }

    if(m->state == SEND_PROMPT)
      return 1;

    return 0;
  }

  int oft_tx_got_ooback(struct oft_tx_machine *m) {
    switch(m->state) {
    case LISTENING:
      m->state = WAIT_CONNECT;
      break;
    case WAIT_START:
      m->state = SEND_PROMPT;
      break;
    default:
      /* XXX recovery? */
      break;
    }

    if(m->state == SEND_PROMPT)
      return 1;

    return 0;
  }

  void oft_tx_genprompt(struct oft_tx_machine *m, struct frame_t *f) {
    f->l = m->fh.hdrlen;
    f->buf = (unsigned char*)malloc(f->l);
    aim_oft_buildheader((unsigned char*)f->buf, &(m->fh));
  }

  void oft_tx_reset_headerbuf(struct oft_tx_machine *m) {
    memset(m->cur_header, 0, 512);
    m->header_pos = 0;
    m->header_len = 0;
  }

  int oft_tx_read_ack(struct oft_tx_machine *m, unsigned char* buf, int pos, int l) { 
    /* Why yes, yes this is just cut and paste from oft_rx_read_prompt */
    int avail_buf = l-pos;

    if(m->header_pos <= 6) { /* we still need length */
      int min_left = 6 - m->header_pos;
      int grab = avail_buf > min_left ? min_left : avail_buf;

      memcpy(m->cur_header+m->header_pos, buf+pos, grab);

      /* consume the input */
      pos += grab;
      avail_buf -= grab;

      /* extend the output */
      m->header_pos += grab;

      if(m->header_pos >= 6) { /* can now parse header length */
	m->header_len = aimutil_get16(m->cur_header+4);
      }
    }

    if(m->header_len) {
      int header_left = m->header_len - m->header_pos;
      int grab = avail_buf > header_left ? header_left : avail_buf;

      memcpy(m->cur_header+m->header_pos, buf+pos, grab);
      m->header_pos += grab;
      pos += grab;
      avail_buf -= grab;
    }

    if(m->header_len && m->header_len == m->header_pos) {
      struct aim_fileheader_t prompt_fh;

      aim_oft_getfh(&prompt_fh, (unsigned char*)m->cur_header);

      if(prompt_fh.hdrtype == 0x0205) {
	if(m->resume_offset)
	  m->resume_offset(m, m->file_token, prompt_fh.nrecvd);
	m->fh.nrecvd = prompt_fh.nrecvd;
	m->fh.hdrtype = 0x0106;
	m->file_pos = m->fh.nrecvd;
	m->state = GOT_RESUME;
	oft_tx_reset_headerbuf(m);

	return pos;
      }



      /* TODO Should we actually sanity-check this? */
      switch(m->state) {
      case WAIT_ACK:
	m->state = TX_IN_XFER;
	m->begin_xfer(m, m->file_token);
	oft_tx_reset_headerbuf(m);
	break;
      case TX_IN_XFER:
      case WAIT_TERM_ACK:
	oft_tx_reset_headerbuf(m);
	m->state = TX_PENDING_CLOSE;
	m->close_conn(m->file_token);
	break;
      default:
	break;
      }
    }

    return pos;
  }

  int oft_tx_sent_bytes(struct oft_tx_machine *m, int n) {
    int left;

    m->file_pos += n;

    left = m->fh.size - m->file_pos;
    if(left <= 0){
      m->state = WAIT_TERM_ACK;
    }

    return left;
  }

  struct frame_queue_t *oft_tx_run(struct oft_tx_machine *m, unsigned char *buf, int l) {
    struct frame_queue_t *retval;
    int pos;
    retval = NULL;

    if(SEND_PROMPT == m->state){
      struct frame_t f;
      
      oft_tx_genprompt(m, &f);
      retval = frame_enqueue(retval, f.buf, f.l, NO_SEQNO_FIXUP);
      m->state = WAIT_ACK;

      /* This loses the content of the buffer... but this should only
	 be called in the case that we've set up the connection.

	 We shouldn't get data until we're in the WAIT_ACK state! */

      return retval;
    }

    for(pos = 0; pos < l; /**/) { 

      if(TX_IN_XFER == m->state) {
	/* The other side snuck up on us! */
	pos = oft_tx_read_ack(m, buf, pos, l);
      }



      if(WAIT_ACK == m->state) {
	pos = oft_tx_read_ack(m, buf, pos, l);
      }

      if(WAIT_TERM_ACK == m->state) {
	pos = oft_tx_read_ack(m, buf, pos, l);
      }

      if(GOT_RESUME == m->state) {
	struct frame_t f;
	
	oft_tx_genprompt(m, &f);
	retval = frame_enqueue(retval, f.buf, f.l, NO_SEQNO_FIXUP);
	
	m->state = TX_IN_XFER;
	m->begin_xfer(m, m->file_token);
      }
    }

    return retval;
  }

#ifdef __cplusplus
}
#endif
