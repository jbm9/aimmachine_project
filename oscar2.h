#ifndef __OSCAR2_H__
#define __OSCAR2_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __OSCAR2_TEST__
#define TEST
#endif

#ifdef __OFT_TEST__
#define TEST
#endif


#ifdef DEBUG
#define FUNC(x) printf("Entering %s at %s:%d\n", (x), __FILE__, __LINE__)
#else
#define FUNC(x) {}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

#ifdef TEST
static void fail(char *file, int line, char *msg) {
  printf("FAIL: %s:%d: %s\n", file, line, msg);
}

#define FAIL(msg) fail(__FILE__, __LINE__, (msg))

#endif


#include "frame.h"

#define aimutil_put8(buf, data) ((*(buf) = (unsigned char)(data)&0xff),1)
#define aimutil_get8(buf) ((*(buf))&0xff)
#define aimutil_put16(buf, data) ( \
                                  (*(buf) = (unsigned char)((data)>>8)&0xff), \
                                  (*((buf)+1) = (unsigned char)(data)&0xff),  \
                                  2)
#define aimutil_get16(buf) ((((*(buf))<<8)&0xff00) + ((*((buf)+1)) & 0xff))
#define aimutil_put32(buf, data) ( \
                                  (*((buf)) = (unsigned char)((data)>>24)&0xff), \
                                  (*((buf)+1) = (unsigned char)((data)>>16)&0xff), \
                                  (*((buf)+2) = (unsigned char)((data)>>8)&0xff), \
                                  (*((buf)+3) = (unsigned char)(data)&0xff), \
                                  4)
#define aimutil_get32(buf) ((((*(buf))<<24)&0xff000000) + \
                            (((*((buf)+1))<<16)&0x00ff0000) + \
                            (((*((buf)+2))<< 8)&0x0000ff00) + \
                            (((*((buf)+3)    )&0x000000ff)))



#define aimutil_put_tlv_short(buf, t, l, s) \
  aimutil_put16( (buf), (t) );\
    aimutil_put16( (buf)+2, (l) );		\
    aimutil_put16( (buf)+4, (s) );


  /* These come in the presence packets */
#define AIM_FLAG_AWAY            0x0020
#define AIM_FLAG_ICQ             0x0040

  /**/
#define AIM_USERINFO_PRESENT_IDLE         0x00000008

  enum { AIM_PRES_ONLINE = 0x00,
	 AIM_PRES_AWAY   = 0x01,
	 AIM_PRES_DND    = 0x02,
	 AIM_PRES_OUT    = 0x04,
	 AIM_PRES_BUSY   = 0x10,
	 AIM_PRES_CHAT   = 0x20,
	 AIM_PRES_INVIS  = 0x100 };



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
  enum { BASE_SEQNO = 0x1899 };

  enum { NO_SEQNO_FIXUP = 0, SEQNO_FIXUP_YES = 1 };

  struct aim_machine {
    enum flow_state_t { DEAD_AIR=0, PREHEADER=1, INHEADER=2, INBODY=3 } flow_state;
    unsigned short seqno;
    unsigned char header[HEADER_LEN];

    unsigned short body_len;
    unsigned char *body_buf;
    unsigned short body_pos;

    struct frame_queue_t *to_inject;

    struct am_cb_entry_t *callbacks;

    struct frame_queue_t *forbidden_cookies;

    struct aim_machine *partner;

    int nukebyte;
  };

  typedef int (*am_event_cb)(struct aim_machine *, ...);

  struct am_cb_entry_t {
    unsigned short family;
    unsigned short subtype;
    am_event_cb f;

    struct am_cb_entry_t *next;
  };

  struct am_tlv {
    unsigned short type;
    unsigned short length;
    unsigned char *value;
  };

  struct am_tlv_list {
    struct am_tlv tlv;
    struct am_tlv_list *next;
  };




  /* exported interfaces */

  void am_init(struct aim_machine *s);
  void am_finish(struct aim_machine *s);
  void am_init_pair(struct aim_machine *in, struct aim_machine *out);
  struct frame_queue_t * am_run(struct aim_machine *, struct frame_t *);
  int am_inject_frame(struct aim_machine *, struct frame_t *, int);
  int am_register_event_cb(struct aim_machine *, unsigned short, unsigned short, am_event_cb); 
  int am_inject_message_outbound(struct aim_machine *, char *, char *);
  int am_inject_message_inbound(struct aim_machine *, char *, char *);
  int am_inject_sendfile(struct aim_machine *, char *, char *, short, char *, int);


#ifdef __OSCAR2_TEST__
    /* Private/"static" interfaces only exposed for testing */
  void mk_cookie(unsigned char *cookie);
  int am_cur_body_sanitized(struct aim_machine *, unsigned char*);
  void am_parse_body(struct aim_machine *, unsigned char *, int);
  
  struct frame_queue_t *am_frame_inject(struct aim_machine *, struct frame_queue_t *) ;
  int am_attempt_header_parse(struct aim_machine *, unsigned char *, int);
  int am_scan_to_header(struct frame_t *f, int);
  int am_load_body(struct aim_machine *, struct frame_queue_t **, unsigned char *, int, int);
  void am_apply_seqno(struct aim_machine *, unsigned char *);
  void am_fixup_seqnos(struct aim_machine *, struct frame_queue_t *);


  void parse_msg_inbound(struct aim_machine *s, unsigned char *buf, int l, am_event_cb cb);

  am_event_cb am_cb_find(struct am_cb_entry_t *, unsigned short, unsigned short);

  struct frame_t *am_toy_format_message(unsigned char *, unsigned char *, unsigned char *);
  struct frame_t *am_format_oft_sendfile_req(const unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *, unsigned short, const unsigned char *,  int);

  void am_add_forbidden_cookie(struct aim_machine *s, unsigned char *cookie);

  struct frame_t *am_format_message_outbound(unsigned char *, unsigned char *, unsigned char *, unsigned char *);
  struct frame_t *am_format_message_inbound(unsigned char *, unsigned char *, unsigned char *, unsigned char *);

#endif /* def __OSCAR2_TEST__ */



#ifdef __cplusplus
}
#endif

#endif /* ndef __OSCAR2_H__ */
