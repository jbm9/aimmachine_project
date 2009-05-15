/**
 **	File ......... RemoteSocket.h
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
#ifndef _REMOTESOCKET_H
#define _REMOTESOCKET_H

#include <TcpSocket.h>
#include <ISocketHandler.h>
#include "oscar2.h"

class Socks4Socket;

class RemoteSocket : public TcpSocket
{
public:
	RemoteSocket(ISocketHandler& );
	~RemoteSocket();

	void SetRemote(Socks4Socket *p) { m_pRemote = p; }

	void OnConnect();
	void OnConnectFailed();
	void OnAccept();

	void OnRead();

	void SendBuf(const char*buf, size_t len, int f=0);

private:
	RemoteSocket(const RemoteSocket& s) : TcpSocket(s) {} // copy constructor
	RemoteSocket& operator=(const RemoteSocket& ) { return *this; } // assignment operator

	Socks4Socket *m_pRemote;

	void DumpBuf(const char *, int);

	struct aim_machine ostate;
	struct aim_machine istate;
};




#endif // _REMOTESOCKET_H
