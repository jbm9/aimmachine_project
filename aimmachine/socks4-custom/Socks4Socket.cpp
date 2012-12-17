/**
 **	File ......... Socks4Socket.cpp
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
#include <ListenSocket.h>

#include "Socks4Handler.h"
#include "RemoteSocket.h"
#include "Socks4Socket.h"




Socks4Socket::Socks4Socket(ISocketHandler& h)
:TcpSocket(h)
,m_state(STATE_VN)
,m_userid_ptr(0)
,m_pRemote(NULL)
,m_bSocks5(false)
,m_state5(STATE5_VER)
,m_s5_method(0)
{
}


Socks4Socket::~Socks4Socket()
{
}




void Socks4Socket::OnRead()
{
	// ibuf is inherited member variable of TcpSocket
	TcpSocket::OnRead();
	if (m_bSocks5)
		Socks5Read();
	else
		Socks4Read();
}


void Socks4Socket::Socks4Read()
{
	bool need_more = false;
	while (ibuf.GetLength() && !need_more && !CloseAndDelete() )
	{
		size_t l = ibuf.GetLength();
		switch (m_state)
		{
		case STATE_VN: // get socks version
			ibuf.Read( (char *)&m_vn, 1);
			if (m_vn == 5) // socks5
			{
				m_bSocks5 = true;
				m_state5 = STATE5_NMETHODS;
			}
			else
			if (m_vn == 4)
			{
				m_state = STATE_CD;
			}
			else
			{
				Handler().LogError(this, "OnRead", m_vn, "Unsupported socks version", LOG_LEVEL_FATAL);
				SetCloseAndDelete();
			}
			break;
		case STATE_CD:
			ibuf.Read( (char *)&m_cd, 1);
			if (m_cd != 1 && m_cd != 2)
			{
				Handler().LogError(this, "OnRead", m_cd, "Bad command code", LOG_LEVEL_FATAL);
				SetCloseAndDelete();
			}
			else
			{
				m_state = STATE_DSTPORT;
			}
			break;
		case STATE_DSTPORT:
			if (l > 1)
			{
				ibuf.Read( (char *)&m_dstport, 2);
				m_state = STATE_DSTIP;
			}
			else
			{
				need_more = true;
			}
			break;
		case STATE_DSTIP:
			if (l > 3)
			{
				ibuf.Read( (char *)&m_dstip, 4);
				m_state = STATE_USERID;
			}
			else
			{
				need_more = true;
			}
			break;
		case STATE_USERID:
			if (m_userid_ptr < MAXLEN_USERID)
			{
				ibuf.Read(m_userid + m_userid_ptr, 1);
				if (m_userid[m_userid_ptr] == 0)
				{
					switch (m_cd)
					{
					case 1:
						SetupConnect();
						break;
					case 2:
						SetupBind();
						break;
					}
				}
				m_userid_ptr++;
			}
			else
			{
				Handler().LogError(this, "OnRead", m_userid_ptr, "Userid too long", LOG_LEVEL_FATAL);
				SetCloseAndDelete();
			}
			break;
		case STATE_TRY_CONNECT:
			break;
		case STATE_CONNECTED:
			{
				char buf[TCP_BUFSIZE_READ];
				ibuf.Read(buf, l);
				if (m_pRemote && Handler().Valid(m_pRemote))
				{
					m_pRemote -> SendBuf(buf, l);
				}
			}
			break;
		case STATE_FAILED:
			SetCloseAndDelete();
			break;
		case STATE_ACCEPT:
			break;
		}
	}
}


void Socks4Socket::Connected()
{
	if (m_state == STATE_TRY_CONNECT)
	{
		char reply[8];
		reply[0] = 0;
		reply[1] = 90; // granted
		memcpy(reply + 2, &m_dstport, 2);
		memcpy(reply + 4, &m_dstip, 4);
		SendBuf(reply, 8);
		m_state = STATE_CONNECTED;
	}
	else
	{
		Handler().LogError(this, "Connected", m_state, "bad state", LOG_LEVEL_FATAL);
		SetCloseAndDelete();
	}
}


void Socks4Socket::ConnectFailed()
{
	if (m_state == STATE_TRY_CONNECT)
	{
		char reply[8];
		reply[0] = 0;
		reply[1] = 91; // failed
		memcpy(reply + 2, &m_dstport, 2);
		memcpy(reply + 4, &m_dstip, 4);
		SendBuf(reply, 8);
		m_state = STATE_FAILED;
		SetCloseAndDelete();
	}
	else
	{
		Handler().LogError(this, "ConnectFailed", m_state, "bad state", LOG_LEVEL_FATAL);
		SetCloseAndDelete();
	}
}


void Socks4Socket::SetupConnect()
{
char buf[128];

	m_pRemote = new RemoteSocket(Handler());
	port_t port = ntohs(m_dstport);
	ipaddr_t addr = m_dstip; //ntohl(m_dstip);

printf("SetupConnect(%s, %d)\n", inet_ntop(AF_INET, (void*)&m_dstip, buf, 128), port);

	m_pRemote -> Open(addr, port);
	m_pRemote -> SetDeleteByHandler();
	m_pRemote -> SetRemote(this);
	Handler().Add(m_pRemote);
	//
	m_state = STATE_TRY_CONNECT;
}


void Socks4Socket::SetupBind()
{
	ListenSocket<RemoteSocket> *p = new ListenSocket<RemoteSocket>(Handler());
	p -> SetDeleteByHandler();
	p -> Bind(0);
	Handler().Add(p);
	static_cast<Socks4Handler&>(Handler()).SetBind(p, this);

	char reply[8];
	reply[0] = 0;
	reply[1] = 90;
	port_t port = p -> GetPort();
	unsigned short s = htons(port);
	memcpy(reply + 2,&s,2);
	memset(reply + 4,0,4);
	SendBuf(reply, 8);
	//
	m_state = STATE_ACCEPT;
}


void Socks4Socket::Accept()
{
	if (m_state == STATE_ACCEPT)
	{
		ipaddr_t remote = m_pRemote -> GetRemoteIP4();
		ipaddr_t dstip = ntohl(m_dstip);
		char reply[8];
		reply[0] = 0;
		reply[1] = (dstip == remote) ? 90 : 91;
		memcpy(reply + 2, &m_dstport, 2);
		memcpy(reply + 4, &m_dstip, 4);
		SendBuf(reply, 8);
		m_state = (dstip == remote) ? STATE_CONNECTED : STATE_FAILED;
		if (m_state == STATE_FAILED)
		{
			Handler().LogError(this, "Accept", remote, "Remote ip mismatch", LOG_LEVEL_FATAL);
			SetCloseAndDelete();
		}
	}
	else
	{
		Handler().LogError(this, "Accept", m_state, "bad state", LOG_LEVEL_FATAL);
		SetCloseAndDelete();
	}
}


void Socks4Socket::Socks5Read()
{
/*
		STATE5_VER = 0,
		STATE5_NMETHODS,
		STATE5_METHODS,
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
		STATE5_DST_PORT,
*/
	bool need_more = false;
	while (ibuf.GetLength() && !need_more && !CloseAndDelete() )
	{
		size_t l = ibuf.GetLength();
		switch (m_state5)
		{
		case STATE5_VER: // always done by Socks4Read
			break;
		case STATE5_NMETHODS:
			ibuf.Read( (char *)&m_s5_nmethods, 1);
			m_state5 = STATE5_METHODS;
			m_s5_method = 0;
			memset(m_s5_methods, 0, sizeof(m_s5_methods));
			break;
		case STATE5_METHODS:
			while (ibuf.GetLength() && m_s5_method < m_s5_nmethods)
			{
				unsigned char c;
				ibuf.Read( (char *)&c, 1);
				m_s5_methods[c] = 1;
				m_s5_method++;
			}
			if (m_s5_method == m_s5_nmethods)
			{
				char reply[2];
				// select method
				if (m_s5_methods[2]) // username/password
				{
					reply[0] = 5; // 5?
					reply[1] = 2;
					SendBuf(reply, 2);
					m_state5 = STATE5_USERPASSWORD_VER;
				}
				else
				if (m_s5_methods[0]) // no auth
				{
					reply[0] = 5; // 5?
					reply[1] = 0;
					SendBuf(reply, 2);
					m_state5 = STATE5_VER2;
				}
				else
				{
					reply[0] = 5; // 5?
					reply[1] = (char)255;
					SendBuf(reply, 2);
					m_state5 = STATE5_FAILED;
				}
			}
			else
			{
				need_more = true;
			}
			break;
		case STATE5_FAILED:
			break;
		case STATE5_GSSAPI: // not implemented
			break;
		case STATE5_USERPASSWORD_VER:
			ibuf.Read( &m_s5_userpassword_ver, 1);
			if (m_s5_userpassword_ver == 1)
				m_state5 = STATE5_USERPASSWORD_ULEN;
			else
			{
				Handler().LogError(this, "Socks5", m_s5_userpassword_ver, "Username/Password version number != 1", LOG_LEVEL_FATAL);
				m_state5 = STATE5_FAILED;
				SetCloseAndDelete();
			}
			break;
		case STATE5_USERPASSWORD_ULEN:
			ibuf.Read( (char *)&m_s5_userpassword_ulen, 1);
			m_state5 = STATE5_USERPASSWORD_UNAME;
			m_s5_uptr = 0;
			break;
		case STATE5_USERPASSWORD_UNAME:
			while (ibuf.GetLength() && m_s5_uptr < m_s5_userpassword_ulen)
			{
				ibuf.Read( &m_s5_userpassword_uname[m_s5_uptr], 1);
				m_s5_uptr++;
			}
			if (m_s5_uptr == m_s5_userpassword_ulen)
			{
				m_s5_userpassword_uname[m_s5_uptr] = 0;
				m_state5 = STATE5_USERPASSWORD_PLEN;
			}
			else
			{
				need_more = true;
			}
			break;
		case STATE5_USERPASSWORD_PLEN:
			ibuf.Read( (char *)&m_s5_userpassword_plen, 1);
			m_state5 = STATE5_USERPASSWORD_PASSWD;
			m_s5_pptr = 0;
			break;
		case STATE5_USERPASSWORD_PASSWD:
			while (ibuf.GetLength() && m_s5_pptr < m_s5_userpassword_plen)
			{
				ibuf.Read( &m_s5_userpassword_passwd[m_s5_pptr], 1);
				m_s5_pptr++;
			}
			if (m_s5_pptr == m_s5_userpassword_plen)
			{
				m_s5_userpassword_passwd[m_s5_pptr] = 0;
				// %! verify username/password
				m_state5 = STATE5_VER2;
			}
			else
			{
				need_more = true;
			}
			break;
		case STATE5_VER2:
			ibuf.Read(&m_s5_ver2, 1);
			if (m_s5_ver2 == 5)
				m_state5 = STATE5_CMD;
			else
			{
				Handler().LogError(this, "Socks5", m_s5_ver2, "Version != 5", LOG_LEVEL_FATAL);
				m_state5 = STATE5_FAILED;
				SetCloseAndDelete();
			}
			break;
		case STATE5_CMD:
			ibuf.Read(&m_s5_cmd, 1);
			if (m_s5_cmd == 1 || m_s5_cmd == 2 || m_s5_cmd == 3)
				m_state5 = STATE5_RSV;
			else
			{
				Handler().LogError(this, "Socks5", m_s5_cmd, "Bad command code", LOG_LEVEL_FATAL);
				m_state5 = STATE5_FAILED;
				SetCloseAndDelete();
			}
			break;
		case STATE5_RSV:
			m_state5 = STATE5_ATYP;
			break;
		case STATE5_ATYP:
			ibuf.Read(&m_s5_atyp, 1);
			switch (m_s5_atyp)
			{
			case 1: // IPv4
				m_state5 = STATE5_DST_ADDR;
				m_s5_alen = 4;
				break;
			case 3: // domain name
				m_state5 = STATE5_ALEN;
				break;
			case 4: // IPv6
				m_state5 = STATE5_DST_ADDR;
				m_s5_alen = 6;
				break;
			default:
				Handler().LogError(this, "Socks5", m_s5_atyp, "Bad address type", LOG_LEVEL_FATAL);
				m_state5 = STATE5_FAILED;
				SetCloseAndDelete();
			}
			m_s5_aptr = 0;
			break;
		case STATE5_ALEN:
			ibuf.Read( (char *)&m_s5_alen,1);
			m_state5 = STATE5_DST_ADDR;
			break;
		case STATE5_DST_ADDR:
			while (ibuf.GetLength() && m_s5_aptr < m_s5_alen)
			{
				ibuf.Read( (char *)&m_s5_dst_addr[m_s5_aptr], 1);
				m_s5_aptr++;
			}
			if (m_s5_aptr == m_s5_alen)
			{
				m_s5_dst_addr[m_s5_aptr] = 0;
				m_state5 = STATE5_DST_PORT;
			}
			else
			{
				need_more = true;
			}
			break;
		case STATE5_DST_PORT:
			if (l > 1)
			{
				ibuf.Read( (char *)&m_s5_dst_port, 2);
				DoSocks5();
			}
			else
			{
				need_more = true;
			}
			break;
		}
	}
}


void Socks4Socket::DoSocks5()
{
	switch (m_s5_cmd)
	{
	case 1: // CONNECT
		break;
	case 2: // BIND
		break;
	case 3: // UDP ASSOCIATE
		break;
	}
}


