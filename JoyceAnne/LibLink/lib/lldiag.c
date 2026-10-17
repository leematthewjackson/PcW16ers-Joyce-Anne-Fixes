/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002, 2023  John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ll.h"

const char *sigs[] =
{
	"STROBE",
	"AUFEED",
	"OSELECT",
	"INIT",
	"",
	"",
	"",
	"",
	"BUSY",
	"ACK",
	"NOPAPER",
	"ISELECT",
	"ERROR",
	NULL
};

void ll_dump_signals(char *s, unsigned ctl)
{
	unsigned mask = 1;
	int n, l;

	s[0] = 0;
	for (n = 0; sigs[n]; n++, mask = mask << 1)
	{
		if (sigs[n][0] == 0) continue;

		l = strlen(sigs[n]) + 1;
		if (ctl & mask) 
		{
			sprintf(s + strlen(s), "%-*.*s", l, l, sigs[n]);
		}
		else
		{
			sprintf(s + strlen(s), "%-*.*s", l, l, "-");
		}
	}
}
