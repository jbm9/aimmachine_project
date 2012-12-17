#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>

#include "oscar2.h"
#include "oft.h"

#define QUEUE_SIZE          1024

static int in_xfer = 0;

int close_conn(int token) { 
  printf("CLOSE callback\n");
  /* token happens to be our sock fd... */
  close(token);
}

int begin_xfer(struct oft_tx_machine *m, int token) {
  printf("XFER callback\n");
  in_xfer = 1;
}

int resume_offset(struct oft_tx_machine *m, int token, int pos) {
  printf("resume callback\n");
  lseek(token, pos, SEEK_SET);
}

int main(int argc, char* argv[])
{
  int sock,sock_serv;
  struct hostent* pHostInfo;
  struct sockaddr_in addr;
  int addr_size = sizeof(struct sockaddr_in);
  int nHostPort;

  int i,fd;

  fd_set rset, wset, eset;
  struct timeval timeout;

  struct frame_queue_t *queue = NULL;
  struct oft_tx_machine tx;

  struct aim_fileheader_t fh;

  char *filename;
  struct stat s;
  int len;
  char cookie[8] = "ignored!";


  sock = -1;

  if(argc < 3) {
    printf("\nUsage: server host-port filename\n");
    return 1;
  } else {
    nHostPort = atoi(argv[1]);
    filename = argv[2];
  }


  /* set up the filetransfer */
  i = stat(filename, &s);
  if(-1 == i) {
    perror("stat");
    return 1;
  }
  
  printf("Filling fileheader for file \"%s\", length %d (cookie: \"%s\")\n",
	 filename, s.st_size, cookie);

  oft_fill_fileheader(&fh, cookie, filename, s.st_size);
  
  oft_tx_init(&tx, &fh, close_conn, begin_xfer, resume_offset, 0x9981);





  /* make a socket */
  sock_serv = socket(AF_INET,SOCK_STREAM,0);

  if(-1 == sock_serv) {
    perror("socket");
    return 2;
  }

  /* fill address struct */
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(nHostPort);
  addr.sin_family = AF_INET;

  printf("\nBinding to port %d",nHostPort);

  /* bind to a port */
  i = bind(sock_serv,(struct sockaddr*)&addr,sizeof(addr)); 
  if(-1 == i) {
    perror("bind");
    return 0;
  }

  /* establish listener */
  i = listen(sock_serv,QUEUE_SIZE);
  if(-1 == i) {
    perror("listen");
    return 1;
  }

  for(;;){
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    if(-1 == sock) {
      /* only room for one client, so don't look at listener until they're done =) */
      printf("RSET: sock_serv\n");
      FD_SET(sock_serv, &rset);
    } else {
      printf("RSET: sock\n");
      FD_SET(sock, &rset);

      if(in_xfer || queue) {
	printf("WSET: sock\n");
	FD_SET(sock, &wset);
      }
    }

    /* 32 sounds good... */
    i = select(32, &rset, &wset, &eset, NULL);

    printf("select gave me %d\n", i);

    if(FD_ISSET(sock_serv, &rset)) {
      int flags;
      /* get the connected socket */
      sock = accept(sock_serv,(struct sockaddr*)&addr,(socklen_t *)&addr_size);
      printf("accepted socket %d\n", sock);
      if(-1 == sock) {
	perror("accept");
	return 1;
      }

      /* This actually should be initialized before establishing the listener... 
	 But we have to refresh it between clients, so it's easiest to do it here.
       */
      /* open our file */
      fd = open(filename, O_RDONLY);
      in_xfer = 0;

      /* Set up our tx machine */
  oft_tx_init(&tx, &fh, close_conn, begin_xfer, resume_offset, 0x9981);
      oft_tx_accepted(&tx);
      if(1 == oft_tx_got_ooback(&tx)) {
	queue = oft_tx_run(&tx, NULL, 0);
      }


      /* Set our socket to non-blocking for the obvious reasons */
      flags = fcntl(sock, F_GETFL, 0);
      if(-1 == flags) {
	perror("fcntl get");
	return 1;
      }

      flags |= O_NONBLOCK;
      i = fcntl(sock, F_SETFL, flags);
      if(-1 == i) {
	perror("fcntl set");
	return 1;
      }

    }

    if(FD_ISSET(sock, &rset)) {
      char buf[8192];
      int l;
      printf("got client ready for read\n");
      l = read(sock, buf, 8192);

      printf("read: %d\n", l);

      if(l < 0) {
	if(errno != EWOULDBLOCK)
	  perror("client read");
      } else if(l == 0) {
	printf("client disconnected");
	close(sock);
	sock = -1;
	in_xfer = 0;
      }	else {
	struct frame_queue_t *next = oft_tx_run(&tx, buf, l);
	queue = frame_enqueue_append(queue, next);

	printf("got %d bytes from client, engine output: %p\n", l, next);
      }
    }

    if(-1 != sock && FD_ISSET(sock, &wset)) {
      printf("got client ready for write, queue=%p, in_xfer=%d\n", queue, in_xfer);
      if(queue) {	// send next chunk from queue
	struct frame_queue_t *q;
	int n = 0;
	q = queue;

	while(q) {
	  /* force completion of current frame */
	  while(n < q->f.l) {
	    n += write(sock, q->f.buf+n, q->f.l-n);
	    printf("Queue: Wrote %d out of %d bytes\n", n, q->f.l);
	  }
	  queue = q->next;
	  free(q->f.buf);
	  free(q);

	  q = queue;
	}
      }

      if(!queue && in_xfer) {
	int n_in, n_out;
	char buf[2048];
	n_in = read(fd, buf, 2048);

	if(n_in == 0) {
	  close(fd);
	  in_xfer = 0;
	}

	for(n_out = 0; n_out < n_in; /**/) {
	  // send next chunk of file
	  n_out += write(sock, buf+n_out, n_in-n_out);
	  printf("Wrote %d out of %d bytes\n", n_out, n_in);
	  oft_tx_sent_bytes(&tx, n_out);
	}
      }
    }
  }
}




