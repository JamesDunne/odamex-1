// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	SDL input handler
//
//-----------------------------------------------------------------------------


// SoM 12-24-05: yeah... I'm programming on christmas eve.
// Removed all the DirectX crap.

#include <stdlib.h>
#include <list>
#include <sstream>

#include <SDL.h>
#include "win32inc.h"

#include "doomstat.h"
#include "m_argv.h"
#include "i_input.h"
#include "v_video.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#define JOY_DEADZONE 6000

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

static bool window_focused = false;
static bool nomouse = false;

EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)

typedef struct
{
	SDL_Event	Event;
	unsigned int RegTick;
	unsigned int LastTick;
} JoystickEvent_t;

static SDL_Joystick *openedjoy = NULL;
static std::list<JoystickEvent_t*> JoyEventList;

// denis - from chocolate doom
//
// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.
EXTERN_CVAR (mouse_acceleration)
EXTERN_CVAR (mouse_threshold)

extern constate_e ConsoleState;

static void I_InitMouseDriver();
CVAR_FUNC_IMPL(mouse_driver)
{
	I_InitMouseDriver();
}

static MouseInput* mouse_input = NULL;

//
// I_ShutdownMouseDriver
//
// Frees the memory used by mouse_input
//
static void I_ShutdownMouseDriver()
{
	delete mouse_input;
	mouse_input = NULL;
}

//
// I_InitMouseDriver
//
// Instantiates the proper concrete MouseInput object based on the
// mouse_driver cvar and stores a pointer to the object in mouse_input.
//
static void I_InitMouseDriver()
{
	static float prev_mouse_driver = -1.0f;
	if (mouse_driver == prev_mouse_driver)
		return;

	I_ShutdownMouseDriver();

	if (!nomouse && screen)
	{
#ifdef WIN32
		if (mouse_input == NULL && mouse_driver == RAW_WIN32_MOUSE_DRIVER)
		{
			mouse_input = RawWin32Mouse::create();
			if (mouse_input != NULL)
				Printf(PRINT_HIGH, "I_InitMouseDriver: Initializing Win32 Raw Mouse Input.\n");
		}
#endif

		if (mouse_input == NULL)
		{
			mouse_input = SDLMouse::create();
			if (mouse_input != NULL)
				Printf(PRINT_HIGH, "I_InitMouseDriver: Initializing SDL Mouse Input.\n");
		}
	}

	prev_mouse_driver = mouse_driver;
}

//
// I_FlushInput
//
// Eat all pending input from outside the game
//
void I_FlushInput()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev));
	if (mouse_input)
		mouse_input->flushEvents();
}

void I_EnableKeyRepeat()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY / 2, SDL_DEFAULT_REPEAT_INTERVAL);
}

void I_DisableKeyRepeat()
{
	SDL_EnableKeyRepeat(0, 0);
}

void I_ResetKeyRepeat()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

//
// I_CheckMouseGrab
//
// Determines if SDL should grab the mouse based on the game window having
// focus and the status of the menu and console.
//
static bool I_CheckMouseGrab()
{
	// if the window doesn't have focus, never grab it
	if (!window_focused)
		return false;

	// always grab the mouse when full screen (dont want to
	// see the mouse pointer)
	if (vid_fullscreen)
		return true;

	// Don't grab the mouse if mouse input is disabled
	if (nomouse)
		return false;

	// when menu is active, console is down or game is paused, release the mouse
	if (menuactive || ConsoleState == c_down || paused)
		return false;

	// only grab mouse when playing levels (but not demos)
	return (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && !demoplayback;
}


//
// I_CheckFocusState
//
// Determines if the Odamex window currently has the window manager focus.
// We should have input (keyboard) focus and be visible (not minimized).
//
static bool I_CheckFocusState()
{
	SDL_PumpEvents();
	Uint8 state = SDL_GetAppState();
	return (state & SDL_APPINPUTFOCUS) && (state & SDL_APPACTIVE);
}

//
// I_InitFocus
//
// Sets the initial value of window_focused.
//
static void I_InitFocus()
{
	window_focused = I_CheckFocusState();

	if (window_focused)
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
		I_ResumeMouse();
	}
	else
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		I_PauseMouse();
	}
}

//
// I_UpdateFocus
//
// Update the value of window_focused each tic and in response to SDL
// window manager events.
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.
//
static void I_UpdateFocus()
{
	bool new_window_focused = I_CheckFocusState();

	if (new_window_focused && !window_focused)
	{
		I_FlushInput();
	}
	else if (!new_window_focused && window_focused)
	{
	}

	window_focused = new_window_focused;

	bool mouse_grabbed = mouse_input && !mouse_input->paused();
	bool can_grab_mouse = I_CheckMouseGrab();

	if (can_grab_mouse && !mouse_grabbed)
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
		I_FlushInput();
		I_ResumeMouse();
	}
	else if (mouse_grabbed && !can_grab_mouse)
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		I_PauseMouse();
	}
}

// Add any joystick event to a list if it will require manual polling
// to detect release. This includes hat events (mostly due to d-pads not
// triggering the centered event when released) and analog axis bound
// as a key/button -- HyperEye
//
// RegisterJoystickEvent
//
static int RegisterJoystickEvent(SDL_Event *ev, int value)
{
	JoystickEvent_t *evc = NULL;
	event_t		  event;

	if(!ev)
		return -1;

	if(ev->type == SDL_JOYHATMOTION)
	{
		if(!JoyEventList.empty())
		{
			std::list<JoystickEvent_t*>::iterator i;

			for(i = JoyEventList.begin(); i != JoyEventList.end(); ++i)
			{
				if(((*i)->Event.type == ev->type) && ((*i)->Event.jhat.which == ev->jhat.which)
							&& ((*i)->Event.jhat.hat == ev->jhat.hat) && ((*i)->Event.jhat.value == value))
					return 0;
			}
		}

		evc = new JoystickEvent_t;

		memcpy(&evc->Event, ev, sizeof(SDL_Event));
		evc->Event.jhat.value = value;
		evc->LastTick = evc->RegTick = SDL_GetTicks();

		event.data1 = event.data2 = event.data3 = 0;

		event.type = ev_keydown;
		if(value == SDL_HAT_UP)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT1;
		else if(value == SDL_HAT_RIGHT)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT2;
		else if(value == SDL_HAT_DOWN)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT3;
		else if(value == SDL_HAT_LEFT)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT4;

		event.data2 = event.data1;
	}

	if(evc)
	{
		JoyEventList.push_back(evc);
		D_PostEvent(&event);
		return 1;
	}

	return 0;
}

void UpdateJoystickEvents()
{
	std::list<JoystickEvent_t*>::iterator i;
	event_t	event;

	if(JoyEventList.empty())
		return;

	i = JoyEventList.begin();
	while(i != JoyEventList.end())
	{
		if((*i)->Event.type == SDL_JOYHATMOTION)
		{
			// Hat position released
			if(!(SDL_JoystickGetHat(openedjoy, (*i)->Event.jhat.hat) & (*i)->Event.jhat.value))
				event.type = ev_keyup;
			// Hat button still held - Repeat at key repeat interval
			else if((SDL_GetTicks() - (*i)->RegTick >= SDL_DEFAULT_REPEAT_DELAY) &&
					(SDL_GetTicks() - (*i)->LastTick >= SDL_DEFAULT_REPEAT_INTERVAL*2))
			{
				(*i)->LastTick = SDL_GetTicks();
				event.type = ev_keydown;
			}
			else
			{
				++i;
				continue;
			}

			event.data1 = event.data2 = event.data3 = 0;

			if((*i)->Event.jhat.value == SDL_HAT_UP)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT1;
			else if((*i)->Event.jhat.value == SDL_HAT_RIGHT)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT2;
			else if((*i)->Event.jhat.value == SDL_HAT_DOWN)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT3;
			else if((*i)->Event.jhat.value == SDL_HAT_LEFT)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT4;

			D_PostEvent(&event);

			if(event.type == ev_keyup)
			{
				// Delete the released event
				delete *i;
				i = JoyEventList.erase(i);
				continue;
			}
		}
		++i;
	}
}

// This turns on automatic event polling for joysticks so that the state
// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
//
// EnableJoystickPolling
//
static int EnableJoystickPolling()
{
	return SDL_JoystickEventState(SDL_ENABLE);
}

static int DisableJoystickPolling()
{
	return SDL_JoystickEventState(SDL_IGNORE);
}

CVAR_FUNC_IMPL (use_joystick)
{
	if(var <= 0.0)
	{
		// Don't let console users disable joystick support because
		// they won't have any way to reenable through the menu.
#ifdef GCONSOLE
		use_joystick = 1.0;
#else
		I_CloseJoystick();
		DisableJoystickPolling();
#endif
	}
	else
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}
}


CVAR_FUNC_IMPL (joy_active)
{
	if( (var < 0.0) || ((int)var > I_GetJoystickCount()) )
		var = 0.0;

	I_CloseJoystick();

	I_OpenJoystick();
}

//
// I_GetJoystickCount
//
int I_GetJoystickCount()
{
	return SDL_NumJoysticks();
}

//
// I_GetJoystickNameFromIndex
//
std::string I_GetJoystickNameFromIndex (int index)
{
	const char  *joyname = NULL;
	std::string  ret;

	joyname = SDL_JoystickName(index);

	if(!joyname)
		return "";

	ret = joyname;

	return ret;
}

//
// I_OpenJoystick
//
bool I_OpenJoystick()
{
	int numjoy;

	numjoy = I_GetJoystickCount();

	if(!numjoy || !use_joystick)
		return false;

	if((int)joy_active > numjoy)
		joy_active.Set(0.0);

	if(!SDL_JoystickOpened(joy_active))
		openedjoy = SDL_JoystickOpen(joy_active);

	if(!SDL_JoystickOpened(joy_active))
		return false;

	return true;
}

//
// I_CloseJoystick
//
void I_CloseJoystick()
{
	extern int joyforward, joystrafe, joyturn, joylook;
	int		ndx;

#ifndef _XBOX // This is to avoid a bug in SDLx
	if(!I_GetJoystickCount() || !openedjoy)
		return;

	ndx = SDL_JoystickIndex(openedjoy);

	if(SDL_JoystickOpened(ndx))
		SDL_JoystickClose(openedjoy);

	openedjoy = NULL;
#endif

	// Reset joy position values. Wouldn't want to get stuck in a turn or something. -- Hyper_Eye
	joyforward = joystrafe = joyturn = joylook = 0;
}

//
// I_InitInput
//
bool I_InitInput (void)
{
	if (Args.CheckParm("-nomouse"))
		nomouse = true;

	atterm (I_ShutdownInput);

	SDL_EnableUNICODE(true);

	I_DisableKeyRepeat();

	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	Printf(PRINT_HIGH, "I_InitInput: Initializing SDL's joystick subsystem.\n");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	if((int)use_joystick && I_GetJoystickCount())
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

	I_InitMouseDriver();
	I_InitFocus();

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	I_ShutdownMouseDriver();

	SDL_WM_GrabInput(SDL_GRAB_OFF);
	I_ResetKeyRepeat();

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//UnhookWindowsHookEx(g_hKeyboardHook);
#endif
}

//
// I_PauseMouse
//
// Enables the mouse cursor and prevents the game from processing mouse movement
// or button events
//
void I_PauseMouse()
{
	SDL_ShowCursor(true);
	if (mouse_input)
		mouse_input->pause();
}

//
// I_ResumeMouse
//
// Disables the mouse cursor and allows the game to process mouse movement
// or button events
//
void I_ResumeMouse()
{
	SDL_ShowCursor(false);
	if (mouse_input)
		mouse_input->resume();
}

//
// I_GetEvent
//
void I_GetEvent()
{
	const int MAX_EVENTS = 256;
	static SDL_Event sdl_events[MAX_EVENTS];
	event_t event;

	// Process mouse movement and button events
	if (mouse_input)
		mouse_input->processEvents();

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();
	const unsigned int mask =
		SDL_KEYEVENTMASK | SDL_JOYEVENTMASK | SDL_VIDEORESIZEMASK |
		SDL_VIDEOEXPOSEMASK | SDL_QUITMASK | SDL_SYSWMEVENTMASK;

	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, mask);

	for (int i = 0; i < num_events; i++)
	{
		event.data1 = event.data2 = event.data3 = 0;

		SDL_Event* sdl_ev = &sdl_events[i];
		switch (sdl_ev->type)
		{
		case SDL_QUIT:
			AddCommandString("quit");
			break;

		case SDL_VIDEORESIZE:
		{
			// Resizable window mode resolutions
			if (!vid_fullscreen)
			{
				std::stringstream Command;
				Command << "vid_setmode " << sdl_ev->resize.w << " " << sdl_ev->resize.h;
				AddCommandString(Command.str());

				vid_defwidth.Set((float)sdl_ev->resize.w);
				vid_defheight.Set((float)sdl_ev->resize.h);
			}
			break;
		}

		case SDL_ACTIVEEVENT:
			// need to update our focus state
			I_UpdateFocus();
			break;

		case SDL_KEYDOWN:
			event.type = ev_keydown;
			event.data1 = sdl_ev->key.keysym.sym;

			if (event.data1 >= SDLK_KP0 && event.data1 <= SDLK_KP9)
				event.data2 = event.data3 = '0' + (event.data1 - SDLK_KP0);
			else if (event.data1 == SDLK_KP_PERIOD)
				event.data2 = event.data3 = '.';
			else if (event.data1 == SDLK_KP_DIVIDE)
				event.data2 = event.data3 = '/';
			else if (event.data1 == SDLK_KP_ENTER)
				event.data2 = event.data3 = '\r';
			else if ((sdl_ev->key.keysym.unicode & 0xFF80) == 0)
				event.data2 = event.data3 = sdl_ev->key.keysym.unicode;
			else
				event.data2 = event.data3 = 0;

#ifdef _XBOX
			// Fix for ENTER key on Xbox
			if (event.data1 == SDLK_RETURN)
				event.data2 = event.data3 = '\r';
#endif

#ifdef WIN32
			//HeX9109: Alt+F4 for cheats! Thanks Spleen
			if (event.data1 == SDLK_F4 && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
				AddCommandString("quit");
			// SoM: Ignore the tab portion of alt-tab presses
			// [AM] Windows 7 seems to preempt this check.
			if (event.data1 == SDLK_TAB && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
				event.data1 = event.data2 = event.data3 = 0;
			else
#endif
			D_PostEvent(&event);
			break;

		case SDL_KEYUP:
			event.type = ev_keyup;
			event.data1 = sdl_ev->key.keysym.sym;
			if ((sdl_ev->key.keysym.unicode & 0xFF80) == 0)
				event.data2 = event.data3 = sdl_ev->key.keysym.unicode;
			else
				event.data2 = event.data3 = 0;
			D_PostEvent(&event);
			break;

		case SDL_JOYBUTTONDOWN:
			if (sdl_ev->jbutton.which == joy_active)
			{
				event.type = ev_keydown;
				event.data1 = sdl_ev->jbutton.button + KEY_JOY1;
				event.data2 = event.data1;

				D_PostEvent(&event);
				break;
			}

		case SDL_JOYBUTTONUP:
			if (sdl_ev->jbutton.which == joy_active)
			{
				event.type = ev_keyup;
				event.data1 = sdl_ev->jbutton.button + KEY_JOY1;
				event.data2 = event.data1;

				D_PostEvent(&event);
				break;
			}

		case SDL_JOYAXISMOTION:
			if (sdl_ev->jaxis.which == joy_active)
			{
				event.type = ev_joystick;
				event.data1 = 0;
				event.data2 = sdl_ev->jaxis.axis;
				if ((sdl_ev->jaxis.value < JOY_DEADZONE) && (sdl_ev->jaxis.value > -JOY_DEADZONE))
					event.data3 = 0;
				else
					event.data3 = sdl_ev->jaxis.value;

				D_PostEvent(&event);
				break;
			}

		case SDL_JOYHATMOTION:
			if (sdl_ev->jhat.which == joy_active)
			{
				// Each of these need to be tested because more than one can be pressed and a
				// unique event is needed for each
				if (sdl_ev->jhat.value & SDL_HAT_UP)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_UP);
				if (sdl_ev->jhat.value & SDL_HAT_RIGHT)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_RIGHT);
				if (sdl_ev->jhat.value & SDL_HAT_DOWN)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_DOWN);
				if (sdl_ev->jhat.value & SDL_HAT_LEFT)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_LEFT);

				break;
			}
		};
	}

	if (use_joystick)
		UpdateJoystickEvents();
}

//
// I_StartTic
//
void I_StartTic (void)
{
	I_UpdateFocus();
	I_GetEvent();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

// ============================================================================
//
// DirectInputMouse
//
// ============================================================================

#ifdef WIN32
// ============================================================================
//
// RawWin32Mouse
//
// ============================================================================

// define the static member variables declared in the header
HHOOK RawWin32Mouse::mHookHandle = NULL;
RawWin32Mouse* RawWin32Mouse::mThis = NULL;

//
// RawWin32Mouse::RawWin32Mouse
//
RawWin32Mouse::RawWin32Mouse() :
	mActive(false), mInitialized(false),
	mHasBackedupMouseDevice(false),
	mPrevX(0), mPrevY(0), mPrevValid(false)
{
	backupMouseDevice();

	RAWINPUTDEVICE device;
	device.usUsagePage = 1;
	device.usUsage = 2;
	device.dwFlags = 0;
	device.hwndTarget = NULL;

	if (RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
		return;

	// set up the callback and save its handle for later
	setHook();

	mInitialized = true;
}

//
// RawWin32Mouse::~RawWin32Mouse
//
// Remove the hook for retreiving input and unregister the RAWINPUTDEVICE
//
RawWin32Mouse::~RawWin32Mouse()
{
	// remove the callback hook
	if (mHookHandle != NULL)
		UnhookWindowsHookEx(mHookHandle);
	mHookHandle = NULL;

	RAWINPUTDEVICE device;
	device.usUsagePage = 1;
	device.usUsage = 2;
	device.dwFlags = RIDEV_REMOVE;
	device.hwndTarget = NULL;

	RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE));

	restoreMouseDevice();
}

//
// RawWin32Mouse::create
//
// Instantiates a new RawWin32Mouse and returns a pointer to it if successful
// or returns NULL if unable to instantiate it.
//
RawWin32Mouse* RawWin32Mouse::create()
{
	RawWin32Mouse* obj = new RawWin32Mouse();

	if (obj && obj->mInitialized)
		return obj;

	// could not properly initialize
	delete obj;
	return NULL;
}

//
// RawWin32Mouse::flushEvents
//
// Clears the queued events
//
void RawWin32Mouse::flushEvents()
{
	clear();
}

//
// RawWin32Mouse::processEvents
//
// Iterates our queue of RAWINPUT events and converts them to Doom event_t
// and posts them for processing by the game internals.
//
void RawWin32Mouse::processEvents()
{
	if (!mActive)
		return;

	// [SL] accumulate the total mouse movement over all events polled
	// and post one aggregate mouse movement event after all are polled.
	event_t movement_event;
	movement_event.type = ev_mouse;
	movement_event.data1 = movement_event.data2 = movement_event.data3 = 0;

	for (size_t i = 0; i < queueSize(); i++)
	{
		const RAWMOUSE* mouse = &front()->data.mouse;

		if (mouse->usFlags & MOUSE_MOVE_ABSOLUTE)
		{
			// we're given absolute mouse coordinates
			if (mPrevValid)
			{
				movement_event.data2 += mouse->lLastX - mPrevX;
				movement_event.data3 -= mouse->lLastY - mPrevY;
			}

			mPrevX = mouse->lLastX;
			mPrevY = mouse->lLastY;
			mPrevValid = true;
		}
		else
		{
			// we're given relative mouse coordinates
			movement_event.data2 += mouse->lLastX;
			movement_event.data3 -= mouse->lLastY;
			mPrevValid = false;
		}

		event_t button_event;
		button_event.data2 = button_event.data3 = 0;

		// handle mouse button down events
		button_event.type = ev_keydown;
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_1_DOWN)
		{
			button_event.data1 = KEY_MOUSE1;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_2_DOWN)
		{
			button_event.data1 = KEY_MOUSE2;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
		{
			button_event.data1 = KEY_MOUSE3;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
		{
			button_event.data1 = KEY_MOUSE4;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
		{
			button_event.data1 = KEY_MOUSE5;
			D_PostEvent(&button_event);
		}

		// handle mouse button up events
		button_event.type = ev_keyup;
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_1_UP)
		{
			button_event.data1 = KEY_MOUSE1;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_2_UP)
		{
			button_event.data1 = KEY_MOUSE2;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_3_UP)
		{
			button_event.data1 = KEY_MOUSE3;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_4_UP)
		{
			button_event.data1 = KEY_MOUSE4;
			D_PostEvent(&button_event);
		}
		if (mouse->usButtonFlags & RI_MOUSE_BUTTON_5_UP)
		{
			button_event.data1 = KEY_MOUSE5;
			D_PostEvent(&button_event);
		}

		// handle scroll-wheel events
		button_event.type = ev_keydown;
		if (mouse->usButtonFlags & RI_MOUSE_WHEEL)
		{
			if ((SHORT)mouse->usButtonData < 0)
			{
				button_event.data1 = KEY_MWHEELDOWN;
				D_PostEvent(&button_event);
			}
			else if ((SHORT)mouse->usButtonData > 0)
			{
				button_event.data1 = KEY_MWHEELUP;
				D_PostEvent(&button_event);
			}
		}

		popFront();
	}

	if (movement_event.data2 != 0 || movement_event.data3 != 0)
		D_PostEvent(&movement_event);
}

void RawWin32Mouse::center()
{
	// not needed with raw mouse input
}

bool RawWin32Mouse::paused() const
{
	return mActive == false;
}

void RawWin32Mouse::pause()
{
	mActive = false;
}

void RawWin32Mouse::resume()
{
	mActive = true;
	center();
	flushEvents();
}

//
// RawWin32Mouse::setHook
//
// Sets up the Windows message hook so that hookProc can receive WM_INPUT
// messages.
//
void RawWin32Mouse::setHook()
{
	RawWin32Mouse::mThis = this;
	mHookHandle =
		SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)RawWin32Mouse::hookProcWrapper, NULL, GetCurrentThreadId());
}

//
// RawWin32Mouse::hookProc
//
// A callback function that reads WM_Input messages and queues them for polling
// in processEvents;
//
LRESULT RawWin32Mouse::hookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		MSG* msg = (MSG*)lParam;
		if (msg->message == WM_INPUT)
		{
			const UINT buf_size = sizeof(RAWINPUT);
			BYTE buf[buf_size];

			// get the size of the input data
			UINT size = 0, count;
			count = GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

			if (count == 0 && size > 0 && size <= buf_size)
			{
				// get the input data itself
				count = GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, buf, &size, sizeof(RAWINPUTHEADER));

				if (count == size)
				{
					// add the input to our queue
					RAWINPUT* raw = (RAWINPUT*)buf;
					if (count > 0 && raw->header.dwType == RIM_TYPEMOUSE)
						pushBack(raw);
				}
			}

			return 0;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

//
// RawWin32Mouse::hookProcWrapper
//
// A static member function that wraps calls to hookProc to allow member
// functions to use Windows callbacks.
//
LRESULT RawWin32Mouse::hookProcWrapper(int nCode, WPARAM wParam, LPARAM lParam)
{
	return mThis->hookProc(nCode, wParam, lParam);
}

//
// RawWin32Mouse::backupMouseDevice
//
// Saves a copy of an existing mouse raw input device. This should be called
// prior to registering our own mouse input device.
//
void RawWin32Mouse::backupMouseDevice()
{
	mHasBackedupMouseDevice = false;

	// get the number of raw input devices
	UINT num_devices;
	GetRegisteredRawInputDevices(NULL, &num_devices, sizeof(RAWINPUTDEVICE));

	// create a buffer to hold the raw input device info
	RAWINPUTDEVICE* devices = new RAWINPUTDEVICE[num_devices];

	// retrieve the raw input device info
	UINT res = GetRegisteredRawInputDevices(devices, &num_devices, sizeof(RAWINPUTDEVICE));
	if (res == num_devices)
	{
		for (UINT i = 0; i < num_devices; i++)
		{
			// if it's a mouse, back it up
			if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
			{
				memcpy(&mBackedupMouseDevice, &devices[i], sizeof(RAWINPUTDEVICE));
				mHasBackedupMouseDevice = true;
			}
		}
    }

    delete [] devices;
}

//
// RawWin32Mouse::restoreMouseDevice
//
// Restores a saved copy of a mouse raw input device. This should be called
// after unregistering our own mouse input device.
//
void RawWin32Mouse::restoreMouseDevice()
{
	if (mHasBackedupMouseDevice)
		RegisterRawInputDevices(&mBackedupMouseDevice, 1, sizeof(RAWINPUTDEVICE));
}

#endif	// WIN32

// ============================================================================
//
// SDLMouse
//
// ============================================================================

//
// SDLMouse::SDLMouse
//
SDLMouse::SDLMouse() :
	mActive(false)
{
}

//
// SDLMouse::~SDLMouse
//
SDLMouse::~SDLMouse()
{
	SDL_ShowCursor(true);
}

//
// SDLMouse::create
//
// Instantiates a new SDLMouse and returns a pointer to it if successful
// or returns NULL if unable to instantiate it. However, since SDL is
// always availible, this never returns NULL
//
//
SDLMouse* SDLMouse::create()
{
	return new SDLMouse();
}

void SDLMouse::flushEvents()
{
	SDL_PumpEvents();
	SDL_PeepEvents(mEvents, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEEVENTMASK);
}

//
// SDLMouse::processEvents
//
// Pumps SDL's event queue and processes only its mouse events. The events
// are converted to Doom event_t and posted for processing by the game
// internals.
//
void SDLMouse::processEvents()
{
	if (!mActive)
		return;

	// [SL] accumulate the total mouse movement over all events polled
	// and post one aggregate mouse movement event after all are polled.
	event_t movement_event;
	movement_event.type = ev_mouse;
	movement_event.data1 = movement_event.data2 = movement_event.data3 = 0;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();
	int num_events = SDL_PeepEvents(mEvents, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEEVENTMASK);

	for (int i = 0; i < num_events; i++)
	{
		SDL_Event* sdl_ev = &mEvents[i];
		switch (sdl_ev->type)
		{
		case SDL_MOUSEMOTION:
		{
			movement_event.data2 += sdl_ev->motion.xrel;
			movement_event.data3 -= sdl_ev->motion.yrel;
			break;
		}

		case SDL_MOUSEBUTTONDOWN:
		{
			event_t button_event;
			button_event.type = ev_keydown;
			button_event.data1 = button_event.data2 = button_event.data3 = 0;

			if (sdl_ev->button.button == SDL_BUTTON_LEFT)
				button_event.data1 = KEY_MOUSE1;
			else if (sdl_ev->button.button == SDL_BUTTON_RIGHT)
				button_event.data1 = KEY_MOUSE2;
			else if (sdl_ev->button.button == SDL_BUTTON_MIDDLE)
				button_event.data1 = KEY_MOUSE3;
			else if (sdl_ev->button.button == SDL_BUTTON_X1)
				button_event.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
			else if (sdl_ev->button.button == SDL_BUTTON_X2)
				button_event.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
			else if (sdl_ev->button.button == SDL_BUTTON_WHEELUP)
				button_event.data1 = KEY_MWHEELUP;
			else if (sdl_ev->button.button == SDL_BUTTON_WHEELDOWN)
				button_event.data1 = KEY_MWHEELDOWN;

			if (button_event.data1 != 0)
				D_PostEvent(&button_event);
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			event_t button_event;
			button_event.type = ev_keyup;
			button_event.data1 = button_event.data2 = button_event.data3 = 0;

			if (sdl_ev->button.button == SDL_BUTTON_LEFT)
				button_event.data1 = KEY_MOUSE1;
			else if (sdl_ev->button.button == SDL_BUTTON_RIGHT)
				button_event.data1 = KEY_MOUSE2;
			else if (sdl_ev->button.button == SDL_BUTTON_MIDDLE)
				button_event.data1 = KEY_MOUSE3;
			else if (sdl_ev->button.button == SDL_BUTTON_X1)
				button_event.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
			else if (sdl_ev->button.button == SDL_BUTTON_X2)
				button_event.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5

			if (button_event.data1 != 0)
				D_PostEvent(&button_event);
			break;
		}
		default:
			// do nothing
			break;
		}
	}

	if (movement_event.data2 != 0 || movement_event.data3 != 0)
		D_PostEvent(&movement_event);

	center();
}

//
// SDLMouse::center
//
// Moves the mouse to the center of the screen to prevent absolute position
// methods from causing problems when the mouse is near the screen edges.
//
void SDLMouse::center()
{
	if (screen)
	{
		// warp the mouse to the center of the screen
		SDL_WarpMouse(screen->width / 2, screen->height / 2);
		// SDL_WarpMouse creates a new event in the queue and needs to be thrown out
		flushEvents();
	}
}


bool SDLMouse::paused() const
{
	return mActive == false;
}


void SDLMouse::pause()
{
	mActive = false;
}


void SDLMouse::resume()
{
	mActive = true;
	center();
}

VERSION_CONTROL (i_input_cpp, "$Id$")


