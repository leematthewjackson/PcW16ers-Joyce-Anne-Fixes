/************************************************************************

    JOYCE v2.5.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2022  John Elliott <seasip.webmaster@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*************************************************************************/

#include "Pcw.hxx"
#include "inline.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

extern int jfastcrt_doinit(void);

#define SUBSYS_CPM2  "CPM2    "
#define SUBSYS_CPM86 "CPM86   "

static const char *all_subsys =
        SUBSYS_CPM2
	SUBSYS_CPM86
	"$";

GuestSystem::GuestSystem()
{
	m_guest = NULL;	
	m_thread = NULL;
	m_bufadd = 0;
	m_bufend = 0;
}

static Uint32 callback(Uint32 interval, void *param)
{
	PcwGuest *self = (PcwGuest *)param;

	return self->on_timer();
}

PcwGuest::PcwGuest() : PcwDevice("guest")
{
	m_cpm2.m_guest = create_z80_guest(this, SUBSYS_CPM2);
	m_cpm2.m_name = "CP/M 2";
	m_cpm86.m_guest = create_i86_guest(this, SUBSYS_CPM86);
	m_cpm86.m_name = "CP/M-86";
	m_timer_id = 0;
}

PcwGuest::~PcwGuest()
{
	stop(SUBSYS_CPM2);
	stop(SUBSYS_CPM86);
	if (m_timer_id)
	{
		SDL_RemoveTimer(m_timer_id);
	}
}

bool PcwGuest::lookup(const char *name, GuestSystem **guest)
{
	GuestSystem *p = NULL;
	bool result = false;

	if (!strcmp(name, SUBSYS_CPM2))
	{
		p = &m_cpm2; result = true;
	}
	if (!strcmp(name, SUBSYS_CPM86))
	{
		p = &m_cpm86; result = true;
	}
	if (guest) *guest = p;
	return result;
}

void PcwGuest::reset()
{
	if (m_cpm2.m_thread)
	{
		m_cpm2.m_guest->reset();
	}
	if (m_cpm86.m_thread)
	{
		m_cpm86.m_guest->reset();
	}

}


/* Pass the 100Hz timer onto any guest operating systems */
Uint32 PcwGuest::on_timer()
{
	if (m_cpm2.m_thread)
	{
		m_cpm2.m_guest->on_timer();	
	}
	if (m_cpm86.m_thread)
	{
		m_cpm86.m_guest->on_timer();	
	}
	return 1;	
}

void PcwGuest::tick()
{

}

int PcwGuest::start(const char *id)
{
	string resourceDir1 = findPcwDir(FT_GUEST, true);
	string resourceDir2 = findPcwDir(FT_GUEST, false);
	if (!m_timer_id)
	{
		m_timer_id = SDL_AddTimer(10, callback, this);
		if (!m_timer_id)
		{
			joyce_dprintf("Failed to start 100Hz timer: %s\n",	
				SDL_GetError());
			return -1;
		}
	}
	if (!strcmp(id, SUBSYS_CPM2))
	{
		m_cpm2.m_guest->set_resourcedirs(resourceDir1.c_str(), resourceDir2.c_str());
		if (m_cpm2.m_guest->reset())
		{
			return -2;
		}
		if (!m_cpm2.m_thread)
		{
			m_cpm2.m_thread = SDL_CreateThread(z80_guest_main, m_cpm2.m_guest);
		}
		m_cpm2.m_guest->set_state(GS_Running);
		return 0;
	}

	if (!strcmp(id, SUBSYS_CPM86))
	{
		m_cpm86.m_guest->set_resourcedirs(resourceDir1.c_str(), resourceDir2.c_str());
		if (m_cpm86.m_guest->reset())
		{
			return -2;
		}
		if (!m_cpm86.m_thread)
		{
			m_cpm86.m_thread = SDL_CreateThread(i86_guest_main, m_cpm86.m_guest);
		}
		m_cpm86.m_guest->set_state(GS_Running);
		return 0;
	}
	return -3;
}


void PcwGuest::stop(const char * id)
{
	if (!strcmp(id, SUBSYS_CPM2))
	{
		if (m_cpm2.m_thread)
		{
			m_cpm2.m_guest->set_state(GS_Terminate);
			SDL_WaitThread(m_cpm2.m_thread, NULL);
			m_cpm2.m_thread = NULL;
		}
	}
	if (!strcmp(id, SUBSYS_CPM86))
	{
		if (m_cpm86.m_thread)
		{
			m_cpm86.m_guest->set_state(GS_Terminate);
			SDL_WaitThread(m_cpm86.m_thread, NULL);
			m_cpm86.m_thread = NULL;
		}
	}
}


void PcwGuest::service(Z80 *R)
{
	zword subsys;
	GuestSystem *guest = NULL;
	char subsys_name[10];
	const char *p;
	unsigned offset;

	switch (R->c)
	{
		case 0:	/* Init subsystem, HL->name */
			subsys = R->getHL();
			for (int n = 0; n < 8; n++) 
				subsys_name[n] = R->fetchB(subsys + n);
			subsys_name[8] = 0;
			if (lookup(subsys_name, &guest))
			{
/* Init the UC1 driver if it is not already so */
				if (!gl_sys->m_fastCRT->isInitialised())
				{
					jfastcrt_doinit();
				}

				guest->m_bufadd = R->ix;
				guest->m_bufend = R->iy;
				guest->m_guest->set_xferbuf(R->ix, R->iy);
				R->a = start(subsys_name);
			}
			break;
		case 1:	/* Terminate subsystem */
			subsys = R->getHL();
			for (int n = 0; n < 8; n++) 
				subsys_name[n] = R->fetchB(subsys + n);
			subsys_name[8] = 0;
			stop(subsys_name);
			break;
		case 2:	/* List subsystems */
			subsys = R->getHL();
			p = all_subsys;
			while (*p)
			{
				R->storeB(subsys, *p);
				++p;
				++subsys;
			}
			break;
		case 3:	/* Peek subsystem memory */
			subsys = R->getHL();
			for (int n = 0; n < 8; n++) 
				subsys_name[n] = R->fetchB(subsys + n);
			subsys_name[8] = 0;
			if (lookup(subsys_name, &guest))
			{
				offset = R->ix;
				offset = (offset << 16) | R->iy;
				R->e = guest->m_guest->peek(offset);
			}
			break;	
		case 4:	/* Poke subsystem memory */
			subsys = R->getHL();
			for (int n = 0; n < 8; n++) 
				subsys_name[n] = R->fetchB(subsys + n);
			subsys_name[8] = 0;
			if (lookup(subsys_name, &guest))
			{
				offset = R->ix;
				offset = (offset << 16) | R->iy;
				guest->m_guest->poke(offset, R->e);
			}
			break;	

	}
}

void PcwGuest::abort(Guest *caller, int err, const char *fmt, ...)
{
// XXX Log error
	caller->set_state(GS_Terminate);
}



bool PcwGuest::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next)
	{
		if (!strcmp((char *)cur->name, "system") && cur->ns == ns)
		{
			if (!parseSystemNode(doc, ns, cur)) return false;
		}
	}
	return true;
}


bool PcwGuest::parseDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
		const char *sysid)
{
	GuestSystem *guest;
	int n;
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"letter");
	string *str;
	if (!lookup(sysid, &guest))
	{
		alert("Internal error: system ID not recognised");
		return false;
	}

	if (s)
	{
		n = s[0] - 'a';
		if (n >= 0 && n < 16)
		{
			str = &guest->m_settings.drivepath[n];
			parseFilename(doc, ns, cur, "filename", *str);
		}
		xmlFree(s);
	}
	return true;
}	




bool PcwGuest::parseSystemNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	string xmlid;
	const char *p;
	char sysid[9];
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"type");
	xmlNodePtr drv;
	GuestSystem *guest;

	if (!s)
	{
		joyce_dprintf("<system> must have a type= field\n");
		return true;
	}
	for (p = all_subsys; p[0] != '$'; p += 8)
	{
		strncpy(sysid, p, 8); sysid[8] = 0;
		xmlid = get_xmlid(sysid);

		if (!strcmp(xmlid.c_str(), s))
		{
			if (!lookup(sysid, &guest))
			{
				alert("Internal error: system ID not recognised");
				return false;
			}

			for (drv = cur->xmlChildrenNode; drv != NULL; 
				drv = drv->next)
			{
				if (!strcmp((char *)drv->name, "drive") && 
					drv->ns == ns)
				{
					if (!parseDriveNode(doc, ns, drv, sysid)) return false;
				}	
			}
			guest->m_guest->refresh_drives(guest->m_settings);

		}
	}
	xmlFree(s);
	return true;
}	


string PcwGuest::get_xmlid(const char *sysid)
{
	string xmlid;

	while (*sysid)
	{
		if (*sysid == ' ' || *sysid == '\0') break;
		xmlid += (char)(tolower(*sysid));
		++sysid;
	}
	return xmlid;
}



bool PcwGuest::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	const char *p;
	char sysid[9];
	string xmlid;
	int n;	
	xmlNodePtr sys;
	bool some;

	for (p = all_subsys; p[0] != '$'; p += 8)
	{
		some = false;
		strncpy(sysid, p, 8); sysid[8] = 0;
		xmlid = get_xmlid(sysid);

		for (sys = cur->xmlChildrenNode; sys; sys = sys->next)
		{
			if (strcmp((char *)sys->name, "system")) continue;
			char *s = (char *)xmlGetProp(sys, (xmlChar *)"type");
			if (!s) continue;
			if (xmlid != s)
			{
				xmlFree(s);
				continue;
			}
			some = true;
			storeSys(doc, ns, sys, sysid);
			xmlFree(s);
		}
		if (!some)
		{
			sys = xmlNewNode(ns, (xmlChar *)"system");
			xmlSetProp(sys, (xmlChar *)"type", 
				(const xmlChar *)xmlid.c_str());
			xmlAddChild(cur, sys);
			storeSys(doc, ns, sys, sysid);
		}
	}
	return true;
}


bool PcwGuest::storeSys(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
			const char *sysid)
{
	int drv;
	string xmlid;
	int n;	
	xmlNodePtr dnode;
	bool some;
	GuestSystem *guest;
	char buf[3];

	if (!lookup(sysid, &guest))
	{
		return false;
	}

	for (drv = 0; drv < 16; drv++)
	{
		some = false;
		sprintf(buf, "%c", drv + 'a');
		for (dnode = cur->xmlChildrenNode; dnode; dnode = dnode->next)
		{
			if (strcmp((char *)dnode->name, "drive")) continue;
			char *s = (char *)xmlGetProp(dnode, (xmlChar *)"letter");
			if (!s) continue;
			if (strcmp(s, buf))
			{
				xmlFree(s);
				continue;
			}
			some = true;
			storeFilename(doc, ns, dnode, "filename", 
				guest->m_settings.drivepath[drv].c_str());
			xmlFree(s);
		}
		if (!some)
		{
			dnode = xmlNewNode(ns, (const xmlChar *)"drive");
			xmlSetProp(dnode, (xmlChar *)"letter", (const xmlChar *)buf);
			xmlAddChild(cur, dnode);
			storeFilename(doc, ns, dnode, "filename", 
				guest->m_settings.drivepath[drv].c_str());
		}
	}
	return true;
}




void PcwGuest::yield()
{
#ifdef HAVE_SCHED_YIELD
	sched_yield();
#else
	SDL_Delay(0);
#endif
}


bool PcwGuest::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_u;
	caption = "  Guest systems  ";
	return true;
}

UiEvent PcwGuest::emergencyStop(UiDrawer *d)
{
	const char *p;
	bool some = false;
	GuestSystem *g;
	string str;
	UiEvent uie;
	char sysid[9];

	UiMenu uim(d);
	uim.add(new UiLabel("  Guest systems  ", d));
	uim.add(new UiSeparator(d));
	for (p = all_subsys; p[0] != '$'; p += 8)
	{
		strncpy(sysid, p, 8); sysid[8] = 0;
		if (lookup(sysid, &g))
		{
			if (g->m_thread)
			{
				some = true;
				stop(sysid);
/* Replace the whole host program with RST 0 */
				for (int n = 0x100; n < g->m_bufadd; n++)
				{
					store_tpa(n, 0xC7);
				}	
				store_tpa(g->m_bufadd, 0);
			
				str = "  " + g->m_name + " - stopped.  ";
				uim.add(new UiLabel(str, d));
			}
		}
	}
	if (!some)
	{
		uim.add(new UiLabel("  No guest systems are active.  ", d));
	}
	uim.add(new UiSeparator(d));
	uim.add(new UiCommand(SDLK_o, "  OK  ", d));
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie == UIE_QUIT) return uie;
	return UIE_OK;
}



UiEvent PcwGuest::settings(UiDrawer *d)
{
	UiEvent uie;

	int n;
	int sel = -1;
	while (1)
	{
		UiMenu uim(d);
		uim.add(new UiLabel("  Guest systems  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_1, "  CP/M-86  ", d));
		uim.add(new UiCommand(SDLK_2, "  CP/M 2  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		switch(uim.getKey(sel = uim.getSelected()))
		{
			case SDLK_ESCAPE:
			return UIE_OK;
			case SDLK_1:
				uie = configureGuest(d, SUBSYS_CPM86);
				if (uie == UIE_QUIT) return uie;
				break;
			case SDLK_2:
				uie = configureGuest(d, SUBSYS_CPM2);
				if (uie == UIE_QUIT) return uie;
				break;
		}
	}
	return UIE_CONTINUE;
}



UiEvent PcwGuest::configureGuest(UiDrawer *d, const char *sysid)
{
	UiEvent uie;
	GuestSystem *guest;

	if (!lookup(sysid, &guest))
	{
		alert("Internal error: system ID not recognised");
		return UIE_CONTINUE;
	}

	int n;
	int sel = -1;
	while (1)
	{
		string title = "  Configure ";
		title += guest->m_name;
		title += "  ";
		string status, filename;
		UiMenu uim(d);
		uim.add(new UiLabel(title, d));
		uim.add(new UiSeparator(d));
		for (n = 0; n < 16; n++)
		{
			status = "  Drive ";
			status += (char)(n + 'A');
			status += ":  ";
			filename = guest->m_settings.drivepath[n];
			if (filename == "")
				status += "(host)";
			else	status += displayName(filename, 36);
			uim.add(new UiCommand(SDLKey(SDLK_a + n), status, d));
		}
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		SDLKey k = uim.getKey(sel = uim.getSelected());
		switch (k)
		{
			case SDLK_ESCAPE:
				guest->m_guest->refresh_drives(guest->m_settings);
				return UIE_OK;
			default:
				n = k - SDLK_a;
				uie = configureDrive(d, sysid, n);
				if (uie == UIE_QUIT) return uie;
		}
	}
	return UIE_CONTINUE;
}


UiEvent PcwGuest::configureDrive(UiDrawer *d, const char *sysid, int drive)
{
	int sel = -1;
	UiEvent uie;
	GuestSystem *guest;

	if (!lookup(sysid, &guest))
	{
		alert("Internal error: system ID not recognised");
		return UIE_CONTINUE;
	}
	string *filename = &guest->m_settings.drivepath[drive];
	int n;
	if (*filename == "") sel = 2; else sel = 3;
	while (1)
	{
		string title = "  Configure ";
		title += guest->m_name;
		title += "  Drive ";
		title += (char)(drive + 'A');
		title += ":  ";
		UiMenu uim(d);
		uim.add(new UiLabel(title, d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_h, "  Shared with host CP/M  ",d));
		uim.add(new UiCommand(SDLK_g, "  File: " + displayName(*filename, 40), d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		SDLKey k = uim.getKey(sel = uim.getSelected());
		switch (k)
		{
			case SDLK_ESCAPE:
				return UIE_CONTINUE;
			case SDLK_g:
			{
				string s = *filename;
				UiEvent e;
				PcwFileChooser f("  OK  ", d);

				f.initDir(s);
				e = f.eventLoop();
				if (e == UIE_QUIT) return e;
				if (e == UIE_OK)
				{
					*filename = f.getChoice();
				} 
				break;
			}
			case SDLK_h:
				*filename = "";
				break;

		}
	}
	return UIE_CONTINUE;
}
