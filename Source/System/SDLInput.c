// SDL INPUT
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/CroMagRally

#include "game.h"
#include <SDL.h>

#ifdef __3DS__
	#include "Platform/3ds/Pomme3ds.h"
#endif

#ifndef __3DS__
	extern SDL_Window* gSDLWindow;
#endif

// Provide GameController stubs for CI runners that have old SDL packages.
// This lets us run quick compile checks on the CI without recompiling SDL.
#ifndef __3DS__
#if !(SDL_VERSION_ATLEAST(2,0,12))
	#warning "Multiplayer controller support requires SDL 2.0.12 or later. The game will compile but controllers won't work!"
	static void SDL_GameControllerSetPlayerIndex(SDL_GameController *c, int i) {}
	static SDL_GameController *SDL_GameControllerFromPlayerIndex(int i) { return NULL; }
	#if !(SDL_VERSION_ATLEAST(2,0,9))
		static int SDL_GameControllerGetPlayerIndex(SDL_GameController *c) { return 0; }
	#endif
#endif
#endif

/***************/
/* CONSTANTS   */
/***************/

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_PRESSED		= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};

#define kJoystickDeadZoneFrac			(.33f)
#define kJoystickDeadZoneFrac_UI		(.66f)

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

typedef struct Controller
{
	bool					open;
	bool					fallbackToKeyboard;
#ifndef __3DS__
	SDL_GameController*		controllerInstance;
	SDL_JoystickID			joystickInstance;
#endif
	KeyState				needStates[NUM_CONTROL_NEEDS];
	float					needAnalog[NUM_CONTROL_NEEDS];
	float					needAnalogRaw[NUM_CONTROL_NEEDS];
} Controller;

Boolean				gUserPrefersGamepad = false;

static Boolean		gControllerPlayerMappingLocked = false;
Controller			gControllers[MAX_LOCAL_PLAYERS];

#ifndef __3DS__
static KeyState		gKeyboardStates[SDL_NUM_SCANCODES];
#endif
static KeyState		gMouseButtonStates[NUM_SUPPORTED_MOUSE_BUTTONS];
static KeyState		gNeedStates[NUM_CONTROL_NEEDS];

Boolean				gMouseMotionNow = false;
#ifndef __3DS__
char				gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
#endif

#ifndef __3DS__
static void OnJoystickRemoved(SDL_JoystickID which);
static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex);
static SDL_GameController* TryOpenAnyController(bool showMessage);
static int GetControllerSlotFromSDLJoystickInstanceID(SDL_JoystickID joystickInstanceID);
#endif

#pragma mark -
/**********************/

static inline void UpdateKeyState(KeyState* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;

		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

void InvalidateNeedState(int need)
{
	gNeedStates[need] = KEYSTATE_IGNOREHELD;
}

void InvalidateAllInputs(void)
{
#ifndef __3DS__
	_Static_assert(1 == sizeof(KeyState), "sizeof(KeyState) has changed -- Rewrite this function without memset()!");

	memset(gNeedStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	memset(gKeyboardStates, KEYSTATE_IGNOREHELD, SDL_NUM_SCANCODES);
	memset(gMouseButtonStates, KEYSTATE_IGNOREHELD, NUM_SUPPORTED_MOUSE_BUTTONS);

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		memset(gControllers[i].needStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	}
#endif
}

static void UpdateRawKeyboardStates(void)
{
#ifndef __3DS__
	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);

	int minNumKeys = GAME_MIN(numkeys, SDL_NUM_SCANCODES);

	for (int i = 0; i < minNumKeys; i++)
		UpdateKeyState(&gKeyboardStates[i], keystate[i]);

	// fill out the rest
	for (int i = minNumKeys; i < SDL_NUM_SCANCODES; i++)
		UpdateKeyState(&gKeyboardStates[i], false);
#endif
}

static void ParseAltEnter(void)
{
#ifndef __3DS__
	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		&& (GetKeyState(SDL_SCANCODE_LALT) || GetKeyState(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = !gGamePrefs.fullscreen;
		SetFullscreenMode(false);

		InvalidateAllInputs();
	}
#endif
}

static void UpdateMouseButtonStates(int mouseWheelDelta)
{
#ifndef __3DS__
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	for (int i = 1; i < NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL; i++)	// SDL buttons start at 1!
	{
		bool buttonBit = 0 != (mouseButtons & SDL_BUTTON(i));
		UpdateKeyState(&gMouseButtonStates[i], buttonBit);
	}

	// Fake buttons for mouse wheel up/down
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELUP], mouseWheelDelta > 0);
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELDOWN], mouseWheelDelta < 0);
#endif
}

static void UpdateInputNeeds(void)
{
#ifdef __3DS__
	unsigned downButtons = GetHeldButtons3ds();

	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		// TODO CARL: Improve 3DS bindings
		bool downNow = false;
		switch(i)
		{
			case kNeed_ThrowForward:  downNow = downButtons & POMME_3DS_KEY_R; break;
			case kNeed_ThrowBackward: downNow = downButtons & POMME_3DS_KEY_L; break;
			case kNeed_Brakes:        downNow = downButtons & POMME_3DS_KEY_X; break;
			case kNeed_Forward:       downNow = downButtons & POMME_3DS_KEY_A; break;
			case kNeed_Backward:      downNow = downButtons & POMME_3DS_KEY_B; break;
			case kNeed_CameraMode:    downNow = downButtons & POMME_3DS_KEY_SELECT; break;
			case kNeed_RearView:      downNow = downButtons & POMME_3DS_KEY_Y; break;
			case kNeed_Left:          downNow = downButtons & POMME_3DS_KEY_DLEFT; break;
			case kNeed_Right:         downNow = downButtons & POMME_3DS_KEY_DRIGHT; break;
			case kNeed_UILeft:        downNow = downButtons & POMME_3DS_KEY_DLEFT; break;
			case kNeed_UIRight:       downNow = downButtons & POMME_3DS_KEY_DRIGHT; break;
			case kNeed_UIUp:          downNow = downButtons & POMME_3DS_KEY_DUP; break;
			case kNeed_UIDown:        downNow = downButtons & POMME_3DS_KEY_DDOWN; break;
			// case kNeed_UIPrev:        downNow = downButtons & POMME_3DS_KEY_R; break;
			//c ase kNeed_UINext:        downNow = downButtons & POMME_3DS_KEY_R; break;
			case kNeed_UIConfirm:     downNow = downButtons & POMME_3DS_KEY_A; break;
			case kNeed_UIBack:        downNow = downButtons & POMME_3DS_KEY_B; break;
			// case kNeed_UIDelete:      downNow = downButtons & POMME_3DS_KEY_R; break;
			case kNeed_UIPause:       downNow = downButtons & POMME_3DS_KEY_START; break;
			// case kNeed_UIStart:       downNow = downButtons & POMME_3DS_KEY_R; break;
		}
		UpdateKeyState(&gNeedStates[i], downNow);
	}
#else
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const InputBinding* kb = &gGamePrefs.bindings[i];

		bool downNow = false;

		for (int j = 0; j < MAX_BINDINGS_PER_NEED; j++)
		{
			int16_t scancode = kb->key[j];
			if (scancode && scancode < SDL_NUM_SCANCODES)
			{
				downNow |= gKeyboardStates[scancode] & KEYSTATE_ACTIVE_BIT;
			}
		}

//		downNow |= gMouseButtonStates[kb->mouseButton] & KEYSTATE_ACTIVE_BIT;

		UpdateKeyState(&gNeedStates[i], downNow);
	}
#endif
}

static void UpdateControllerSpecificInputNeeds(int controllerNum)
{
#ifndef __3DS__
	Controller* controller = &gControllers[controllerNum];

	if (!controller->open)
	{
		return;
	}

	SDL_GameController* controllerInstance = controller->controllerInstance;

	for (int needNum = 0; needNum < NUM_CONTROL_NEEDS; needNum++)
	{
		const InputBinding* kb = &gGamePrefs.bindings[needNum];

		float deadZoneFrac = needNum >= NUM_REMAPPABLE_NEEDS
						   ? kJoystickDeadZoneFrac_UI
						   : kJoystickDeadZoneFrac;

		bool pressed = false;
		float actuation = 0;
		float analogRaw = 0;

		for (int buttonNum = 0; buttonNum < MAX_BINDINGS_PER_NEED; buttonNum++)
		{
			const PadBinding* pb = &kb->pad[buttonNum];
			int type = pb->type;

			if (type == kInputTypeButton)
			{
				if (0 != SDL_GameControllerGetButton(controllerInstance, pb->id))
				{
					pressed = true;
					actuation = 1;
				}
			}
			else if (type == kInputTypeAxisPlus || type == kInputTypeAxisMinus)
			{
				int16_t axis = SDL_GameControllerGetAxis(controllerInstance, pb->id);
				if (type == kInputTypeAxisPlus)
					analogRaw = axis * (1.0f / 32767.0f);
				else
					analogRaw = axis * (1.0f / -32768.0f);

				if (analogRaw < 0)
					analogRaw = 0;

#if _DEBUG
				GAME_ASSERT(analogRaw >= 0);
#endif

				if (analogRaw >= deadZoneFrac)
				{
					pressed = true;
					float pastDeadZone = analogRaw - deadZoneFrac / (1.0f - deadZoneFrac);
					actuation = SDL_max(actuation, pastDeadZone);
				}
			}
		}

		controller->needAnalog[needNum] = actuation;
		controller->needAnalogRaw[needNum] = analogRaw;

		UpdateKeyState(&controller->needStates[needNum], pressed);
	}
#endif
}

#pragma mark -

/**********************/
/* PUBLIC FUNCTIONS   */
/**********************/

void DoSDLMaintenance(void)
{
#ifdef __3DS__
	ScanInput3ds();
	gUserPrefersGamepad = true;
#else
	gTextInput[0] = '\0';
	gMouseMotionNow = false;
	int mouseWheelDelta = 0;

			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				CleanQuit();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						CleanQuit();	// throws Pomme::QuitRequest
						return;

					/*
					case SDL_WINDOWEVENT_RESIZED:
						QD3D_OnWindowResized(event.window.data1, event.window.data2);
						break;
					*/
				}
				break;

				case SDL_TEXTINPUT:
					memcpy(gTextInput, event.text.text, sizeof(gTextInput));
					_Static_assert(sizeof(gTextInput) == sizeof(event.text.text), "size mismatch: gTextInput/event.text.text");
					break;

				case SDL_MOUSEMOTION:
					gMouseMotionNow = true;
					break;

				case SDL_MOUSEWHEEL:
					mouseWheelDelta += event.wheel.y;
					mouseWheelDelta += event.wheel.x;
					break;

				case SDL_CONTROLLERDEVICEADDED:
					// event.cdevice.which is the joystick's DEVICE INDEX (not an instance id!)
					TryOpenControllerFromJoystick(event.cdevice.which);
					break;

				case SDL_CONTROLLERDEVICEREMOVED:
					// event.cdevice.which is the joystick's UNIQUE INSTANCE ID (not an index!)
					OnJoystickRemoved(event.cdevice.which);
					break;

				/*
				case SDL_CONTROLLERDEVICEREMAPPED:
					printf("C-Device remapped! %d\n", event.cdevice.which);
					break;
				*/

				case SDL_KEYDOWN:
					gUserPrefersGamepad = false;
					break;

				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
				case SDL_JOYBUTTONDOWN:
					gUserPrefersGamepad = true;
					break;
		}
	}
#endif

	// Refresh the state of each individual keyboard key
	UpdateRawKeyboardStates();

	// On ALT+ENTER, toggle fullscreen, and ignore ENTER until keyup
	ParseAltEnter();

	// Refresh the state of each mouse button
#ifndef __3DS__
	UpdateMouseButtonStates(mouseWheelDelta);
#endif

	// Refresh the state of each input need
	UpdateInputNeeds();

	//-------------------------------------------------------------------------
	// Multiplayer gamepad input
	//-------------------------------------------------------------------------

	for (int controllerNum = 0; controllerNum < MAX_LOCAL_PLAYERS; controllerNum++)
	{
		UpdateControllerSpecificInputNeeds(controllerNum);
	}

			/*******************/
			/* CHECK FOR CMD+Q */
			/*******************/
			// When in-game, take a different path (see PlayArea)

	if ((!gIsInGame || gGamePaused) && IsCmdQPressed())
	{
		CleanQuit();
	}
}

#pragma mark -

Boolean GetKeyState(uint16_t sdlScancode)
{
#ifndef __3DS__
	if (sdlScancode >= SDL_NUM_SCANCODES)
		return false;
	return 0 != (gKeyboardStates[sdlScancode] & KEYSTATE_ACTIVE_BIT);
#else
	return false;
#endif
}

Boolean GetNewKeyState(uint16_t sdlScancode)
{
#ifndef __3DS__
	if (sdlScancode >= SDL_NUM_SCANCODES)
		return false;
	return gKeyboardStates[sdlScancode] == KEYSTATE_PRESSED;
#else
	return false;
#endif
}

#pragma mark -

Boolean GetClickState(int mouseButton)
{
#ifndef __3DS__
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return false;
	return 0 != (gMouseButtonStates[mouseButton] & KEYSTATE_ACTIVE_BIT);
#else
	return false;
#endif
}

Boolean GetNewClickState(int mouseButton)
{
#ifndef __3DS__
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return false;
	return gMouseButtonStates[mouseButton] == KEYSTATE_PRESSED;
#else
	return false;
#endif
}

#pragma mark -

Boolean GetNeedState(int needID, int playerID)
{
#ifndef __3DS__
	const Controller* controller = &gControllers[playerID];

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (controller->open && (controller->needStates[needID] & KEYSTATE_ACTIVE_BIT))
	{
		return true;
	}

	// Fallback to KB/M
	if (gNumLocalPlayers <= 1 || controller->fallbackToKeyboard)
	{
		return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
	}

#else
	return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
#endif
	return false;
}

Boolean GetNeedStateAnyP(int needID)
{
#ifndef __3DS__
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open
			&& (gControllers[i].needStates[needID] & KEYSTATE_ACTIVE_BIT))
		{
			return true;
		}
	}

	// Fallback to KB/M
	return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
#else
	return gNeedStates[needID] & KEYSTATE_ACTIVE_BIT;
#endif
}

Boolean GetNewNeedState(int needID, int playerID)
{
#ifndef __3DS__
	const Controller* controller = &gControllers[playerID];

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	if (controller->open && controller->needStates[needID] == KEYSTATE_PRESSED)
	{
		return true;
	}

	// Fallback to KB/M
    if (gNumLocalPlayers <= 1 || controller->fallbackToKeyboard)
	{
		return gNeedStates[needID] == KEYSTATE_PRESSED;
	}

	return false;
#else
	return gNeedStates[needID] == KEYSTATE_PRESSED;
#endif
}

Boolean GetNewNeedStateAnyP(int needID)
{
#ifndef __3DS__
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open
			&& (gControllers[i].needStates[needID] == KEYSTATE_PRESSED))
		{
			return true;
		}
	}

	// Fallback to KB/M
	return gNeedStates[needID] == KEYSTATE_PRESSED;
#else
	return gNeedStates[needID] == KEYSTATE_PRESSED;
#endif
}

static float GetAnalogValue(int needID, bool raw, int playerID)
{

	GAME_ASSERT(playerID >= 0);
	GAME_ASSERT(playerID < MAX_LOCAL_PLAYERS);
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
#ifdef __3DS__
	float left = SDL_max(0, Get3dsCPadX());
	if (needID == kNeed_Left)
	{
		return SDL_max(0, Get3dsCPadX());
	}
	else if (needID == kNeed_Right)
	{
		return SDL_min(0, Get3dsCPadX());
	}
	else if (needID == kNeed_Forward)
	{
		return SDL_max(0, Get3dsCPadY());
	}
	else if (needID == kNeed_Backward)
	{
		return SDL_min(0, Get3dsCPadY());
	}
#else
	const Controller* controller = &gControllers[playerID];

	// Keyboard takes precedence when the key is pressed
	if ((gNumLocalPlayers <= 1 || controller->fallbackToKeyboard) && gNeedStates[needID])
	{
		return 1.0f;
	}

	if (controller->open && controller->needAnalogRaw[needID] != 0.0f)
	{
		float value = controller->needAnalogRaw[needID];
#if _DEBUG
		GAME_ASSERT(value >= 0);
		GAME_ASSERT(value <= 1);
#endif

		if (!raw)
		{
			// Adjust value for dead zone.
			float deadZone = needID < NUM_REMAPPABLE_NEEDS ? kJoystickDeadZoneFrac : kJoystickDeadZoneFrac_UI;

			// Avoid magnitude bump when thumbstick is pushed past dead zone:
			// Bring magnitude from [kJoystickDeadZoneFrac, 1.0] to [0.0, 1.0].
			value = (value - deadZone) / (1.0f - deadZone);
			value = SDL_max(0, value);	// clamp to 0 if within dead zone
		}

		return value;
	}

	return 0;
#endif
}

float GetNeedAxis1D(int negativeNeedID, int positiveNeedID, int playerID)
{
	float neg = GetAnalogValue(negativeNeedID, false, playerID);
	float pos = GetAnalogValue(positiveNeedID, false, playerID);

	if (neg > pos)
	{
		return -neg;
	}
	else
	{
		return pos;
	}
}

#if 0
OGLPolar2D GetNeedAxis2D(int negXID, int posXID, int negYID, int posYID)
{
	float deadZone = negXID < NUM_REMAPPABLE_NEEDS ? kJoystickDeadZoneFrac : kJoystickDeadZoneFrac_UI;

	float negX = GetAnalogValue(negXID, true);
	float posX = GetAnalogValue(posXID, true);
	float x = negX > posX? -negX: posX;

	float negY = GetAnalogValue(negYID, true);
	float posY = GetAnalogValue(posYID, true);
	float y = negY > posY? -negY: posY;

	float mag = SDL_sqrtf(SQUARED(x) + SQUARED(y));
	if (mag < deadZone)
	{
		mag = 0;
	}
	else
	{
		mag = (mag - deadZone) / (1.0f - deadZone);
		mag = SDL_min(mag, 1);		// clamp to 0..1
	}

	float angle = SDL_atan2f(y, x);
	return (OGLPolar2D) { .a=angle, .r=mag };
}
#endif

Boolean UserWantsOut(void)
{
	return GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| GetNewNeedStateAnyP(kNeed_UIBack)
		|| GetNewNeedStateAnyP(kNeed_UIPause)
//		|| GetNewClickState(SDL_BUTTON_LEFT)
        ;
}

Boolean IsCmdQPressed(void)
{
#if __APPLE__
	return (GetKeyState(SDL_SCANCODE_LGUI) || GetKeyState(SDL_SCANCODE_RGUI))
		&& GetNewKeyState(SDL_GetScancodeFromKey(SDLK_q));
#else
	// on non-mac systems, alt-f4 is handled by the system
	return false;
#endif
}

Boolean IsCheatKeyComboDown(void)
{
#ifndef __3DS__
	// The original Mac version used B-R-I, but some cheap PC keyboards can't register
	// this particular key combo, so C-M-R is available as an alternative.
	return (GetKeyState(SDL_SCANCODE_B) && GetKeyState(SDL_SCANCODE_R) && GetKeyState(SDL_SCANCODE_I))
		|| (GetKeyState(SDL_SCANCODE_C) && GetKeyState(SDL_SCANCODE_M) && GetKeyState(SDL_SCANCODE_R));
#else
	return false;
#endif
}

#pragma mark -

OGLVector2D GetAnalogSteering(int playerID)
{
	return (OGLVector2D)
	{
		.x = GetNeedAxis1D(kNeed_Left, kNeed_Right, playerID),
		.y = GetNeedAxis1D(kNeed_Forward, kNeed_Backward, playerID)
	};
}

#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

int GetNumControllers(void)
{
	int count = 0;

#if 0
	for (int i = 0; i < SDL_NumJoysticks(); ++i)
	{
		if (SDL_IsGameController(i))
		{
			count++;
		}
	}
#else
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open)
		{
			count++;
		}
	}
#endif

	return count;
}

#ifndef __3DS__
SDL_GameController* GetController(int n)
{
	if (gControllers[n].open)
	{
		return gControllers[n].controllerInstance;
	}
	else
	{
		return NULL;
	}
}
#endif

static int FindFreeControllerSlot()
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (!gControllers[i].open)
		{
			return i;
		}
	}

	return -1;
}

static int GetControllerSlotFromJoystick(int joystickIndex)
{
#ifndef __3DS__
	SDL_JoystickID joystickInstanceID = SDL_JoystickGetDeviceInstanceID(joystickIndex);

	for (int controllerSlot = 0; controllerSlot < MAX_LOCAL_PLAYERS; controllerSlot++)
	{
		if (gControllers[controllerSlot].open &&
			gControllers[controllerSlot].joystickInstance == joystickInstanceID)
		{
			return controllerSlot;
		}
	}
#endif
	return -1;
}

#ifndef __3DS__
static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex)
{
	int controllerSlot = -1;

	// First, check that it's not in use already
	controllerSlot = GetControllerSlotFromJoystick(joystickIndex);
	if (controllerSlot >= 0)	// in use
	{
		return gControllers[controllerSlot].controllerInstance;
	}

	// If we can't get an SDL_GameController from that joystick, don't bother
	if (!SDL_IsGameController(joystickIndex))
	{
		return NULL;
	}

	// Reserve a controller slot
	controllerSlot = FindFreeControllerSlot();
	if (controllerSlot < 0)
	{
		printf("All controller slots used up.\n");
		// TODO: when a controller is unplugged, if all controller slots are used up, re-scan connected joysticks and try to open any unopened joysticks.
		return NULL;
	}

	// Use this one
	SDL_GameController* controllerInstance = SDL_GameControllerOpen(joystickIndex);

	// Assign player ID
	SDL_GameControllerSetPlayerIndex(controllerInstance, controllerSlot);

	gControllers[controllerSlot] = (Controller)
	{
		.open = true,
		.controllerInstance = controllerInstance,
		.joystickInstance = SDL_JoystickGetDeviceInstanceID(joystickIndex),
	};

	printf("Opened joystick %d as controller: %s\n",
		gControllers[controllerSlot].joystickInstance,
		SDL_GameControllerName(gControllers[controllerSlot].controllerInstance));

	return gControllers[controllerSlot].controllerInstance;
}

static SDL_GameController* TryOpenAnyUnusedController(bool showMessage)
{
	int numJoysticks = SDL_NumJoysticks();
	int numJoysticksAlreadyInUse = 0;

	if (numJoysticks == 0)
	{
		return NULL;
	}

	for (int i = 0; i < numJoysticks; ++i)
	{
		// Usable as an SDL GameController?
		if (!SDL_IsGameController(i))
		{
			continue;
		}

		// Already in use?
		if (GetControllerSlotFromJoystick(i) >= 0)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		SDL_GameController* newController = TryOpenControllerFromJoystick(i);
		if (newController)
		{
			return newController;
		}
	}

	if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// All joysticks already in use
		return NULL;
	}

	printf("Joystick(s) found, but none is suitable as an SDL_GameController.\n");
	if (showMessage)
	{
		char messageBuf[1024];
		snprintf(messageBuf, sizeof(messageBuf),
					"The game does not support your controller yet (\"%s\").\n\n"
					"You can play with the keyboard and mouse instead. Sorry!",
					SDL_JoystickNameForIndex(0));
		SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
	}

	return NULL;
}
#endif

void Rumble(float strength, uint32_t ms)
{
	#if 0	// TODO: Rumble for specific player
	if (NULL == gSDLController || !gGamePrefs.gamepadRumble)
		return;

#if !(SDL_VERSION_ATLEAST(2,0,9))
	#warning Rumble support requires SDL 2.0.9 or later
#else
	SDL_GameControllerRumble(gSDLController, (Uint16)(strength * 65535), (Uint16)(strength * 65535), ms);
#endif
	#endif
}
#ifndef __3DS__
static int GetControllerSlotFromSDLJoystickInstanceID(SDL_JoystickID joystickInstanceID)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (gControllers[i].open && gControllers[i].joystickInstance == joystickInstanceID)
		{
			return i;
		}
	}

	return -1;
}
#endif

static void CloseController(int controllerSlot)
{
#ifndef __3DS__
	GAME_ASSERT(gControllers[controllerSlot].open);
	GAME_ASSERT(gControllers[controllerSlot].controllerInstance);

	SDL_GameControllerClose(gControllers[controllerSlot].controllerInstance);
	gControllers[controllerSlot].open = false;
	gControllers[controllerSlot].controllerInstance = NULL;
	gControllers[controllerSlot].joystickInstance = -1;
#endif
}

static void MoveController(int oldSlot, int newSlot)
{
#ifndef __3DS__
	if (oldSlot == newSlot)
		return;

	printf("Remapped player controller %d ---> %d\n", oldSlot, newSlot);

	gControllers[newSlot] = gControllers[oldSlot];

	// TODO: Does this actually work??
	if (gControllers[newSlot].open)
	{
		SDL_GameControllerSetPlayerIndex(gControllers[newSlot].controllerInstance, newSlot);
	}

	// Clear duplicate slot so we don't read it by mistake in the future
	gControllers[oldSlot].controllerInstance = NULL;
	gControllers[oldSlot].joystickInstance = -1;
	gControllers[oldSlot].open = false;
#endif
}

static void CompactControllerSlots(void)
{
	int writeIndex = 0;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		GAME_ASSERT(writeIndex <= i);

		if (gControllers[i].open)
		{
			MoveController(i, writeIndex);
			writeIndex++;
		}
	}
}

static void TryFillUpVacantControllerSlots(void)
{
#ifndef __3DS__
	while (TryOpenAnyUnusedController(false) != NULL)
	{
		// Successful; there might be more joysticks available, keep going
	}
#endif
}

#ifndef __3DS__
static void OnJoystickRemoved(SDL_JoystickID joystickInstanceID)
{
	int controllerSlot = GetControllerSlotFromSDLJoystickInstanceID(joystickInstanceID);

	if (controllerSlot >= 0)		// we're using this joystick
	{
		printf("Joystick %d was removed, was used by controller slot #%d\n", joystickInstanceID, controllerSlot);

		// Nuke reference to this controller+joystick
		CloseController(controllerSlot);
	}

	if (!gControllerPlayerMappingLocked)
	{
		CompactControllerSlots();
	}

	// Fill up any controller slots that are vacant
	TryFillUpVacantControllerSlots();
}
#endif

void LockPlayerControllerMapping(void)
{
	int keyboardPlayer = gNumLocalPlayers-1;

	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		gControllers[i].fallbackToKeyboard = (i == keyboardPlayer);
	}

	gControllerPlayerMappingLocked = true;
}

void UnlockPlayerControllerMapping(void)
{
	gControllerPlayerMappingLocked = false;
	CompactControllerSlots();
	TryFillUpVacantControllerSlots();
}

const char* GetPlayerName(int whichPlayer)
{
	static char playerName[64];

	snprintf(playerName, sizeof(playerName),
			"%s %d", Localize(STR_PLAYER), whichPlayer+1);

	return playerName;
}

const char* GetPlayerNameWithInputDeviceHint(int whichPlayer)
{
	static char playerName[128];

	playerName[0] = '\0';

	snprintfcat(playerName, sizeof(playerName),
			"%s %d", Localize(STR_PLAYER), whichPlayer+1);

	if (gGameMode == GAME_MODE_CAPTUREFLAG)
	{
		snprintfcat(playerName, sizeof(playerName),
			", %s", Localize(gPlayerInfo[whichPlayer].team == 0 ? STR_RED_TEAM : STR_GREEN_TEAM));
	}

	bool enoughControllers = GetNumControllers() >= gNumLocalPlayers;

	if (!enoughControllers)
	{
		bool hasGamepad = gControllers[whichPlayer].open;
		snprintfcat(playerName, sizeof(playerName),
					"\n[%s]", Localize(hasGamepad? STR_GAMEPAD: STR_KEYBOARD));
	}

	return playerName;
}

#pragma mark -

void ResetDefaultKeyboardBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.bindings[i].key, kDefaultInputBindings[i].key, sizeof(gGamePrefs.bindings[i].key));
	}
}

void ResetDefaultGamepadBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.bindings[i].pad, kDefaultInputBindings[i].pad, sizeof(gGamePrefs.bindings[i].pad));
	}
}

void ResetDefaultMouseBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		gGamePrefs.bindings[i].mouseButton = kDefaultInputBindings[i].mouseButton;
	}
}
