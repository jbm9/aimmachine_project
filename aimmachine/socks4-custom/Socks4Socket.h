/**
 **	File ......... Socks4Socket.h
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
#ifndef _SOCKS4SOCKET_H
#define _SOCKS4SOCKET_H

#include <TcpSocket.h>
#include <ISocketHandler.h>

#define MAXLEN_USERID 1000

class RemoteSocket;

class Socks4Socket : public TcpSocket
{
	enum state_t {
		STATE_VN = 0,
		STATE_CD,
		STATE_DSTPORT,
		STATE_DSTIP,
		STATE_USERID,
		STATE_TRY_CONNECT,
		STATE_CONNECTED,
		STATE_FAILED,
		STATE_ACCEPT
	};
	enum state5_t {
		STATE5_VER = 0,
		STATE5_NMETHODS,
		STATE5_METHODS,
		STATE5_FAILED,
		STATE5_GSSAPI, // not implemented
		STATE5_USERPASSWORD_VER,
		STATE5_USERPASSWORD_ULEN,
		STATE5_USERPASSWORD_UNAME,
		STATE5_USERPASSWORD_PLEN,
		STATE5_USERPASSWORD_PASSWD,
		STATE5_VER2,
		STATE5_CMD,
		STATE5_RSV,
		STATE5_ATYP,
		STATE5_ALEN,
		STATE5_DST_ADDR,
		STATE5_DST_PORT
	};
public:
	Socks4Socket(ISocketHandler& );
	~Socks4Socket();

	void OnAccept() {
		printf("Incoming\n");
	}
	void OnRead();
	void Socks4Read();
	void Socks5Read();

	void Connected();
	void ConnectFailed();
	void Accept();

	void SetRemote(RemoteSocket *p) { m_pRemote = p; }

private:
	Socks4Socket(const Socks4Socket& s) : TcpSocket(s) {} // copy constructor
	Socks4Socket& operator=(const Socks4Socket& ) { return *this; } // assignment operator
	void SetupConnect();
	void SetupBind();
	void DoSocks5();

	state_t m_state;
	char m_vn;
	char m_cd;
	unsigned short m_dstport;
	unsigned long m_dstip;
	char m_userid[MAXLEN_USERID];
	int m_userid_ptr;
	RemoteSocket *m_pRemote;
	// socks5 support
	bool m_bSocks5;
	state5_t m_state5;
	char m_s5_ver;
	unsigned char m_s5_nmethods;
	int m_s5_method;
	unsigned char m_s5_methods[256];
	// username/password method
	char m_s5_userpassword_ver;
	unsigned char m_s5_userpassword_ulen;
	int m_s5_uptr;
	char m_s5_userpassword_uname[256];
	unsigned char m_s5_userpassword_plen;
	int m_s5_pptr;
	char m_s5_userpassword_passwd[256];
	// socks5 cont'd
	char m_s5_ver2;
	char m_s5_cmd;
//	char m_s5_rsv;//0
	char m_s5_atyp;
	unsigned char m_s5_alen;
	int m_s5_aptr;
	unsigned char m_s5_dst_addr[256];
	unsigned short m_s5_dst_port;
};




#endif // _SOCKS4SOCKET_H
