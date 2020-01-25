// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "i_sdl.h" 
#include "i_input.h"
#include "i_sdlinput.h"

#include "i_video.h"
#include "d_event.h"
#include "doomkeys.h"
#include <queue>
#include <cassert>

//
// convUTF8ToUTF32
// 
// [SL] Based on GPL v2 code from ScummVM
//
/* ScummVM - Graphic Adventure Engine
*
* ScummVM is the legal property of its developers, whose names
* are too numerous to list here. Please refer to the COPYRIGHT
* file distributed with this source distribution.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/
static uint32_t convUTF8ToUTF32(const char *src)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	const char output_type[] = "UTF-32BE";
#else
	const char output_type[] = "UTF-32LE";
#endif 

	uint32_t utf32 = 0;
	char *dst = SDL_iconv_string(output_type, "UTF-8", src, SDL_strlen(src) + 1);
	if (dst) {
		utf32 = *((uint32_t *)dst);
		SDL_free(dst);
	}
	return utf32;
}


//
// I_BuildSDLKeyTranslationTable
//
// Creates a translation table to convert from SDL Key Codes
// to Odamex's internal key representation.
//
static KeyTranslationTable I_BuildSDLKeyTranslationTable()
{
	KeyTranslationTable key_table(128);

	key_table[SDLK_BACKSPACE]		= KEY_BACKSPACE;
	key_table[SDLK_TAB]				= KEY_TAB;
	key_table[SDLK_RETURN]			= KEY_ENTER;
	key_table[SDLK_PAUSE]			= KEY_PAUSE;
	key_table[SDLK_ESCAPE]			= KEY_ESCAPE;
	key_table[SDLK_SPACE]			= KEY_SPACE;
	key_table[SDLK_QUOTE]			= '\'';
	key_table[SDLK_MINUS]			= '-';
	key_table[SDLK_COMMA]			= ',';
	key_table[SDLK_PERIOD]			= '.';
	key_table[SDLK_SLASH]			= '/';
	key_table[SDLK_SEMICOLON]		= ';'; 
	key_table[SDLK_COLON]			= ':';
	key_table[SDLK_EXCLAIM]			= '!';
	key_table[SDLK_EQUALS]			= '='; 
	key_table[SDLK_DOLLAR]			= '$'; 
	key_table[SDLK_CARET]			= '^'; 
	key_table[SDLK_ASTERISK]		= '*'; 
	key_table[SDLK_LEFTBRACKET]		= '['; 
	key_table[SDLK_RIGHTBRACKET]	= ']'; 
	key_table[SDLK_RIGHTPAREN]		= '-'; 
	key_table[SDLK_BACKSLASH]		= '\\'; 
	key_table[SDLK_BACKQUOTE]		= '`'; 
	key_table[178]					= '`';		// "~" for AZERTY keyboards
	key_table[249]					= '[';		// AZERTY only, untranslatable for now.
	key_table[SDLK_0]				= '0'; 
	key_table[SDLK_1]				= '1'; 
	key_table[SDLK_2]				= '2'; 
	key_table[SDLK_3]				= '3'; 
	key_table[SDLK_4]				= '4';
	key_table[SDLK_5]				= '5'; 
	key_table[SDLK_6]				= '6'; 
	key_table[SDLK_7]				= '7'; 
	key_table[SDLK_8]				= '8'; 
	key_table[SDLK_9]				= '9'; 
	key_table[SDLK_a]				= 'a'; 
	key_table[SDLK_b]				= 'b'; 
	key_table[SDLK_c]				= 'c'; 
	key_table[SDLK_d]				= 'd'; 
	key_table[SDLK_e]				= 'e'; 
	key_table[SDLK_f]				= 'f'; 
	key_table[SDLK_g]				= 'g'; 
	key_table[SDLK_h]				= 'h'; 
	key_table[SDLK_i]				= 'i'; 
	key_table[SDLK_j]				= 'j'; 
	key_table[SDLK_k]				= 'k'; 
	key_table[SDLK_l]				= 'l'; 
	key_table[SDLK_m]				= 'm'; 
	key_table[SDLK_n]				= 'n'; 
	key_table[SDLK_o]				= 'o'; 
	key_table[SDLK_p]				= 'p'; 
	key_table[SDLK_q]				= 'q'; 
	key_table[SDLK_r]				= 'r'; 
	key_table[SDLK_s]				= 's'; 
	key_table[SDLK_t]				= 't'; 
	key_table[SDLK_u]				= 'u'; 
	key_table[SDLK_v]				= 'v'; 
	key_table[SDLK_w]				= 'w'; 
	key_table[SDLK_x]				= 'x'; 
	key_table[SDLK_y]				= 'y'; 
	key_table[SDLK_z]				= 'z'; 
#ifdef SDL12
	key_table[SDLK_KP0]				= KEYP_0; 
	key_table[SDLK_KP1]				= KEYP_1; 
	key_table[SDLK_KP2]				= KEYP_2; 
	key_table[SDLK_KP3]				= KEYP_3; 
	key_table[SDLK_KP4]				= KEYP_4; 
	key_table[SDLK_KP5]				= KEYP_5; 
	key_table[SDLK_KP6]				= KEYP_6; 
	key_table[SDLK_KP7]				= KEYP_7; 
	key_table[SDLK_KP8]				= KEYP_8; 
	key_table[SDLK_KP9]				= KEYP_9; 
#endif
#ifdef SDL20
	key_table[SDLK_KP_0]			= KEYP_0; 
	key_table[SDLK_KP_1]			= KEYP_1; 
	key_table[SDLK_KP_2]			= KEYP_2; 
	key_table[SDLK_KP_3]			= KEYP_3; 
	key_table[SDLK_KP_4]			= KEYP_4; 
	key_table[SDLK_KP_5]			= KEYP_5; 
	key_table[SDLK_KP_6]			= KEYP_6; 
	key_table[SDLK_KP_7]			= KEYP_7; 
	key_table[SDLK_KP_8]			= KEYP_8; 
	key_table[SDLK_KP_9]			= KEYP_9; 
#endif
	key_table[SDLK_KP_PERIOD]		= KEYP_PERIOD; 
	key_table[SDLK_KP_DIVIDE]		= KEYP_DIVIDE; 
	key_table[SDLK_KP_MULTIPLY]		= KEYP_MULTIPLY; 
	key_table[SDLK_KP_MINUS]		= KEYP_MINUS; 
	key_table[SDLK_KP_PLUS]			= KEYP_PLUS; 
	key_table[SDLK_KP_ENTER]		= KEYP_ENTER; 
	key_table[SDLK_KP_EQUALS]		= KEYP_EQUALS; 
	key_table[SDLK_UP]				= KEY_UPARROW; 
	key_table[SDLK_DOWN]			= KEY_DOWNARROW; 
	key_table[SDLK_RIGHT]			= KEY_RIGHTARROW; 
	key_table[SDLK_LEFT]			= KEY_LEFTARROW; 
	key_table[SDLK_INSERT]			= KEY_INS; 
	key_table[SDLK_DELETE]			= KEY_DEL; 
	key_table[SDLK_HOME]			= KEY_HOME; 
	key_table[SDLK_END]				= KEY_END; 
	key_table[SDLK_PAGEUP]			= KEY_PGUP; 
	key_table[SDLK_PAGEDOWN]		= KEY_PGDN; 
	key_table[SDLK_F1]				= KEY_F1; 
	key_table[SDLK_F2]				= KEY_F2; 
	key_table[SDLK_F3]				= KEY_F3; 
	key_table[SDLK_F4]				= KEY_F4; 
	key_table[SDLK_F5]				= KEY_F5; 
	key_table[SDLK_F6]				= KEY_F6; 
	key_table[SDLK_F7]				= KEY_F7; 
	key_table[SDLK_F8]				= KEY_F8; 
	key_table[SDLK_F9]				= KEY_F9; 
	key_table[SDLK_F10]				= KEY_F10; 
	key_table[SDLK_F11]				= KEY_F11; 
	key_table[SDLK_F12]				= KEY_F12;
	key_table[SDLK_F13]				= KEY_F13;
	key_table[SDLK_F14]				= KEY_F14;
	key_table[SDLK_F15]				= KEY_F15;
	key_table[SDLK_CAPSLOCK]		= KEY_CAPSLOCK; 
	key_table[SDLK_RSHIFT]			= KEY_RSHIFT; 
	key_table[SDLK_LSHIFT]			= KEY_LSHIFT; 
	key_table[SDLK_RCTRL]			= KEY_RCTRL; 
	key_table[SDLK_LCTRL]			= KEY_LCTRL; 
	key_table[SDLK_RALT]			= KEY_RALT; 
	key_table[SDLK_LALT]			= KEY_LALT;
	key_table[SDLK_HELP]			= KEY_HELP;
	key_table[SDLK_SYSREQ]			= KEY_SYSRQ;
#ifdef SDL12
	key_table[SDLK_SCROLLOCK]		= KEY_SCRLCK;
	key_table[SDLK_NUMLOCK] 		= KEY_NUMLOCK; 
	key_table[SDLK_LSUPER]			= KEY_LWIN;
	key_table[SDLK_RSUPER]			= KEY_RWIN;
	key_table[SDLK_PRINT]			= KEY_PRINT;
	key_table[SDLK_BREAK]			= KEY_BREAK;
#endif
#ifdef SDL20
	key_table[SDLK_SCROLLLOCK]		= KEY_SCRLCK;
	key_table[SDLK_NUMLOCKCLEAR] 	= KEY_NUMLOCK; 
	key_table[SDLK_LGUI]			= KEY_LWIN;
	key_table[SDLK_RGUI]			= KEY_RWIN;
	key_table[SDLK_PRINTSCREEN]		= KEY_PRINT;
#endif

	return key_table;
}


#ifdef SDL12

// ============================================================================
//
// ISDL12KeyboardInputDevice implementation
//
// ============================================================================

//
// ISDL12KeyboardInputDevice::ISDL12KeyboardInputDevice
//
ISDL12KeyboardInputDevice::ISDL12KeyboardInputDevice(int id) :
	mActive(false),
	mSDLKeyTransTable(I_BuildSDLKeyTranslationTable()),
	mSDLKeyTextTransTable(128), mShiftTransTable(128)
{
	initKeyTextTranslation();

	// enable keyboard input
	resume();
}


//
// ISDL12KeyboardInputDevice::~ISDL12KeyboardInputDevice
//
ISDL12KeyboardInputDevice::~ISDL12KeyboardInputDevice()
{
	pause();
}


//
// ISDL12KeyboardInputDevice::initKeyTextTranslation
//
// Initializes the SDL key to text translation table.
//
void ISDL12KeyboardInputDevice::initKeyTextTranslation()
{
	mSDLKeyTextTransTable[SDLK_BACKSPACE] = '\b';
	mSDLKeyTextTransTable[SDLK_TAB] = '\t'; 
	mSDLKeyTextTransTable[SDLK_RETURN] = '\r'; 
	mSDLKeyTextTransTable[SDLK_SPACE] = ' ';
	mSDLKeyTextTransTable[SDLK_EXCLAIM]  = '!';
	mSDLKeyTextTransTable[SDLK_QUOTEDBL]  = '\"';
	mSDLKeyTextTransTable[SDLK_HASH]  = '#';
	mSDLKeyTextTransTable[SDLK_DOLLAR]  = '$';
	mSDLKeyTextTransTable[SDLK_AMPERSAND]  = '&';
	mSDLKeyTextTransTable[SDLK_QUOTE] = '\'';
	mSDLKeyTextTransTable[SDLK_LEFTPAREN] = '(';
	mSDLKeyTextTransTable[SDLK_RIGHTPAREN] = ')';
	mSDLKeyTextTransTable[SDLK_ASTERISK] = '*';
	mSDLKeyTextTransTable[SDLK_PLUS] = '+';
	mSDLKeyTextTransTable[SDLK_COMMA] = ',';
	mSDLKeyTextTransTable[SDLK_MINUS] = '-';
	mSDLKeyTextTransTable[SDLK_PERIOD] = '.';
	mSDLKeyTextTransTable[SDLK_SLASH] = '/';
	mSDLKeyTextTransTable[SDLK_0] = '0';
	mSDLKeyTextTransTable[SDLK_1] = '1';
	mSDLKeyTextTransTable[SDLK_2] = '2';
	mSDLKeyTextTransTable[SDLK_3] = '3';
	mSDLKeyTextTransTable[SDLK_4] = '4';
	mSDLKeyTextTransTable[SDLK_5] = '5';
	mSDLKeyTextTransTable[SDLK_6] = '6';
	mSDLKeyTextTransTable[SDLK_7] = '7';
	mSDLKeyTextTransTable[SDLK_8] = '8';
	mSDLKeyTextTransTable[SDLK_9] = '9';
	mSDLKeyTextTransTable[SDLK_COLON] = ':';
	mSDLKeyTextTransTable[SDLK_SEMICOLON] = ';';
	mSDLKeyTextTransTable[SDLK_LESS] = '<';
	mSDLKeyTextTransTable[SDLK_EQUALS] = '=';
	mSDLKeyTextTransTable[SDLK_GREATER] = '>';
	mSDLKeyTextTransTable[SDLK_QUESTION] = '?';
	mSDLKeyTextTransTable[SDLK_AT] = '@';
	mSDLKeyTextTransTable[SDLK_LEFTBRACKET] = '[';
	mSDLKeyTextTransTable[SDLK_BACKSLASH] = '\\';
	mSDLKeyTextTransTable[SDLK_RIGHTBRACKET] = ']';
	mSDLKeyTextTransTable[SDLK_CARET] = '^';
	mSDLKeyTextTransTable[SDLK_UNDERSCORE] = '_';
	mSDLKeyTextTransTable[SDLK_BACKQUOTE] = '`';
	mSDLKeyTextTransTable[SDLK_a] = 'a';
	mSDLKeyTextTransTable[SDLK_b] = 'b';
	mSDLKeyTextTransTable[SDLK_c] = 'c';
	mSDLKeyTextTransTable[SDLK_d] = 'd';
	mSDLKeyTextTransTable[SDLK_e] = 'e';
	mSDLKeyTextTransTable[SDLK_f] = 'f';
	mSDLKeyTextTransTable[SDLK_g] = 'g';
	mSDLKeyTextTransTable[SDLK_h] = 'h';
	mSDLKeyTextTransTable[SDLK_i] = 'i';
	mSDLKeyTextTransTable[SDLK_j] = 'j';
	mSDLKeyTextTransTable[SDLK_k] = 'k';
	mSDLKeyTextTransTable[SDLK_l] = 'l';
	mSDLKeyTextTransTable[SDLK_m] = 'm';
	mSDLKeyTextTransTable[SDLK_n] = 'n';
	mSDLKeyTextTransTable[SDLK_o] = 'o';
	mSDLKeyTextTransTable[SDLK_p] = 'p';
	mSDLKeyTextTransTable[SDLK_q] = 'q';
	mSDLKeyTextTransTable[SDLK_r] = 'r';
	mSDLKeyTextTransTable[SDLK_s] = 's';
	mSDLKeyTextTransTable[SDLK_t] = 't';
	mSDLKeyTextTransTable[SDLK_u] = 'u';
	mSDLKeyTextTransTable[SDLK_v] = 'v';
	mSDLKeyTextTransTable[SDLK_w] = 'w';
	mSDLKeyTextTransTable[SDLK_x] = 'x';
	mSDLKeyTextTransTable[SDLK_y] = 'y';
	mSDLKeyTextTransTable[SDLK_z] = 'z';
	mSDLKeyTextTransTable[SDLK_KP0] = '0';
	mSDLKeyTextTransTable[SDLK_KP1] = '1';
	mSDLKeyTextTransTable[SDLK_KP2] = '2';
	mSDLKeyTextTransTable[SDLK_KP3] = '3';
	mSDLKeyTextTransTable[SDLK_KP4] = '4';
	mSDLKeyTextTransTable[SDLK_KP5] = '5';
	mSDLKeyTextTransTable[SDLK_KP6] = '6';
	mSDLKeyTextTransTable[SDLK_KP7] = '7';
	mSDLKeyTextTransTable[SDLK_KP8] = '8';
	mSDLKeyTextTransTable[SDLK_KP9] = '9';
	mSDLKeyTextTransTable[SDLK_KP_PERIOD] = '.';
	mSDLKeyTextTransTable[SDLK_KP_DIVIDE] = '/';
	mSDLKeyTextTransTable[SDLK_KP_MULTIPLY] = '*';
	mSDLKeyTextTransTable[SDLK_KP_MINUS] = '-';
	mSDLKeyTextTransTable[SDLK_KP_PLUS] = '+';
	mSDLKeyTextTransTable[SDLK_KP_ENTER] = '\r';
	mSDLKeyTextTransTable[SDLK_KP_EQUALS] = '=';

	// initialize the shift key translation table
	mShiftTransTable['a'] = 'A';
	mShiftTransTable['b'] = 'B';
	mShiftTransTable['c'] = 'C';
	mShiftTransTable['d'] = 'D';
	mShiftTransTable['e'] = 'E';
	mShiftTransTable['f'] = 'F';
	mShiftTransTable['g'] = 'G';
	mShiftTransTable['h'] = 'H';
	mShiftTransTable['i'] = 'I';
	mShiftTransTable['j'] = 'J';
	mShiftTransTable['k'] = 'K';
	mShiftTransTable['l'] = 'L';
	mShiftTransTable['m'] = 'M';
	mShiftTransTable['n'] = 'N';
	mShiftTransTable['o'] = 'O';
	mShiftTransTable['p'] = 'P';
	mShiftTransTable['q'] = 'Q';
	mShiftTransTable['r'] = 'R';
	mShiftTransTable['s'] = 'S';
	mShiftTransTable['t'] = 'T';
	mShiftTransTable['u'] = 'U';
	mShiftTransTable['v'] = 'V';
	mShiftTransTable['w'] = 'W';
	mShiftTransTable['x'] = 'X';
	mShiftTransTable['y'] = 'Y';
	mShiftTransTable['z'] = 'Z';
	mShiftTransTable['A'] = 'a';
	mShiftTransTable['B'] = 'b';
	mShiftTransTable['C'] = 'c';
	mShiftTransTable['D'] = 'd';
	mShiftTransTable['E'] = 'e';
	mShiftTransTable['F'] = 'f';
	mShiftTransTable['G'] = 'g';
	mShiftTransTable['H'] = 'h';
	mShiftTransTable['I'] = 'i';
	mShiftTransTable['J'] = 'j';
	mShiftTransTable['K'] = 'k';
	mShiftTransTable['L'] = 'l';
	mShiftTransTable['M'] = 'm';
	mShiftTransTable['N'] = 'n';
	mShiftTransTable['O'] = 'o';
	mShiftTransTable['P'] = 'p';
	mShiftTransTable['Q'] = 'q';
	mShiftTransTable['R'] = 'r';
	mShiftTransTable['S'] = 's';
	mShiftTransTable['T'] = 't';
	mShiftTransTable['U'] = 'u';
	mShiftTransTable['V'] = 'v';
	mShiftTransTable['W'] = 'w';
	mShiftTransTable['X'] = 'x';
	mShiftTransTable['Y'] = 'y';
	mShiftTransTable['Z'] = 'z';
	mShiftTransTable['1'] = '!';
	mShiftTransTable['2'] = '@';
	mShiftTransTable['3'] = '#';
	mShiftTransTable['4'] = '$';
	mShiftTransTable['5'] = '%';
	mShiftTransTable['6'] = '^';
	mShiftTransTable['7'] = '&';
	mShiftTransTable['8'] = '*';
	mShiftTransTable['9'] = '(';
	mShiftTransTable['0'] = ')';
	mShiftTransTable['`'] = '~';
	mShiftTransTable['-'] = '_';
	mShiftTransTable['='] = '+';
	mShiftTransTable['['] = '{';
	mShiftTransTable[']'] = '}';
	mShiftTransTable['\\'] = '|';
	mShiftTransTable[';'] = ':';
	mShiftTransTable['\''] = '\"';
	mShiftTransTable[','] = '<';
	mShiftTransTable['.'] = '>';
	mShiftTransTable['/'] = '?';
}


//
// ISDL12KeyboardInputDevice::translateKey
//	
// Convert the SDL KeySym to an Odamex key using the mSDLKeyTransTable mapping.
// Returns 0 if the KeySym is unknown.
//
int ISDL12KeyboardInputDevice::translateKey(int sym)
{
	KeyTranslationTable::const_iterator key_it = mSDLKeyTransTable.find(sym);
	if (key_it != mSDLKeyTransTable.end())
		return key_it->second;
	return 0;
}


//
// ISDL12KeyboardInputDevice::translateKeyText
//	
// Convert the SDL KeySym to a text character using the mSDLKeyTextTransTable
// mapping and the mShiftTransTable mapping to handle shift keys.
// Returns 0 if the KeySym is unknown or if it is non-printable.
//
int ISDL12KeyboardInputDevice::translateKeyText(int sym, int mod)
{
	KeyTranslationTable::const_iterator text_it = mSDLKeyTextTransTable.find(sym);	
	if (text_it != mSDLKeyTextTransTable.end())
	{
		int c = text_it->second;

		// handle CAPS LOCK and translate 'a'-'z' to 'A'-'Z'
		if (c >= 'a' && c <= 'z' && (mod & KMOD_CAPS))
			c = mShiftTransTable[c];

		// Handle SHIFT keys
		if (mod & (KMOD_LSHIFT | KMOD_RSHIFT))
		{
			KeyTranslationTable::const_iterator shift_key_it = mShiftTransTable.find(c);
			if (shift_key_it != mShiftTransTable.end())
				c = shift_key_it->second;
		}

		return c;
	}
	return 0;
}


//
// ISDL12KeyboardInputDevice::active
//
bool ISDL12KeyboardInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL12KeyboardInputDevice::flushEvents
//
void ISDL12KeyboardInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL12KeyboardInputDevice::reset
//
void ISDL12KeyboardInputDevice::reset()
{
	flushEvents();
}


//
// ISDL12KeyboardInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12KeyboardInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
}


//
// ISDL12KeyboardInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12KeyboardInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
	SDL_EventState(SDL_KEYUP, SDL_ENABLE);
}


//
// ISDL12KeyboardInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL12KeyboardInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	int num_events = 0;
	const int max_events = 1024;
	SDL_Event sdl_events[max_events];

	bool quit_event = false;

	while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_KEYEVENTMASK)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];
			assert(sdl_ev.type == SDL_KEYDOWN || sdl_ev.type == SDL_KEYUP);

			if (sdl_ev.key.keysym.sym == SDLK_F4 && sdl_ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT))
			{
				// HeX9109: Alt+F4 for cheats! Thanks Spleen
				// [SL] Don't handle it here but make sure we indicate there was an ALT+F4 event.
				quit_event = true;
			}
			else if (sdl_ev.key.keysym.sym == SDLK_TAB && sdl_ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT))
			{
				// do nothing - the event is dropped
			}
			else
			{
				// Normal game keyboard event - insert it into our internal queue
				event_t ev;
				ev.type = (sdl_ev.type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;
				ev.data1 = translateKey(sdl_ev.key.keysym.sym);
				ev.data2 = ev.data3 = translateKeyText(sdl_ev.key.keysym.sym, sdl_ev.key.keysym.mod);
				
				if (ev.data1)
					mEvents.push(ev);
			}
		}
	}

	// Translate the ALT+F4 key combo event into a SDL_QUIT event and push
	// it back into SDL's event queue so that it can be handled elsewhere.
	if (quit_event)
	{
		SDL_Event sdl_ev;
		sdl_ev.type = SDL_QUIT;
		SDL_PushEvent(&sdl_ev);
	}
}


//
// ISDL12KeyboardInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12KeyboardInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL12MouseInputDevice implementation
//
// ============================================================================

//
// ISDL12MouseInputDevice::ISDL12MouseInputDevice
//
ISDL12MouseInputDevice::ISDL12MouseInputDevice(int id) :
	mActive(false)
{
	reset();
}


//
// ISDL12MouseInputDevice::~ISDL12MouseInputDevice
ISDL12MouseInputDevice::~ISDL12MouseInputDevice()
{
	pause();
}


//
// ISDL12MouseInputDevice::active
//
bool ISDL12MouseInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL12MouseInputDevice::center
//
// Moves the mouse to the center of the screen to prevent absolute position
// methods from causing problems when the mouse is near the screen edges.
//
void ISDL12MouseInputDevice::center()
{
	if (active())
	{
		const int centerx = I_GetVideoWidth() / 2, centery = I_GetVideoHeight() / 2;
		int prevx, prevy;

		// get the x and y mouse position prior to centering it
		SDL_GetMouseState(&prevx, &prevy);

		// warp the mouse to the center of the screen
		SDL_WarpMouse(centerx, centery);

		// SDL_WarpMouse inserts a mouse event to warp the cursor to the center of the screen.
		// Filter out this mouse event by reading all of the mouse events in SDL'S queue and
		// re-insert any mouse events that were not inserted by SDL_WarpMouse.
		SDL_PumpEvents();

		// Retrieve chunks of up to 1024 events from SDL
		const int max_events = 1024;
		int num_events = 0;
		SDL_Event sdl_events[max_events];

		num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_MOUSEMOTIONMASK);
		assert(num_events < max_events);
		for (int i = 0; i < num_events; i++)
		{
			SDL_Event& sdl_ev = sdl_events[i];
			assert(sdl_ev.type == SDL_MOUSEMOTION);

			// drop the events caused by SDL_WarpMouse
			if (sdl_ev.motion.x == centerx && sdl_ev.motion.y == centery && 
				sdl_ev.motion.xrel == centerx - prevx && sdl_ev.motion.yrel == centery - prevy)
				continue;

			// this event is not the event caused by SDL_WarpMouse so add it back
			// to the event queue
			SDL_PushEvent(&sdl_ev);
		}
	}
}


//
// ISDL12MouseInputDevice::flushEvents
//
void ISDL12MouseInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL12MouseInputDevice::reset
//
void ISDL12MouseInputDevice::reset()
{
	flushEvents();
	center();
}


//
// ISDL12MouseInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12MouseInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	SDL_ShowCursor(true);
}


//
// ISDL12MouseInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12MouseInputDevice::resume()
{
	mActive = true;
	SDL_ShowCursor(false);
	reset();
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
}


//
// ISDL12MouseInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL12MouseInputDevice::gatherEvents()
{
	if (active())
	{
		// Force SDL to gather events from input devices. This is called
		// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
		// process only mouse events, SDL_PumpEvents is necessary.
		SDL_PumpEvents();

		// Retrieve chunks of up to 1024 events from SDL
		int num_events = 0;
		const int max_events = 1024;
		SDL_Event sdl_events[max_events];

		while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_MOUSEEVENTMASK)))
		{
			// insert the SDL_Events into our queue
			for (int i = 0; i < num_events; i++)
			{
				const SDL_Event& sdl_ev = sdl_events[i];
				assert(sdl_ev.type == SDL_MOUSEMOTION ||
						sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP);

				event_t ev;
				ev.data1 = ev.data2 = ev.data3 = 0;

				if (sdl_ev.type == SDL_MOUSEMOTION)
				{
					ev.type = ev_mouse;
					ev.data2 = sdl_ev.motion.xrel;
					ev.data3 = -sdl_ev.motion.yrel;
				}
				else if (sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP)
				{
					ev.type = (sdl_ev.type == SDL_MOUSEBUTTONDOWN) ? ev_keydown : ev_keyup;
					if (sdl_ev.button.button == SDL_BUTTON_LEFT)
						ev.data1 = KEY_MOUSE1;
					else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
						ev.data1 = KEY_MOUSE2;
					else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
						ev.data1 = KEY_MOUSE3;
					#if SDL_VERSION_ATLEAST(1, 2, 14)
					else if (sdl_ev.button.button == SDL_BUTTON_X1)
						ev.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
					else if (sdl_ev.button.button == SDL_BUTTON_X2)
						ev.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
					#endif
					else if (sdl_ev.button.button == SDL_BUTTON_WHEELUP)
						ev.data1 = KEY_MWHEELUP;
					else if (sdl_ev.button.button == SDL_BUTTON_WHEELDOWN)
						ev.data1 = KEY_MWHEELDOWN;
				}

				mEvents.push(ev);
			}
		}

		center();
	}
}


//
// ISDL12MouseInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12MouseInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL12JoystickInputDevice implementation
//
// ============================================================================

//
// ISDL12JoystickInputDevice::ISDL12JoystickInputDevice
//
ISDL12JoystickInputDevice::ISDL12JoystickInputDevice(int id) :
	mActive(false), mJoystickId(id), mJoystick(NULL),
	mNumHats(0), mHatStates(NULL)
{
	assert(SDL_WasInit(SDL_INIT_JOYSTICK));
	assert(mJoystickId >= 0 && mJoystickId < SDL_NumJoysticks());

	mJoystick = SDL_JoystickOpen(mJoystickId);
	if (mJoystick == NULL)
		return;

	mNumHats = SDL_JoystickNumHats(mJoystick);
	mHatStates = new int[mNumHats];

	// This turns on automatic event polling for joysticks so that the state
	// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
	SDL_JoystickEventState(SDL_ENABLE);
	
	resume();
}


//
// ISDL12JoystickInputDevice::~ISDL12JoystickInputDevice
//
ISDL12JoystickInputDevice::~ISDL12JoystickInputDevice()
{
	pause();

	SDL_JoystickEventState(SDL_IGNORE);

	#ifndef _XBOX // This is to avoid a bug in SDLx
	if (mJoystick != NULL)
		SDL_JoystickClose(mJoystick);
	#endif

	delete [] mHatStates;
}


//
// ISDL12JoystickInputDevice::active
//
bool ISDL12JoystickInputDevice::active() const
{
	return mJoystick != NULL && mActive && I_GetWindow()->isFocused();
}


//
// ISDL12JoystickInputDevice::flushEvents
//
void ISDL12JoystickInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
	for (int i = 0; i < mNumHats; i++)
		mHatStates[i] = SDL_HAT_CENTERED;
}


//
// ISDL12JoystickInputDevice::reset
//
void ISDL12JoystickInputDevice::reset()
{
	flushEvents();
}


//
// ISDL12JoystickInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12JoystickInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
}


//
// ISDL12JoystickInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12JoystickInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_JOYAXISMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_ENABLE);
}


//
// ISDL12JoystickInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any joystick events and translates
// them to an event_t instances before putting them into this instance's
// event queue.
//
void ISDL12JoystickInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	int num_events = 0;
	const int max_events = 1024;
	SDL_Event sdl_events[max_events];

	while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_JOYEVENTMASK)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];

			assert(sdl_ev.type == SDL_JOYBUTTONDOWN || sdl_ev.type == SDL_JOYBUTTONUP ||
					sdl_ev.type == SDL_JOYAXISMOTION || sdl_ev.type == SDL_JOYHATMOTION ||
					sdl_ev.type == SDL_JOYBALLMOTION);

			if ((sdl_ev.type == SDL_JOYBUTTONDOWN || sdl_ev.type == SDL_JOYBUTTONUP) &&
				sdl_ev.jbutton.which == mJoystickId)
			{
				event_t button_event;
				button_event.data1 = button_event.data2 = sdl_ev.jbutton.button + KEY_JOY1;
				button_event.data3 = 0;
				button_event.type = (sdl_ev.type == SDL_JOYBUTTONDOWN) ? ev_keydown : ev_keyup;
				mEvents.push(button_event);
			}
			else if (sdl_ev.type == SDL_JOYAXISMOTION && sdl_ev.jaxis.which == mJoystickId)
			{
				event_t motion_event;
				motion_event.type = ev_joystick;
				motion_event.data1 = motion_event.data3 = 0;
				motion_event.data2 = sdl_ev.jaxis.axis;
				if ((sdl_ev.jaxis.value >= JOY_DEADZONE) || (sdl_ev.jaxis.value <= -JOY_DEADZONE))
					motion_event.data3 = sdl_ev.jaxis.value;
				mEvents.push(motion_event);
			}
			else if (sdl_ev.type == SDL_JOYHATMOTION && sdl_ev.jhat.which == mJoystickId)
			{
				// [SL] A single SDL joystick hat event indicates on/off for each of the
				// directional triggers for that hat. We need to create a separate 
				// ev_keydown or ev_keyup event_t instance for each directional trigger
				// indicated in this SDL joystick event.
				assert(sdl_ev.jhat.hat < mNumHats);
				int new_state = sdl_ev.jhat.value;
				int old_state = mHatStates[sdl_ev.jhat.hat];

				static const int flags[4] = { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT };
				for (int i = 0; i < 4; i++)
				{
					event_t hat_event;
					hat_event.data1 = hat_event.data2 = (sdl_ev.jhat.hat * 4) + KEY_HAT1 + i;
					hat_event.data3 = 0;

					// determine if the flag's state has changed (ignore it if it hasn't)
					if (!(old_state & flags[i]) && (new_state & flags[i]))
					{
						hat_event.type = ev_keydown;
						mEvents.push(hat_event);
					}
					else if ((old_state & flags[i]) && !(new_state & flags[i]))
					{
						hat_event.type = ev_keyup;
						mEvents.push(hat_event);
					}
				}

				mHatStates[sdl_ev.jhat.hat] = new_state;
			}
		}
	}
}


//
// ISDL12JoystickInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12JoystickInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}

// ============================================================================
//
// ISDL12InputSubsystem implementation
//
// ============================================================================

//
// ISDL12InputSubsystem::ISDL12InputSubsystem
//
ISDL12InputSubsystem::ISDL12InputSubsystem() :
	IInputSubsystem(),
	mInputGrabbed(false)
{
	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// Tell SDL to ignore events from the input devices
	// IInputDevice constructors will enable these events when they're initialized.
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);

	SDL_ShowCursor(false);
	grabInput();
}


//
// ISDL12InputSubsystem::~ISDL12InputSubsystem
//
ISDL12InputSubsystem::~ISDL12InputSubsystem()
{
	if (getKeyboardInputDevice())
		shutdownKeyboard(0);
	if (getMouseInputDevice())
		shutdownMouse(0);
	if (getJoystickInputDevice())
		shutdownJoystick(0);

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


//
// ISDL12InputSubsystem::getKeyboardDevices
//
// SDL only allows for one logical keyboard so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL12InputSubsystem::getKeyboardDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& device_info = devices.back();
	device_info.mId = 0;
	device_info.mDeviceName = "SDL 1.2 keyboard";
	return devices;
}


//
// ISDL12InputSubsystem::initKeyboard
//
void ISDL12InputSubsystem::initKeyboard(int id)
{
	shutdownKeyboard(0);

	const std::vector<IInputDeviceInfo> devices = getKeyboardDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setKeyboardInputDevice(new ISDL12KeyboardInputDevice(id));
	registerInputDevice(getKeyboardInputDevice());
	getKeyboardInputDevice()->resume();
}


//
// ISDL12InputSubsystem::shutdownKeyboard
//
void ISDL12InputSubsystem::shutdownKeyboard(int id)
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setKeyboardInputDevice(NULL);
	}
}


//
// ISDL12InputSubsystem::pauseKeyboard
//
void ISDL12InputSubsystem::pauseKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->pause();
}


//
// ISDL12InputSubsystem::resumeKeyboard
//
void ISDL12InputSubsystem::resumeKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->resume();
}


//
// ISDL12InputSubsystem::getMouseDevices
//
// SDL only allows for one logical mouse so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL12InputSubsystem::getMouseDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& sdl_device_info = devices.back();
	sdl_device_info.mId = 0;
	sdl_device_info.mDeviceName = "SDL 1.2 mouse";

	return devices;
}


//
// ISDL12InputSubsystem::initMouse
//
void ISDL12InputSubsystem::initMouse(int id)
{
	shutdownMouse(0);

	const std::vector<IInputDeviceInfo> devices = getMouseDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setMouseInputDevice(new ISDL12MouseInputDevice(id));
	assert(getMouseInputDevice() != NULL);
	registerInputDevice(getMouseInputDevice());
	getMouseInputDevice()->resume();
}


//
// ISDL12InputSubsystem::shutdownMouse
//
void ISDL12InputSubsystem::shutdownMouse(int id)
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setMouseInputDevice(NULL);
	}
}


//
// ISDL12InputSubsystem::pauseMouse
//
void ISDL12InputSubsystem::pauseMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->pause();
}


//
// ISDL12InputSubsystem::resumeMouse
//
void ISDL12InputSubsystem::resumeMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->resume();
}


//
//
// ISDL12InputSubsystem::getJoystickDevices
//
//
std::vector<IInputDeviceInfo> ISDL12InputSubsystem::getJoystickDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		devices.push_back(IInputDeviceInfo());
		IInputDeviceInfo& device_info = devices.back();
		device_info.mId = i;
		char name[256];
		sprintf(name, "SDL 1.2 joystick (%s)", SDL_JoystickName(i));
		device_info.mDeviceName = name;
	}

	return devices;
}


// ISDL12InputSubsystem::initJoystick
//
void ISDL12InputSubsystem::initJoystick(int id)
{
	shutdownJoystick(0);

	const std::vector<IInputDeviceInfo> devices = getJoystickDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setJoystickInputDevice(new ISDL12JoystickInputDevice(id));
	registerInputDevice(getJoystickInputDevice());
	getJoystickInputDevice()->resume();
}


//
// ISDL12InputSubsystem::shutdownJoystick
//
void ISDL12InputSubsystem::shutdownJoystick(int id)
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setJoystickInputDevice(NULL);
	}
}


//
// ISDL12InputSubsystem::pauseJoystick
//
void ISDL12InputSubsystem::pauseJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->pause();
}


//
// ISDL12InputSubsystem::resumeJoystick
//
void ISDL12InputSubsystem::resumeJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->resume();
}


//
// ISDL12InputSubsystem::grabInput
//
void ISDL12InputSubsystem::grabInput()
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
	mInputGrabbed = true;
}


//
// ISDL12InputSubsystem::releaseInput
//
void ISDL12InputSubsystem::releaseInput()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	mInputGrabbed = false;
}

#endif	// SDL12

#ifdef SDL20

// ============================================================================
//
// ISDL20KeyboardInputDevice implementation
//
// ============================================================================

//
// ISDL20KeyboardInputDevice::ISDL20KeyboardInputDevice
//
ISDL20KeyboardInputDevice::ISDL20KeyboardInputDevice(int id) :
	mActive(false), mTextEntry(false),
	mSDLKeyTransTable(I_BuildSDLKeyTranslationTable())
{
	// enable keyboard input
	resume();
}


//
// ISDL20KeyboardInputDevice::~ISDL20KeyboardInputDevice
//
ISDL20KeyboardInputDevice::~ISDL20KeyboardInputDevice()
{
	pause();
}


//
// ISDL20KeyboardInputDevice::translateKey
//	
// Convert the SDL KeySym to an Odamex key using the mSDLKeyTransTable mapping.
// Returns 0 if the KeySym is unknown.
//
int ISDL20KeyboardInputDevice::translateKey(int sym)
{
	KeyTranslationTable::const_iterator key_it = mSDLKeyTransTable.find(sym);
	if (key_it != mSDLKeyTransTable.end())
		return key_it->second;
	return 0;
}


//
// ISDL20KeyboardInputDevice::active
//
bool ISDL20KeyboardInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL20KeyboardInputDevice::flushEvents
//
void ISDL20KeyboardInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL20KeyboardInputDevice::reset
//
void ISDL20KeyboardInputDevice::reset()
{
	flushEvents();
}


//
// ISDL20KeyboardInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20KeyboardInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);
}


//
// ISDL20KeyboardInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20KeyboardInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
	SDL_EventState(SDL_KEYUP, SDL_ENABLE);
	SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
}


//
// ISDL20KeyboardInputDevice::enableTextEntry
//
// Enables text entry for this device.
//
//
void ISDL20KeyboardInputDevice::enableTextEntry()
{
	mTextEntry = true;
	SDL_StartTextInput();
}


//
// ISDL20KeyboardInputDevice::disableTextEntry
//
// Disables text entry for this device.
//
//
void ISDL20KeyboardInputDevice::disableTextEntry()
{
	mTextEntry = false;
	SDL_StopTextInput();
}


//
// ISDL20KeyboardInputDevice::getTextEventValue
//
// Look for an SDL_TEXTINPUT event following this SDL_KEYDOWN event.
//
// On Windows platforms, SDL receives the translation from a key scancode
// to a localized text representation as a separate event. SDL 1.2 used to
// combine these separate events into a single SDL_Event however SDL 2.0
// creates two separate SDL_Events for this.
//
// When SDL2 receives a Windows event message for a key press from the
// Windows message queue, it calls "TranslateMessage" to generate
// a WM_CHAR event message that contains the localized text representation
// of the keypress. The WM_CHAR event ends up as the next event in the
// Windows message queue in practice (though there is no guarantee of this).
//
// Thus SDL2 inserts the key press event into its own queue as SDL_KEYDOWN
// and follows it with the text event SDL_TEXTINPUT.
//
int ISDL20KeyboardInputDevice::getTextEventValue()
{
	const size_t max_events = 32;
	SDL_Event sdl_events[max_events];
	
	SDL_PumpEvents();
	const size_t num_events = SDL_PeepEvents(sdl_events, max_events, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_TEXTINPUT);
	for (size_t i = 0; i < num_events; i++)
	{
		// If we found another SDL_KEYDOWN event prior to SDL_TEXTINPUT event,
		// we must assume the next SDL_TEXTINPUT event does not correspond to
		// our original SDL_KEYDOWN event.
		if (sdl_events[i].type == SDL_KEYDOWN)
			return 0;

		// Looks like we found a corresponding SDL_TEXTINPUT event
		if (sdl_events[i].type == SDL_TEXTINPUT)
			return convUTF8ToUTF32(sdl_events[i].text.text);
	}

	return 0;
}


//
// ISDL20KeyboardInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL20KeyboardInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only keyboard events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	SDL_Event sdl_ev;
	while (SDL_PeepEvents(&sdl_ev, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_TEXTINPUT))
	{
		event_t ev;

		// Process SDL_KEYDOWN / SDL_KEYUP events. SDL_TEXTINPUT events will
		// be implicitly ignored unless handled below.
		if (sdl_ev.type == SDL_KEYDOWN || sdl_ev.type == SDL_KEYUP)
		{
			const int sym = sdl_ev.key.keysym.sym;
			const int mod = sdl_ev.key.keysym.mod;

			ev.type = (sdl_ev.type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;
			ev.data1 = translateKey(sym);

			if (sdl_ev.type == SDL_KEYDOWN)
				ev.data2 = ev.data3 = getTextEventValue();

			// Ch0wW : Fixes a problem of ultra-fast repeats.
			if (sdl_ev.key.repeat != 0)
				continue;

			// drop ALT-TAB events - they're handled elsewhere
			if (sym == SDLK_TAB && mod & (KMOD_LALT | KMOD_RALT))
				continue;

			// HeX9109: Alt+F4 for cheats! Thanks Spleen
			// Translate the ALT+F4 key combo event into a SDL_QUIT event and push
			// it back into SDL's event queue so that it can be handled elsewhere.
			if (sym == SDLK_F4 && mod & (KMOD_LALT | KMOD_RALT))
			{
				SDL_Event sdl_quit_ev;
				sdl_quit_ev.type = SDL_QUIT;
				SDL_PushEvent(&sdl_quit_ev);
				continue;
			}

			// Normal game keyboard event - insert it into our internal queue
			if (ev.data1)
				mEvents.push(ev);
		}
	}
}


//
// ISDL20KeyboardInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL20KeyboardInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL20MouseInputDevice implementation
//
// ============================================================================

//
// ISDL20MouseInputDevice::ISDL20MouseInputDevice
//
ISDL20MouseInputDevice::ISDL20MouseInputDevice(int id) :
	mActive(false)
{
	reset();
}


//
// ISDL20MouseInputDevice::~ISDL20MouseInputDevice
ISDL20MouseInputDevice::~ISDL20MouseInputDevice()
{
	pause();
}


//
// ISDL20MouseInputDevice::active
//
bool ISDL20MouseInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL20MouseInputDevice::center
//
// Moves the mouse to the center of the screen to prevent absolute position
// methods from causing problems when the mouse is near the screen edges.
//
void ISDL20MouseInputDevice::center()
{
	if (active())
	{
		const int centerx = I_GetVideoWidth() / 2, centery = I_GetVideoHeight() / 2;
		int prevx, prevy;

		// get the x and y mouse position prior to centering it
		SDL_GetMouseState(&prevx, &prevy);

		// warp the mouse to the center of the screen
		// TODO: replace NULL parameter with an SDL_Window*
		SDL_WarpMouseInWindow(NULL, centerx, centery);

		// SDL_WarpMouse inserts a mouse event to warp the cursor to the center of the screen.
		// Filter out this mouse event by reading all of the mouse events in SDL'S queue and
		// re-insert any mouse events that were not inserted by SDL_WarpMouse.
		SDL_PumpEvents();

		// Retrieve chunks of up to 1024 events from SDL
		const int max_events = 1024;
		int num_events = 0;
		SDL_Event sdl_events[max_events];

		num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION);
		assert(num_events < max_events);
		for (int i = 0; i < num_events; i++)
		{
			SDL_Event& sdl_ev = sdl_events[i];
			assert(sdl_ev.type == SDL_MOUSEMOTION);

			// drop the events caused by SDL_WarpMouse
			if (sdl_ev.motion.x == centerx && sdl_ev.motion.y == centery && 
				sdl_ev.motion.xrel == centerx - prevx && sdl_ev.motion.yrel == centery - prevy)
				continue;

			// this event is not the event caused by SDL_WarpMouse so add it back
			// to the event queue
			SDL_PushEvent(&sdl_ev);
		}
	}
}


//
// ISDL20MouseInputDevice::flushEvents
//
void ISDL20MouseInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL20MouseInputDevice::reset
//
void ISDL20MouseInputDevice::reset()
{
	flushEvents();
	center();
}


//
// ISDL20MouseInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20MouseInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	SDL_ShowCursor(true);
}


//
// ISDL20MouseInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20MouseInputDevice::resume()
{
	mActive = true;
	SDL_ShowCursor(false);
	reset();
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
}


//
// ISDL20MouseInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL20MouseInputDevice::gatherEvents()
{
	if (active())
	{
		// Force SDL to gather events from input devices. This is called
		// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
		// process only mouse events, SDL_PumpEvents is necessary.
		SDL_PumpEvents();

		// Retrieve chunks of up to 1024 events from SDL
		int num_events = 0;
		const int max_events = 1024;
		SDL_Event sdl_events[max_events];

		while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL)))
		{
			// insert the SDL_Events into our queue
			for (int i = 0; i < num_events; i++)
			{
				const SDL_Event& sdl_ev = sdl_events[i];
				assert(sdl_ev.type == SDL_MOUSEMOTION || sdl_ev.type == SDL_MOUSEWHEEL ||
						sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP);

				event_t ev;
				ev.data1 = ev.data2 = ev.data3 = 0;

				if (sdl_ev.type == SDL_MOUSEMOTION)
				{
					ev.type = ev_mouse;
					ev.data2 = sdl_ev.motion.xrel;
					ev.data3 = -sdl_ev.motion.yrel;
				}
				else if (sdl_ev.type == SDL_MOUSEWHEEL)
				{
					ev.type = ev_keydown;
					int direction = 1;
					#if (SDL_VERSION >= SDL_VERSIONNUM(2, 0, 4))
					if (sdl_ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
						direction = -1;
					#endif

					// Don't add mouse wheel events other than wheel up/down
					if (sdl_ev.wheel.y == 0)
						continue;

					ev.data1 = (direction * sdl_ev.wheel.y > 0) ? KEY_MWHEELUP : KEY_MWHEELDOWN;
				}
				else if (sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP)
				{
					ev.type = (sdl_ev.type == SDL_MOUSEBUTTONDOWN) ? ev_keydown : ev_keyup;
					if (sdl_ev.button.button == SDL_BUTTON_LEFT)
						ev.data1 = KEY_MOUSE1;
					else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
						ev.data1 = KEY_MOUSE2;
					else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
						ev.data1 = KEY_MOUSE3;
					else if (sdl_ev.button.button == SDL_BUTTON_X1)
						ev.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
					else if (sdl_ev.button.button == SDL_BUTTON_X2)
						ev.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
				}

				mEvents.push(ev);
			}
		}

		center();
	}
}


//
// ISDL20MouseInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL20MouseInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL20JoystickInputDevice implementation
//
// ============================================================================

//
// ISDL20JoystickInputDevice::ISDL20JoystickInputDevice
//
ISDL20JoystickInputDevice::ISDL20JoystickInputDevice(int id) :
	mActive(false), mJoystickId(id), mJoystick(NULL),
	mNumHats(0), mHatStates(NULL)
{
	assert(SDL_WasInit(SDL_INIT_JOYSTICK));
	assert(mJoystickId >= 0 && mJoystickId < SDL_NumJoysticks());

	mJoystick = SDL_JoystickOpen(mJoystickId);
	if (mJoystick == NULL)
		return;

	mNumHats = SDL_JoystickNumHats(mJoystick);
	mHatStates = new int[mNumHats];

	// This turns on automatic event polling for joysticks so that the state
	// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
	SDL_JoystickEventState(SDL_ENABLE);
	
	resume();
}


//
// ISDL20JoystickInputDevice::~ISDL20JoystickInputDevice
//
ISDL20JoystickInputDevice::~ISDL20JoystickInputDevice()
{
	pause();

	SDL_JoystickEventState(SDL_IGNORE);

	if (mJoystick != NULL)
		SDL_JoystickClose(mJoystick);

	delete [] mHatStates;
}


//
// ISDL20JoystickInputDevice::active
//
bool ISDL20JoystickInputDevice::active() const
{
	return mJoystick != NULL && mActive && I_GetWindow()->isFocused();
}


//
// ISDL20JoystickInputDevice::flushEvents
//
void ISDL20JoystickInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
	for (int i = 0; i < mNumHats; i++)
		mHatStates[i] = SDL_HAT_CENTERED;
}


//
// ISDL20JoystickInputDevice::reset
//
void ISDL20JoystickInputDevice::reset()
{
	flushEvents();
}


//
// ISDL20JoystickInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20JoystickInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
}


//
// ISDL20JoystickInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20JoystickInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_JOYAXISMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_ENABLE);
}


//
// ISDL20JoystickInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any joystick events and translates
// them to an event_t instances before putting them into this instance's
// event queue.
//
void ISDL20JoystickInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	int num_events = 0;
	const int max_events = 1024;
	SDL_Event sdl_events[max_events];

	while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_JOYAXISMOTION, SDL_JOYBUTTONUP)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];

			assert(sdl_ev.type == SDL_JOYBUTTONDOWN || sdl_ev.type == SDL_JOYBUTTONUP ||
					sdl_ev.type == SDL_JOYAXISMOTION || sdl_ev.type == SDL_JOYHATMOTION ||
					sdl_ev.type == SDL_JOYBALLMOTION);

			if ((sdl_ev.type == SDL_JOYBUTTONDOWN || sdl_ev.type == SDL_JOYBUTTONUP) &&
				sdl_ev.jbutton.which == mJoystickId)
			{
				event_t button_event;
				button_event.data1 = button_event.data2 = sdl_ev.jbutton.button + KEY_JOY1;
				button_event.data3 = 0;
				button_event.type = (sdl_ev.type == SDL_JOYBUTTONDOWN) ? ev_keydown : ev_keyup;
				mEvents.push(button_event);
			}
			else if (sdl_ev.type == SDL_JOYAXISMOTION && sdl_ev.jaxis.which == mJoystickId)
			{
				event_t motion_event;
				motion_event.type = ev_joystick;
				motion_event.data1 = motion_event.data3 = 0;
				motion_event.data2 = sdl_ev.jaxis.axis;
				if ((sdl_ev.jaxis.value >= JOY_DEADZONE) || (sdl_ev.jaxis.value <= -JOY_DEADZONE))
					motion_event.data3 = sdl_ev.jaxis.value;
				mEvents.push(motion_event);
			}
			else if (sdl_ev.type == SDL_JOYHATMOTION && sdl_ev.jhat.which == mJoystickId)
			{
				// [SL] A single SDL joystick hat event indicates on/off for each of the
				// directional triggers for that hat. We need to create a separate 
				// ev_keydown or ev_keyup event_t instance for each directional trigger
				// indicated in this SDL joystick event.
				assert(sdl_ev.jhat.hat < mNumHats);
				int new_state = sdl_ev.jhat.value;
				int old_state = mHatStates[sdl_ev.jhat.hat];

				static const int flags[4] = { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT };
				for (int i = 0; i < 4; i++)
				{
					event_t hat_event;
					hat_event.data1 = hat_event.data2 = (sdl_ev.jhat.hat * 4) + KEY_HAT1 + i;
					hat_event.data3 = 0;

					// determine if the flag's state has changed (ignore it if it hasn't)
					if (!(old_state & flags[i]) && (new_state & flags[i]))
					{
						hat_event.type = ev_keydown;
						mEvents.push(hat_event);
					}
					else if ((old_state & flags[i]) && !(new_state & flags[i]))
					{
						hat_event.type = ev_keyup;
						mEvents.push(hat_event);
					}
				}

				mHatStates[sdl_ev.jhat.hat] = new_state;
			}
		}
	}
}


//
// ISDL20JoystickInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL20JoystickInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}

// ============================================================================
//
// ISDL20InputSubsystem implementation
//
// ============================================================================

//
// ISDL20InputSubsystem::ISDL20InputSubsystem
//
ISDL20InputSubsystem::ISDL20InputSubsystem() :
	IInputSubsystem(),
	mInputGrabbed(false)
{
	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// Tell SDL to ignore events from the input devices
	// IInputDevice constructors will enable these events when they're initialized.
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);

	SDL_ShowCursor(false);
	grabInput();
}


//
// ISDL20InputSubsystem::~ISDL20InputSubsystem
//
ISDL20InputSubsystem::~ISDL20InputSubsystem()
{
	if (getKeyboardInputDevice())
		shutdownKeyboard(0);
	if (getMouseInputDevice())
		shutdownMouse(0);
	if (getJoystickInputDevice())
		shutdownJoystick(0);

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


//
// ISDL20InputSubsystem::getKeyboardDevices
//
// SDL only allows for one logical keyboard so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL20InputSubsystem::getKeyboardDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& device_info = devices.back();
	device_info.mId = 0;
	device_info.mDeviceName = "SDL 2.0 keyboard";
	return devices;
}


//
// ISDL20InputSubsystem::initKeyboard
//
void ISDL20InputSubsystem::initKeyboard(int id)
{
	shutdownKeyboard(0);

	const std::vector<IInputDeviceInfo> devices = getKeyboardDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setKeyboardInputDevice(new ISDL20KeyboardInputDevice(id));
	registerInputDevice(getKeyboardInputDevice());
	getKeyboardInputDevice()->resume();
}


//
// ISDL20InputSubsystem::shutdownKeyboard
//
void ISDL20InputSubsystem::shutdownKeyboard(int id)
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setKeyboardInputDevice(NULL);
	}
}


//
// ISDL20InputSubsystem::pauseKeyboard
//
void ISDL20InputSubsystem::pauseKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->pause();
}


//
// ISDL20InputSubsystem::resumeKeyboard
//
void ISDL20InputSubsystem::resumeKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->resume();
}


//
// ISDL20InputSubsystem::getMouseDevices
//
// SDL only allows for one logical mouse so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL20InputSubsystem::getMouseDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& sdl_device_info = devices.back();
	sdl_device_info.mId = 0;
	sdl_device_info.mDeviceName = "SDL 2.0 mouse";
	return devices;
}


//
// ISDL20InputSubsystem::initMouse
//
void ISDL20InputSubsystem::initMouse(int id)
{
	shutdownMouse(0);

	const std::vector<IInputDeviceInfo> devices = getMouseDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setMouseInputDevice(new ISDL20MouseInputDevice(id));
	assert(getMouseInputDevice() != NULL);
	registerInputDevice(getMouseInputDevice());
	getMouseInputDevice()->resume();
}


//
// ISDL20InputSubsystem::shutdownMouse
//
void ISDL20InputSubsystem::shutdownMouse(int id)
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setMouseInputDevice(NULL);
	}
}


//
// ISDL20InputSubsystem::pauseMouse
//
void ISDL20InputSubsystem::pauseMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->pause();
}


//
// ISDL20InputSubsystem::resumeMouse
//
void ISDL20InputSubsystem::resumeMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->resume();
}


//
//
// ISDL20InputSubsystem::getJoystickDevices
//
//
std::vector<IInputDeviceInfo> ISDL20InputSubsystem::getJoystickDevices() const
{
	// TODO: does the SDL Joystick subsystem need to be initialized?
	std::vector<IInputDeviceInfo> devices;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		devices.push_back(IInputDeviceInfo());
		IInputDeviceInfo& device_info = devices.back();
		device_info.mId = i;
		char name[256];
		sprintf(name, "SDL 2.0 joystick (%s)", SDL_JoystickNameForIndex(i));
		device_info.mDeviceName = name;
	}

	return devices;
}


// ISDL20InputSubsystem::initJoystick
//
void ISDL20InputSubsystem::initJoystick(int id)
{
	shutdownJoystick(0);

	const std::vector<IInputDeviceInfo> devices = getJoystickDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setJoystickInputDevice(new ISDL20JoystickInputDevice(id));
	registerInputDevice(getJoystickInputDevice());
	getJoystickInputDevice()->resume();
}


//
// ISDL20InputSubsystem::shutdownJoystick
//
void ISDL20InputSubsystem::shutdownJoystick(int id)
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setJoystickInputDevice(NULL);
	}
}


//
// ISDL20InputSubsystem::pauseJoystick
//
void ISDL20InputSubsystem::pauseJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->pause();
}


//
// ISDL20InputSubsystem::resumeJoystick
//
void ISDL20InputSubsystem::resumeJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->resume();
}


//
// ISDL20InputSubsystem::grabInput
//
void ISDL20InputSubsystem::grabInput()
{
	SDL_SetRelativeMouseMode(SDL_TRUE);
	mInputGrabbed = true;
}


//
// ISDL20InputSubsystem::releaseInput
//
void ISDL20InputSubsystem::releaseInput()
{
	SDL_SetRelativeMouseMode(SDL_FALSE);
	mInputGrabbed = false;
}

#endif	// SDL20

VERSION_CONTROL (i_sdlinput_cpp, "$Id$")
