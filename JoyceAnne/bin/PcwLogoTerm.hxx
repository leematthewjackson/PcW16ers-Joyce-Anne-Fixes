/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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


class PcwLogoTerm : public PcwSdlTerm
{
public:
	PcwLogoTerm(PcwSystem *s);
	virtual ~PcwLogoTerm();

protected:
	Sint16 m_splitSz;
	Uint16 m_gfxY;

	void doSplit(void);
	int makerop(Uint8 pen, Uint8 *colour);

	void lios0(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios4(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios5(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios12(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios17(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios18(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios19(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios20(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios21(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios22(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios23(void) {}	/* Possible no-ops, functions implemented in Z80 */

	void zstrcpy(zword dest, char *src);
	zword lios_init(zword liospb, zword msgbuf);
	void lios1(void);
	void lios2(void);
	void lios3(zbyte split);

	void inline lios6(zbyte ch) { if (m_splitSz) (*m_term) << ch; }
	void lios7(zword T, zword B);	/* Clear text lines T to B */
	void lios8(zword X, zword Y);	/* Move text cursor to X,Y */
	void lios9(zword T, zword B);	/* Scroll text window */
	zword lios10(zword X, zword Y, zbyte colour, zbyte pen);
	zword lios11(zword X1, zword Y1, zword X2, zword Y2, zbyte colour, zbyte pen);
	zword lios13(zword colour, zword pixels);
	void lios14(void) { m_term->bel(); }
	zword lios15(void);
	zword  lios16(zword X, zword Y);
	zword lios24(zword buf, zword recno);
	zword lios25(zword buf, zword recno);

public:
	void drLogo(Z80 *R);

private:
	zbyte m_hdr1[54];	/* BMP header */
	zbyte m_palette[1024];	/* BMP palette */

	long m_hdrlen, m_hdrused; /* hdrlen: Length of info area */
	long m_pallen, m_palused, m_palbegin, m_palend;
	long m_piclen, m_pich, m_picw;

	int isbmp(void);
	int is8bit(void);

	void get_hdr1(void); 
	void get_palette(void);
	zbyte colmap[256];	// Mappings of BMP colours to JOYCE colours
	void put_palette(void);
	zword get_bmp_record(zword Z80addr, zword recno);
	zword put_bmp_record(zword Z80addr, zword recno);
};

