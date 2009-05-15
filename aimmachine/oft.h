#ifndef __OFT_H__
#define __OFT_H__

#include "oscar2.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct aim_fileheader_t {
    unsigned char  magic[4];            /* 0 */
    short hdrlen;           /* 4 */
    short hdrtype;             /* 6 */
    unsigned char  bcookie[8];       /* 8 */
    short encrypt;          /* 16 */
    short compress;         /* 18 */
    short totfiles;         /* 20 */
    short filesleft;        /* 22 */
    short totparts;         /* 24 */
    short partsleft;        /* 26 */
    long  totsize;          /* 28 */
    long  size;             /* 32 */
    long  modtime;          /* 36 */
    long  checksum;         /* 40 */
    long  rfrcsum;          /* 44 */
    long  rfsize;           /* 48 */
    long  cretime;          /* 52 */
    long  rfcsum;           /* 56 */
    long  nrecvd;           /* 60 */
    long  recvcsum;         /* 64 */
    unsigned char  idstring[32];     /* 68 */
    unsigned char  flags;            /* 100 */
    unsigned char  lnameoffset;      /* 101 */
    unsigned char  lsizeoffset;      /* 102 */
    unsigned char  dummy[69];        /* 103 */
    unsigned char  macfileinfo[16];  /* 172 */
    short nencode;          /* 188 */
    short nlanguage;        /* 190 */
    unsigned char  name[256];         /* 192 */
    /* 256 */
  };


  struct aim_fileheader_t *aim_oft_getfh(struct aim_fileheader_t *fh, unsigned char *hdr);
  int aim_oft_buildheader(unsigned char *dest,struct aim_fileheader_t *fh);
  int cmp_fileheader(struct aim_fileheader_t *a, struct aim_fileheader_t *b);

  struct aim_fileheader_t *oft_fill_fileheader(struct aim_fileheader_t *fh, char * cookie, const char *filename, int size);

  typedef int(*close_cb_t)(int);
  typedef int(*append_cb_t)(int, unsigned char*, int);


  struct oft_rx_machine {
    struct aim_fileheader_t fh;
    unsigned char cookie[8];
    enum { PENDING_PROMPT = 0, IN_PROMPT, SEND_ACK, RX_IN_XFER, TERM_ACK, RX_PENDING_CLOSE, RX_DONE } state;
    close_cb_t close_conn;
    unsigned int pos;
    int file_token;
    append_cb_t append_to_file;

    unsigned char cur_header[512]; /* well too big, just in case we need it someday */
    int header_pos;
    int header_len;

    int file_len;
    int file_pos;
  };


  void oft_rx_init(struct oft_rx_machine *, unsigned char *, close_cb_t, append_cb_t, int);
  struct frame_queue_t *oft_rx_run(struct oft_rx_machine *, unsigned char *, int);

  struct oft_tx_machine {
    struct aim_fileheader_t fh;

    enum { LISTENING = 0, WAIT_CONNECT, WAIT_START, SEND_PROMPT, WAIT_ACK, GOT_RESUME, TX_IN_XFER, WAIT_TERM_ACK, TX_PENDING_CLOSE, TX_DONE } state;
    close_cb_t close_conn;

    int(*begin_xfer)(struct oft_tx_machine *, int);
    int(*resume_offset)(struct oft_tx_machine *, int, int);
    int file_token;

    unsigned char cur_header[512]; /* well too big, just in case we need it someday */
    int header_pos;
    int header_len;

    int file_pos;
  };

  typedef int(*begin_xfer_cb)(struct oft_tx_machine *, int);
  typedef int(*resume_cb)(struct oft_tx_machine *, int, int);

  void oft_tx_init(struct oft_tx_machine *, struct aim_fileheader_t *, close_cb_t, begin_xfer_cb, resume_cb, int);
  int oft_tx_accepted(struct oft_tx_machine *);
  int oft_tx_got_ooback(struct oft_tx_machine *);

  struct frame_queue_t *oft_tx_run(struct oft_tx_machine *, unsigned char *, int);
  int oft_tx_sent_bytes(struct oft_tx_machine *, int);


#ifdef __cplusplus
}
#endif

#endif /* ndef __OFT_H__ */
