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

#include "guest.hxx"
#include <SDL_thread.h>

struct GuestSystem
{
	GuestSystem();

	string m_name;
	Guest *m_guest;
	GuestSettings m_settings;
	SDL_Thread *m_thread;
	unsigned short m_bufadd;
	unsigned short m_bufend;	
};

class PcwGuest : public PcwDevice, public Host
{
public:
	PcwGuest();
	virtual ~PcwGuest();

public:
	int start(const char* id);	/* Start the emulated guest CPU */
	void stop(const char* id);	/* Stop the emulated guest CPU */
        virtual void tick(void);
	virtual void reset(void);
	virtual void abort(Guest *caller, int err, const char *fmt, ...);
	virtual void yield(void);

	virtual void service(Z80 *R);
	bool hasSettings(SDLKey *key, string &caption);
	UiEvent settings(UiDrawer *d);	
	UiEvent configureGuest(UiDrawer *d, const char *sysid);
	UiEvent configureDrive(UiDrawer *d, const char *sysid, int drive);
	UiEvent emergencyStop(UiDrawer *d);

	Uint32 on_timer();
protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool parseSystemNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool parseDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, const char *sysid);
        virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool storeSys(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
			const char *sysid);

	bool lookup(const char *name, GuestSystem **pguest);

	void refreshDrives(const char *sysid);
private:
	string get_xmlid(const char *sysid);
	GuestSystem m_cpm86;
	GuestSystem m_cpm2;
	SDL_TimerID m_timer_id;

};





