/************************************************************************

    JOYCE v2.5.2 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2024 John Elliott <seasip.webmaster@gmail.com>

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

/* A generalised PCW device that uses LibLink for I/O */

class PcwLibLink : public PcwDevice
{
public:
	PcwLibLink(const string devname, const string portname);
	virtual ~PcwLibLink();

	virtual UiEvent settings(UiDrawer *d);

	virtual void libLinkTick(void);

protected:
	UiEvent getFile(UiDrawer *d);
	UiEvent getPipe(UiDrawer *d);
	UiEvent getPort(UiDrawer *d);
	UiEvent getSharedMem(UiDrawer *d);
	UiEvent getSocket(UiDrawer *d);
	UiEvent getLocoLink(UiDrawer *d);

        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	virtual bool open(void);
	virtual void close(void);
	virtual void reset(void);
	virtual string getTitle() = 0;

	void showError(ll_error_t e);

// LibLink variables
	LL_PDEV m_lldev;
	bool	m_llwarned;	// Have we warned the user about errors?
	string	m_llfilename;	// LibLink filename
	string	m_lldriver;	// LibLink driver
};


string displaySocket (const string filename, int len);
string displayShm    (const string filename, int len);



