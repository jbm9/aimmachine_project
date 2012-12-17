/**
 **	File ......... socks4.cpp
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
#include <StdoutLog.h>
#include <ListenSocket.h>
#include "Socks4Handler.h"
#include "Socks4Socket.h"


int main(int argc,char *argv[])
{
	StdoutLog log;
	Socks4Handler h(&log);
	ListenSocket<Socks4Socket> l(h);
	if (l.Bind(1080) == -1)
	{
		exit(-1);
	}
	h.Add(&l);
	h.Select(1,0);
	while (h.GetCount())
	{
		h.Select(1,0);
	}
}
