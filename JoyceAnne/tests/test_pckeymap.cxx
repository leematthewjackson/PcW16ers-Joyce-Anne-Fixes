/* Regression tests for the SDL key -> PS/2 scancode mapping used by the
 * PCW16 keyboard in ANNE (bin/PcKeyMap.hxx).
 *
 * Build and run with "make check" in this directory. Needs only the SDL 1.2
 * headers (no SDL library, no emulator build). */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../bin/PcKeyMap.hxx"

static int failures = 0, checks = 0;

/* Expect "keysym" to produce exactly the bytes given (count, then bytes) */
static void expect(const char *name, SDLKey keysym, bool down, bool numlock,
		   int count, ...)
{
	unsigned char got[8];
	int want[8];
	int len, n;
	bool ok;
	va_list ap;

	va_start(ap, count);
	for (n = 0; n < count; n++) want[n] = va_arg(ap, int);
	va_end(ap);

	++checks;
	len = pcKeyScancodes(keysym, down, numlock, got);
	ok = (len == count);
	for (n = 0; ok && n < len; n++) if (got[n] != want[n]) ok = false;
	if (ok) return;

	++failures;
	printf("FAIL: %s %s%s: expected", name, down ? "make" : "break",
		numlock ? " (NumLock)" : "");
	for (n = 0; n < count; n++) printf(" %02X", want[n]);
	printf(", got");
	if (len < 0) printf(" <unmapped>");
	for (n = 0; n < len; n++) printf(" %02X", got[n]);
	printf("\n");
}

/* Make + break for a key with a one-byte code */
static void expect1(const char *name, SDLKey k, int code, bool numlock = false)
{
	expect(name, k, true,  numlock, 1, code);
	expect(name, k, false, numlock, 2, 0xF0, code);
}

/* Make + break for an E0-prefixed key */
static void expectE0(const char *name, SDLKey k, int code, bool numlock = false)
{
	expect(name, k, true,  numlock, 2, 0xE0, code);
	expect(name, k, false, numlock, 3, 0xE0, 0xF0, code);
}

int main(void)
{
	/* Navigation block: previously unmapped */
	expectE0("Insert",    SDLK_INSERT,   0x70);
	expectE0("Delete",    SDLK_DELETE,   0x71);
	expectE0("Home",      SDLK_HOME,     0x6C);
	expectE0("End",       SDLK_END,      0x69);
	expectE0("Page Up",   SDLK_PAGEUP,   0x7D);
	expectE0("Page Down", SDLK_PAGEDOWN, 0x7A);

	/* Keypad with NumLock off: same codes as the navigation keys */
	expectE0("KP0/Ins",   SDLK_KP0, 0x70);
	expectE0("KP1/End",   SDLK_KP1, 0x69);
	expectE0("KP2/Down",  SDLK_KP2, 0x72);
	expectE0("KP3/PgDn",  SDLK_KP3, 0x7A);
	expectE0("KP4/Left",  SDLK_KP4, 0x6B);
	expect1 ("KP5",       SDLK_KP5, 0x73);
	expectE0("KP6/Right", SDLK_KP6, 0x74);
	expectE0("KP7/Home",  SDLK_KP7, 0x6C);
	expectE0("KP8/Up",    SDLK_KP8, 0x75);
	expectE0("KP9/PgUp",  SDLK_KP9, 0x7D);
	expectE0("KP./Del",   SDLK_KP_PERIOD, 0x71);

	/* Keypad with NumLock on: main-keyboard digits and '.' */
	expect1("KP0", SDLK_KP0, 0x45, true);
	expect1("KP1", SDLK_KP1, 0x16, true);
	expect1("KP2", SDLK_KP2, 0x1E, true);
	expect1("KP3", SDLK_KP3, 0x26, true);
	expect1("KP4", SDLK_KP4, 0x25, true);
	expect1("KP5", SDLK_KP5, 0x2E, true);
	expect1("KP6", SDLK_KP6, 0x36, true);
	expect1("KP7", SDLK_KP7, 0x3D, true);
	expect1("KP8", SDLK_KP8, 0x3E, true);
	expect1("KP9", SDLK_KP9, 0x46, true);
	expect1("KP.", SDLK_KP_PERIOD, 0x49, true);

	/* Keypad keys that ignore NumLock */
	expect1("KP Enter", SDLK_KP_ENTER,    0x5A);
	expect1("KP Enter", SDLK_KP_ENTER,    0x5A, true);
	expect1("KP -",     SDLK_KP_MINUS,    0x4E);
	expect1("KP /",     SDLK_KP_DIVIDE,   0x4A);
	expect1("KP +",     SDLK_KP_PLUS,     0x79);
	expect1("KP *",     SDLK_KP_MULTIPLY, 0x7C);

	/* Keypad NumLock-on codes must match the main keyboard exactly */
	{
		static const SDLKey kp[]   = { SDLK_KP0, SDLK_KP1, SDLK_KP2,
			SDLK_KP3, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7,
			SDLK_KP8, SDLK_KP9, SDLK_KP_PERIOD };
		static const SDLKey main_[] = { SDLK_0, SDLK_1, SDLK_2,
			SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
			SDLK_8, SDLK_9, SDLK_PERIOD };
		unsigned char a[8], b[8];
		int la, lb, n, d;

		for (n = 0; n < 11; n++) for (d = 0; d < 2; d++)
		{
			++checks;
			la = pcKeyScancodes(kp[n],    d, true, a);
			lb = pcKeyScancodes(main_[n], d, true, b);
			if (la != lb || la < 1 || memcmp(a, b, la))
			{
				++failures;
				printf("FAIL: keypad key %d (%s) differs from main key\n", n, d ? "make" : "break");
			}
		}
	}

	/* Existing keys must be unchanged */
	expect1 ("Escape",    SDLK_ESCAPE,    0x76);
	expect1 ("F1",        SDLK_F1,        0x05);
	expect1 ("F10",       SDLK_F10,       0x09);
	expect1 ("1",         SDLK_1,         0x16);
	expect1 ("0",         SDLK_0,         0x45);
	expect1 ("a",         SDLK_a,         0x1C);
	expect1 ("z",         SDLK_z,         0x1A);
	expect1 ("m",         SDLK_m,         0x3A);
	expect1 ("Space",     SDLK_SPACE,     0x29);
	expect1 ("Return",    SDLK_RETURN,    0x5A);
	expect1 ("Backspace", SDLK_BACKSPACE, 0x66);
	expect1 ("LShift",    SDLK_LSHIFT,    0x12);
	expect1 ("LCtrl",     SDLK_LCTRL,     0x14);
	expect1 ("LAlt",      SDLK_LALT,      0x11);
	expectE0("RCtrl",     SDLK_RCTRL,     0x14);
	expectE0("RAlt",      SDLK_RALT,      0x11);
	expectE0("Up",        SDLK_UP,        0x75);
	expectE0("Down",      SDLK_DOWN,      0x72);
	expectE0("Left",      SDLK_LEFT,      0x6B);
	expectE0("Right",     SDLK_RIGHT,     0x74);
	/* NumLock must not affect ordinary keys */
	expect1 ("a",         SDLK_a,         0x1C, true);
	expectE0("Up",        SDLK_UP,        0x75, true);
	expectE0("Home",      SDLK_HOME,      0x6C, true);

	/* Typos fixed in the original table: these released the wrong key
	 * (Q sent the break code for '1', Tab the one for '`') */
	expect1 ("q",         SDLK_q,         0x15);
	expect1 ("Tab",       SDLK_TAB,       0x0D);

	/* Multi-byte special cases */
	expect("PrtSc", SDLK_PRINT, true, false, 4, 0xE0, 0x12, 0xE0, 0x7C);
	expect("Pause", SDLK_BREAK, true, false, 8,
		0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77);
	/* PrtSc / Pause release used to send 0F FF FF FE / 0F FF FF F8 */
	expect("PrtSc", SDLK_PRINT, false, false, 6,
		0xE0, 0xF0, 0x12, 0xE0, 0xF0, 0x7C);
	expect("Pause", SDLK_BREAK, false, false, 0);

	/* Keys with no PC equivalent stay unmapped */
	{
		unsigned char c[8];
		++checks;
		if (pcKeyScancodes(SDLK_MENU, true, false, c) != -1)
		{
			++failures;
			printf("FAIL: Menu key should be unmapped\n");
		}
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
