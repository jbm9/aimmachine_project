/**
 **	File ......... Socks4Handler.h
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
#ifndef _SOCKS4HANDLER_H
#define _SOCKS4HANDLER_H

#include <SocketHandler.h>
#include <StdLog.h>

class Socks4Handler : public SocketHandler
{
public:
	Socks4Handler(StdLog *);
	~Socks4Handler();

	void SetBind(Socket *listen,Socket *socks4);
	Socket *GetSocks4(Socket *listen);

private:
	Socks4Handler(const Socks4Handler& ) {} // copy constructor
	Socks4Handler& operator=(const Socks4Handler& ) { return *this; } // assignment operator

	std::map<Socket *,Socket *> m_bind;
};




#endif // _SOCKS4HANDLER_H
