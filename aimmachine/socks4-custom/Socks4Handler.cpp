/**
 **	File ......... Socks4Handler.cpp
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

#include "Socks4Socket.h"
#include "Socks4Handler.h"




Socks4Handler::Socks4Handler(StdLog *p)
:SocketHandler(p)
{
}


Socks4Handler::~Socks4Handler()
{
}


void Socks4Handler::SetBind(Socket *listen,Socket *socks4)
{
	m_bind[listen] = socks4;
}


Socket *Socks4Handler::GetSocks4(Socket *listen)
{
	for (std::map<Socket *,Socket *>::iterator it = m_bind.begin(); it != m_bind.end(); it++)
	{
		Socket *p = (*it).first; // listen
		if (p == listen)
		{
			return (*it).second;
		}
	}
	return NULL;
}


