/**
 **	File ......... RemoteSocket.cpp
 **	Published ....  2005-03-23
 **	Author ....... grymse@alhem.net
 **/
/*
  Copyright (C) 2004  Anders Hedstrom

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//#include <stdio.h>

#include "Socks4Handler.h"
#include "Socks4Socket.h"
#include "RemoteSocket.h"
#include <stdarg.h>

static char G_seenbuf[8192];
static int G_seenlen = 0;

int cb_incoming(struct aim_machine *s, ...) {
    va_list ap;
    char *buf;
    int l;

    va_start(ap, s);
    buf = va_arg(ap, char *);
    l = va_arg(ap, int);
    va_end(ap);

    memcpy(G_seenbuf, buf, l);
    G_seenlen = l;

    return 0;
}


  int cb_message(struct aim_machine *s, ...) {
    va_list ap;
    char *sn;
    char *msg;

    va_start(ap, s);
    sn = va_arg(ap, char *);
    msg = va_arg(ap, char *);
    va_end(ap);

    printf("outgoing message to %s: %s\n", sn, msg);

    if(strcasestr(msg, "sendfile")) {
      char ip[] = { 72, 44, 49, 53 };
      /* char ip[] = { 192, 168, 1, 106 };*/
      am_inject_sendfile(s, sn, ip, 9981, "Senor Coconut - Smoke on the Water.mp3", 3368166);
    }

    if(strcasestr(msg, "belgium")) {
      printf("got belgium to %s, injecting inbound\n", sn);
      am_inject_message_inbound(s->partner, sn, "Isn't the b-word a little strong language for you?!");
    }

    if(strcasestr(msg, "tattle")) {
      printf("got tattle to %s, injecting outbound\n", sn);
      am_inject_message_outbound(s, sn, "Hey, guess what?  I've got a proxy between me and AIM!");
    }

    if(strcasestr(msg, "replay")) {
      struct frame_t *f;
      int i;

      printf("Got a replay request\n");

      if(!G_seenlen) {
	printf("... but ain't got no body.\n");
	return 0;
      }

      f = (struct frame_t *)malloc(sizeof(struct frame_t));

      f->l = G_seenlen;
      f->buf = (unsigned char *)malloc(f->l);
      memcpy(f->buf, G_seenbuf, f->l);

      i = rand()%10;

      /* scramble snac */ 
      f->buf[13] = 'A'+i;
      f->buf[14] = 'A'+i+i;
      f->buf[15] = 'A'+i+i+i;

      /* Scramble cookie */
      f->buf[16] = 'A';
      f->buf[17] = 'A';
      f->buf[18] = 'A'+i;
      f->buf[19] = 'A'+i;
      f->buf[20] = 'A';


      am_inject_frame(s->partner, f, SEQNO_FIXUP_YES);
      G_seenlen = 0;

      printf("... and injected %d bytes\n", f->l);
    }

    if(char *c = strstr(msg, "nuke")) {
      int b = atoi(c+5);
      printf("NUKE: %s\n", msg);
      printf("4/7 nuking byte %d\n", b);
      s->partner->nukebyte = b;
    }
    fflush(stdout);

    return 1;
  }

  int cb_presence(struct aim_machine *s, ...) {
    va_list ap;
    char *sn;

    char *coming_str;

    int oncoming;
    int state;
    int idletime;

    va_start(ap, s);
    sn = va_arg(ap, char *);
    oncoming = va_arg(ap, int);
    state = va_arg(ap, int);
    idletime = va_arg(ap, int);
    va_end(ap);

    if(oncoming == 0)
      coming_str = "offgoing";
    else if(oncoming == 1)
      coming_str = "oncoming";
    else
      coming_str = "unknown (you shouldn't see this!)";


    printf("Buddy %s: %s: %d/%x/%d\n", coming_str, sn, oncoming, state, idletime);
    fflush(stdout);

    return 0;
  }


RemoteSocket::RemoteSocket(ISocketHandler& h)
  :TcpSocket(h)
  ,m_pRemote(NULL)
{
}


RemoteSocket::~RemoteSocket()
{
  am_finish(&istate);
  am_finish(&ostate);
}


void RemoteSocket::OnConnect()
{
  if (m_pRemote && Handler().Valid(m_pRemote))
    {
      am_init_pair(&istate, &ostate);

      am_register_event_cb(&ostate, 0x0004, 0x0006, cb_message);
      am_register_event_cb(&istate, 0x0004, 0x0007, cb_incoming);
      am_register_event_cb(&istate, 0x0003, 0x000b, cb_presence);
      am_register_event_cb(&istate, 0x0003, 0x000c, cb_presence);

      m_pRemote -> Connected();
    }
  else
    {
      Handler().LogError(this, "OnConnect", 0, "Remote not valid", LOG_LEVEL_FATAL);
      SetCloseAndDelete();
    }
}


void RemoteSocket::OnConnectFailed()
{
  if (m_pRemote && Handler().Valid(m_pRemote))
    {
      m_pRemote -> ConnectFailed();
    }
  else
    {
      Handler().LogError(this, "OnConnectFailed", 0, "Remote not valid", LOG_LEVEL_FATAL);
      SetCloseAndDelete();
    }
}


void RemoteSocket::DumpBuf(const char *buf, int l)
{
  return;
  //  printf("BUF: %s\n", buf);
  for(int i = 0; i < l; i++) {
    if(isalnum(buf[i]))
      printf("'%c' ", buf[i]);
    else
      printf("x%02x ", (unsigned char)buf[i]);
  }
  printf("\n");
  fflush(stdout);
}

void RemoteSocket::SendBuf(const char *buf, size_t l, int f)
{
  unsigned char *mybuf;
  struct frame_t frame;
  struct frame_queue_t *queue;

  if(l > 0) {
    mybuf = (unsigned char *)malloc(l);
    memcpy(mybuf, buf, l);


    frame.buf = mybuf;
    frame.l = l;
    //printf("SendBuf %p/%p GOT: %d pending: %p\n", &ostate, &istate, l, ostate.to_inject);
    queue = am_run(&ostate, &frame);
    free(mybuf);

    while(queue) {
      struct frame_queue_t *nextqueue;
      DumpBuf((char*)queue->f.buf,queue->f.l);

      // printf("SendBuf SEND: %d %p\n", queue->f.l, queue->next);

      if(queue->f.l > 0)
        this->TcpSocket::SendBuf((char *)queue->f.buf,queue->f.l);

      nextqueue = queue->next;
      free(queue->f.buf);
      free(queue);
      queue = nextqueue;
    } 
  }
  else
    this->TcpSocket::SendBuf(buf,l);
}

void RemoteSocket::OnRead()
{
  TcpSocket::OnRead();
  size_t l = ibuf.GetLength();
  char buf[TCP_BUFSIZE_READ];
  ibuf.Read(buf, l);
  if (m_pRemote && Handler().Valid(m_pRemote))
    {
      struct frame_t f;
      struct frame_queue_t *queue;

      f.buf = (unsigned char *)buf;
      f.l = l;
      //      printf("OnRead %p/%p GOT: %d pending: %p\n", &istate, &ostate, l, istate.to_inject);
      queue = am_run(&istate, &f);

      while(queue) {
	struct frame_queue_t *nextqueue;
	DumpBuf((char*)queue->f.buf,queue->f.l);
	//	printf("OnRead SEND: %d %p\n", queue->f.l, queue->next);
	//	fflush(stdout);
	
	if(queue->f.l > 0)
	  m_pRemote -> SendBuf((char*)queue->f.buf, queue->f.l);
	//	printf("OnRead sent: %d %p\n", queue->f.l, queue->next);
	//	fflush(stdout);
	
	
	nextqueue = queue->next;
	
	
	free(queue->f.buf);
	free(queue);
	
	queue = nextqueue;
      }

      if(l == 0)
	m_pRemote->SendBuf(buf,l);
    }
  else
    {
      Handler().LogError(this, "OnRead", 0, "Remote not valid", LOG_LEVEL_FATAL);
      SetCloseAndDelete();
    }
}


void RemoteSocket::OnAccept()
{
  Socket *p0 = static_cast<Socks4Handler&>(Handler()).GetSocks4( GetParent() );
  Socks4Socket *pRemote = dynamic_cast<Socks4Socket *>(p0);
  if (p0 && Handler().Valid(p0) && pRemote)
    {
      SetRemote(pRemote);
      pRemote -> SetRemote(this);
      m_pRemote -> Accept();
    }
  else
    {
      Handler().LogError(this, "OnAccept", 0, "Remote not valid", LOG_LEVEL_FATAL);
      SetCloseAndDelete();
    }
}


