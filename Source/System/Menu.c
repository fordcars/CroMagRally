// MENU.C
// (c)2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>

#include "globals.h"
#include "misc.h"
#include "menu.h"
#include "window.h"
#include "ogl_support.h"
#include "objects.h"
#include "3dmath.h"
#include "main.h"
#include "player.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include "stb_image.h"
#include "localization.h"
#include "atlas.h"
#include <string.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec		gDataSpec;
extern	int			gCurrentSplitScreenPane;
extern	OGLVector3D		gRecentTerrainNormal;
extern	PrefsType		gGamePrefs;
extern	int				gGameWindowWidth,gGameWindowHeight;
extern	float	g2DLogicalWidth;
extern	float	g2DLogicalHeight;
extern	SDL_GameController* gSDLController;

#define DECLARE_WORKBUF(buf, bufSize) char (buf)[256]; const int (bufSize) = 256
#define DECLARE_STATIC_WORKBUF(buf, bufSize) static char (buf)[256]; static const int (bufSize) = 256

#define UpdateInput() DoSDLMaintenance()
#define UserWantsOut() AreAnyNewKeysPressed()
#define FlushMouseButtonPress(whatever) true

/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode* MakeTextAtRowCol(const char* text, int row, int col);
static void LayOutMenu(const MenuItem* menu);
static void LayOutCMRCycler(int row, float sweepFactor);
static ObjNode* LayOutCyclerValueText(int row);

#define SpecialRow					Special[0]
#define SpecialCol					Special[1]
#define SpecialMuted				Special[2]
#define SpecialPulsateTimer			SpecialF[0]
#define SpecialSweepTimer			SpecialF[1]

/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_MENU_ROWS	25
#define MAX_MENU_COLS	5
#define MAX_STACK_LENGTH 16

#define kSfxNavigate	EFFECT_SELECTCLICK
#define kSfxMenuChange	EFFECT_SELECTCLICK
#define kSfxBack		EFFECT_GETPOW
#define kSfxCycle		EFFECT_SELECTCLICK
#define kSfxError		EFFECT_BADSELECT
#define kSfxDelete		EFFECT_BOOM

#define PlayConfirmEffect() PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3)

const int16_t kJoystickDeadZone_BindingThreshold = (75 * 32767 / 100);

enum
{
	kMenuStateOff,
	kMenuStateFadeIn,
	kMenuStateReady,
	kMenuStateFadeOut,
	kMenuStateAwaitingKeyPress,
	kMenuStateAwaitingPadPress,
	kMenuStateAwaitingMouseClick,
};

const MenuStyle kDefaultMenuStyle =
{
	.darkenPane			= true,
	.darkenPaneScaleY	= 480,
	.darkenPaneOpacity	= .8f,
	.fadeInSpeed		= 3.0f,
	.asyncFadeOut		= true,
	.fadeOutSceneOnExit	= true,
	.centeredText		= true,
	.titleColor			= {1.0f, 1.0f, 0.7f, 1.0f},
	.highlightColor		= {0.3f, 0.5f, 0.2f, 1.0f},
	.inactiveColor		= {1.0f, 1.0f, 1.0f, 1.0f},
	.inactiveColor2		= {0.8f, 0.0f, 0.5f, 0.5f},
	.standardScale		= .45f,
	.rowHeight			= 0.15f,
	.uniformXExtent		= 0,
	.playMenuChangeSounds	= true,
	.startButtonExits	= false,
	.isInteractive		= true,
	.canBackOutOfRootMenu	= false,
	.textSlot			= SLOT_OF_DUMB + 100,
};

static const float kMenuItemHeightMultipliers[kMenuItem_NUM_ITEM_TYPES] =  // acts as text scale as well
{
	[kMenuItem_END_SENTINEL] = 0.0f,
	[kMenuItem_Title] = 1.4f,
	[kMenuItem_Subtitle] = 0.8f,
	[kMenuItem_Label] = 1.0f,
	//	[kMenuItem_Action]       = 1,
	//	[kMenuItem_Submenu]      = 1,
	[kMenuItem_Spacer] = 0.5f,
	[kMenuItem_Cycler] = 1,
	[kMenuItem_CMRCycler] = 1,
	[kMenuItem_Pick] = 1,
	[kMenuItem_KeyBinding] = 1,
	[kMenuItem_PadBinding] = 1,
	[kMenuItem_MouseBinding] = 1,
};

/*********************/
/*    VARIABLES      */
/*********************/

typedef struct
{
	const MenuItem**	menuTree;
	const MenuItem*		menu;
	const MenuItem*		rootMenu;
	MenuStyle			style;

	int					numMenuEntries;
	int					menuRow;
	int					keyColumn;
	int					padColumn;
	float				menuColXs[MAX_MENU_COLS];
	float				menuRowYs[MAX_MENU_ROWS];
	float				menuFadeAlpha;
	int					menuState;
	int					menuPick;
	ObjNode*			menuObjects[MAX_MENU_ROWS][MAX_MENU_COLS];

	struct
	{
		const MenuItem* menu;
		int row;
	} history[MAX_STACK_LENGTH];
	int					historyPos;

	bool				mouseHoverValidRow;
	int					mouseHoverColumn;
	SDL_Cursor*			handCursor;
	SDL_Cursor*			standardCursor;

	float				idleTime;
} MenuNavigation;

static MenuNavigation* gNav = NULL;

void InitMenuNavigation(void)
{
	GAME_ASSERT(gNav == NULL);
	gNav = (MenuNavigation*) AllocPtrClear(sizeof(MenuNavigation));
	memcpy(&gNav->style, &kDefaultMenuStyle, sizeof(MenuStyle));
	gNav->menuColXs[0] = 0;
	gNav->menuColXs[1] = 170;
	gNav->menuColXs[2] = 300;
	gNav->menuColXs[3] = 430;
	gNav->menuColXs[4] = 560;
	gNav->menuPick = -1;
	gNav->menuState = kMenuStateOff;
	gNav->mouseHoverColumn = -1;
}

void DisposeMenuNavigation(void)
{
	SafeDisposePtr((Ptr) gNav);
	gNav = nil;
}

const MenuItem* GetCurrentMenu(void)
{
	return gNav->menu;
}

float GetMenuIdleTime(void)
{
	return gNav->idleTime;
}

void KillMenu(int returnCode)
{
	if (gNav->menuState == kMenuStateReady)
	{
		gNav->menuPick = returnCode;
		gNav->menuState = kMenuStateFadeOut;
	}
}

/****************************/
/*    MENU UTILITIES        */
/****************************/
#pragma mark - Utilities

static OGLColorRGBA TwinkleColor(void)
{
	//float rf = .7f + RandomFloat() * .29f;
	//return (OGLColorRGBA){rf, rf, rf, 1};
	return gNav->style.highlightColor;
}

static OGLColorRGBA PulsateColor(float* time)
{
	*time += gFramesPerSecondFrac;
	float intensity = (1.0f + sinf(*time * 10.0f)) * 0.5f;
	return (OGLColorRGBA) {1,1,1,intensity};
}

static KeyBinding* GetBindingAtRow(int row)
{
	return &gGamePrefs.keys[gNav->menu[row].kb][0];
}

static const char* GetKeyBindingName(int row, int col)
{
	int16_t scancode = GetBindingAtRow(row)->key[col];
	if (scancode == 0)
		return Localize(STR_UNBOUND_PLACEHOLDER);
	else
		return SDL_GetScancodeName(scancode);
}

static const char* GetPadBindingName(int row, int col)
{
	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->gamepad[col].type)
	{
		case kInputTypeUnbound:
			return Localize(STR_UNBOUND_PLACEHOLDER);

		case kInputTypeButton:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_BUTTON_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_BUTTON_A:				return "A";
				case SDL_CONTROLLER_BUTTON_B:				return "B";
				case SDL_CONTROLLER_BUTTON_X:				return "X";
				case SDL_CONTROLLER_BUTTON_Y:				return "Y";
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return "LB";
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return "RB";
				case SDL_CONTROLLER_BUTTON_LEFTSTICK:		return "Push LS";
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		return "Push RS";
				default:
					return SDL_GameControllerGetStringForButton(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisPlus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "LS right";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "LS down";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "RS right";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "RS down";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "LT";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "RT";
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisMinus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "LS left";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "LS up";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "RS left";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "RS up";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "LT";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "RT";
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		default:
			return "???";
	}
}

static const char* GetMouseBindingName(int row)
{
	DECLARE_STATIC_WORKBUF(buf, bufSize);

	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->mouseButton)
	{
		case 0:							return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_BUTTON_LEFT:			return Localize(STR_MOUSE_BUTTON_LEFT);
		case SDL_BUTTON_MIDDLE:			return Localize(STR_MOUSE_BUTTON_MIDDLE);
		case SDL_BUTTON_RIGHT:			return Localize(STR_MOUSE_BUTTON_RIGHT);
		case SDL_BUTTON_WHEELUP:		return Localize(STR_MOUSE_WHEEL_UP);
		case SDL_BUTTON_WHEELDOWN:		return Localize(STR_MOUSE_WHEEL_DOWN);
		default:
			snprintf(buf, bufSize, "%s %d", Localize(STR_BUTTON), kb->mouseButton);
			return buf;
	}
}

static bool IsMenuItemSelectable(const MenuItem* mi)
{
	switch (mi->type)
	{
		case kMenuItem_Spacer:
		case kMenuItem_Label:
		case kMenuItem_Title:
		case kMenuItem_Subtitle:
			return false;
		
		default:
			if (mi->enableIf != NULL)
			{
				return mi->enableIf(mi);
			}
			else
			{
				return true;
			}
	}

}

static void ReplaceMenuText(LocStrID originalTextInMenuDefinition, LocStrID newText)
{
	for (int i = 0; i < MAX_MENU_ROWS && gNav->menu[i].type != kMenuItem_END_SENTINEL; i++)
	{
		if (gNav->menu[i].text == originalTextInMenuDefinition)
		{
			MakeTextAtRowCol(Localize(newText), i, 0);
		}
	}
}

/****************************/
/*    MENU MOVE CALLS       */
/****************************/
#pragma mark - Move calls

static void MoveDarkenPane(ObjNode* node)
{
	node->ColorFilter.a = gNav->menuFadeAlpha * gNav->style.darkenPaneOpacity;
}

static void MoveGenericMenuItem(ObjNode* node)
{
	node->SpecialSweepTimer += gFramesPerSecondFrac * 5;

	if (node->SpecialSweepTimer < 0)
	{
		node->ColorFilter.a = 0;
	}
	else if (node->SpecialSweepTimer < 1)
	{
		node->ColorFilter.a *= node->SpecialSweepTimer;

		float xBackup = node->Coord.x;

		float p = (1.0f - node->SpecialSweepTimer);
		node->Coord.x -= p*p * 50.0f;
		UpdateObjectTransforms(node);

		node->Coord.x = xBackup;
	}
	else
	{
		node->ColorFilter.a = 1;
		UpdateObjectTransforms(node);
	}

	if (node->SpecialMuted)
		node->ColorFilter.a *= .5f;
}

static void MoveLabel(ObjNode* node)
{
	node->ColorFilter.a = gNav->menuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveAction(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow)
		node->ColorFilter = TwinkleColor();
	else
		node->ColorFilter = gNav->style.inactiveColor;

	node->ColorFilter.a *= gNav->menuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveKeyBinding(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow && node->SpecialCol == (gNav->keyColumn+1))
	{
		if (gNav->menuState == kMenuStateAwaitingKeyPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gNav->style.inactiveColor;

	node->ColorFilter.a *= gNav->menuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MovePadBinding(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow && node->SpecialCol == (gNav->padColumn+1))
	{
		if (gNav->menuState == kMenuStateAwaitingPadPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gNav->style.inactiveColor;

	node->ColorFilter.a *= gNav->menuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveMouseBinding(ObjNode* node)
{
	if (node->SpecialRow == gNav->menuRow)
	{
		if (gNav->menuState == kMenuStateAwaitingMouseClick)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gNav->style.inactiveColor;

	node->ColorFilter.a *= gNav->menuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveAsyncFadeOutAndDelete(ObjNode *theNode)
{
	theNode->ColorFilter.a -= gFramesPerSecondFrac * 3.0f;
	if (theNode->ColorFilter.a < 0.0f)
		DeleteObject(theNode);
}

/****************************/
/*    MENU CALLBACKS        */
/****************************/
#pragma mark - Callbacks

void MenuCallback_Back(const MenuItem* mi)
{
	MyFlushEvents();

	if (gNav->historyPos != 0)
	{
		PlayEffect(kSfxBack);
		gNav->historyPos--;
		LayOutMenu(gNav->history[gNav->historyPos].menu);
	}
	else if (gNav->style.canBackOutOfRootMenu)
	{
		PlayEffect(kSfxBack);
		gNav->menuState = kMenuStateFadeOut;
	}
	else
	{
		PlayEffect(kSfxError);
	}
}

/****************************/
/*    MENU NAVIGATION       */
/****************************/
#pragma mark - Menu navigation

static void NavigateSettingEntriesVertically(int delta)
{
	bool makeSound = gNav->menuRow >= 0;
	int browsed = 0;
	bool skipEntry = false;

	do
	{
		gNav->menuRow += delta;
		gNav->menuRow = PositiveModulo(gNav->menuRow, (unsigned int)gNav->numMenuEntries);

		skipEntry = !IsMenuItemSelectable(&gNav->menu[gNav->menuRow]);

		if (browsed++ > gNav->numMenuEntries)
		{
			// no entries are selectable
			return;
		}
	} while (skipEntry);

	gNav->idleTime = 0;
	if (makeSound)
		PlayEffect(kSfxNavigate);
	gNav->mouseHoverValidRow = false;
}

static void NavigateSettingEntriesMouseHover(void)
{
	static bool todoShown = false;
	if (!todoShown)
	{
		printf("TODO: %s\n", __func__);
		todoShown = true;
	}
#if 0
	if (!gMouseMotionNow)
		return;

	int mxRaw, myRaw;
	SDL_GetMouseState(&mxRaw, &myRaw);

	float mx = (mxRaw - gGameWindowWidth/2.0f) * g2DLogicalWidth / gGameWindowWidth;
	float my = (myRaw - gGameWindowHeight/2.0f) * g2DLogicalHeight / gGameWindowHeight;

	gNav->mouseHoverValidRow = false;
	gNav->mouseHoverValidColumn = -1;

	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (!IsMenuItemTypeSelectable(gNav->menu[row].type))
			continue;

		OGLRect fullExtents;
		fullExtents.top		= fullExtents.left	= 100000;
		fullExtents.bottom	= fullExtents.right	= -100000;

		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			ObjNode* textNode = gNav->menuObjects[row][col];
			if (!textNode)
				continue;

			OGLRect extents = TextMesh_GetExtents(textNode);
			if (extents.top		< fullExtents.top	) fullExtents.top		= extents.top;
			if (extents.left	< fullExtents.left	) fullExtents.left		= extents.left;
			if (extents.bottom	> fullExtents.bottom) fullExtents.bottom	= extents.bottom;
			if (extents.right	> fullExtents.right	) fullExtents.right		= extents.right;

			if (my >= extents.top
				&& my <= extents.bottom
				&& mx >= extents.left - 10
				&& mx <= extents.right + 10)
			{
				gNav->mouseHoverValidColumn = col;
			}
		}

		if (my >= fullExtents.top &&
			my <= fullExtents.bottom &&
			mx >= fullExtents.left - 10 &&
			mx <= fullExtents.right + 10)
		{
			gNav->mouseHoverValidRow = true;

			if (SDL_GetCursor() != gHandCursor)
			{
				SDL_SetCursor(gHandCursor);
			}

			if (gNav->menuRow != row)
			{
				gNav->menuRow = row;
//				PlayEffect(kSfxNavigate);
			}

			return;
		}
	}

	GAME_ASSERT(!gNav->mouseHoverValidRow);		// if we got here, we're not hovering over anything

	if (SDL_GetCursor() != gStandardCursor)
		SDL_SetCursor(gStandardCursor);
#endif
}

static void NavigatePick(const MenuItem* entry)
{
	if (GetNewNeedStateAnyP(kNeed_UIConfirm) || (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;

		if (entry->callback != MenuCallback_Back)
			PlayConfirmEffect();
		else if (gNav->style.playMenuChangeSounds)
			PlayEffect(kSfxMenuChange);

		gNav->menuPick = entry->id;

		if (entry->callback)
			entry->callback(entry);

		if (entry->gotoMenu > 0)
		{
			const MenuItem* newMenu = gNav->menuTree[entry->gotoMenu];

			gNav->history[gNav->historyPos].row = gNav->menuRow;  // remember which row we were on

			// advance history
			gNav->historyPos++;
			gNav->history[gNav->historyPos].menu = newMenu;
			gNav->history[gNav->historyPos].row = 0;  // don't reuse stale row value

			LayOutMenu(newMenu);
		}
		else if (entry->gotoMenu < 0)
		{
			// Exit
			gNav->menuState = kMenuStateFadeOut;
		}
	}
}

static int GetCyclerNumChoices(const MenuItem* entry)
{
	for (int i = 0; i < MAX_MENU_CYCLER_CHOICES; i++)
	{
		if (entry->cycler.choices[i].text == STR_NULL)
			return i;
	}

	return MAX_MENU_CYCLER_CHOICES;
}

static int GetValueIndexInCycler(const MenuItem* entry, uint8_t value)
{
	for (int i = 0; i < MAX_MENU_CYCLER_CHOICES; i++)
	{
		if (entry->cycler.choices[i].text == STR_NULL)
			break;

		if (entry->cycler.choices[i].value == value)
			return i;
	}

	return -1;
}

static void NavigateCycler(const MenuItem* entry)
{
	int delta = 0;

	if (GetNewNeedStateAnyP(kNeed_UILeft)
		|| GetNewNeedStateAnyP(kNeed_UIPrev)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_RIGHT)))
	{
		delta = -1;
	}
	else if (GetNewNeedStateAnyP(kNeed_UIRight)
		|| GetNewNeedStateAnyP(kNeed_UINext)
		|| GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		delta = 1;
	}

	if (delta != 0)
	{
		gNav->idleTime = 0;
		PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3 + (RandomFloat2() * 0x3000));

		if (entry->cycler.valuePtr && !entry->cycler.callbackSetsValue)
		{
			int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);
			if (index >= 0)
				index = PositiveModulo(index + delta, GetCyclerNumChoices(entry));
			else
				index = 0;
			*entry->cycler.valuePtr = entry->cycler.choices[index].value;
		}

		if (entry->callback)
			entry->callback(entry);

		if (entry->type == kMenuItem_CMRCycler)
			LayOutCMRCycler(gNav->menuRow, 0);
		else
			LayOutCyclerValueText(gNav->menuRow);
	}
}

static void NavigateKeyBinding(const MenuItem* entry)
{
	if (gNav->mouseHoverValidRow && (gNav->mouseHoverColumn == 1 || gNav->mouseHoverColumn == 2))
		gNav->keyColumn = gNav->mouseHoverColumn - 1;

	if (GetNewNeedStateAnyP(kNeed_UILeft) || GetNewNeedStateAnyP(kNeed_UIPrev))
	{
		gNav->idleTime = 0;
		gNav->keyColumn = PositiveModulo(gNav->keyColumn - 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValidRow = false;
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIRight) || GetNewNeedStateAnyP(kNeed_UINext))
	{
		gNav->idleTime = 0;
		gNav->keyColumn = PositiveModulo(gNav->keyColumn + 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValidRow = false;
		return;
	}

	// Past this point we must have a valid column
	if (gNav->keyColumn < 0 || gNav->keyColumn >= KEYBINDING_MAX_KEYS)
	{
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.keys[entry->kb][0].key[gNav->keyColumn] = 0;
		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gNav->menuRow, gNav->keyColumn+1);
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;
		gNav->menuState = kMenuStateAwaitingKeyPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gNav->menuRow, gNav->keyColumn+1);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP_CANCEL);

		return;
	}
}

static void NavigatePadBinding(const MenuItem* entry)
{
	if (gNav->mouseHoverValidRow && (gNav->mouseHoverColumn == 1 || gNav->mouseHoverColumn == 2))
		gNav->padColumn = gNav->mouseHoverColumn - 1;

	if (GetNewNeedStateAnyP(kNeed_UILeft) || GetNewNeedStateAnyP(kNeed_UIPrev))
	{
		gNav->idleTime = 0;
		gNav->padColumn = PositiveModulo(gNav->padColumn - 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValidRow = false;
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIRight) || GetNewNeedStateAnyP(kNeed_UINext))
	{
		gNav->idleTime = 0;
		gNav->padColumn = PositiveModulo(gNav->padColumn + 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		gNav->mouseHoverValidRow = false;
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.keys[entry->kb][0].gamepad[gNav->padColumn].type = kInputTypeUnbound;
		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gNav->menuRow, gNav->padColumn+1);
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;
		while (GetNeedStateAnyP(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		gNav->menuState = kMenuStateAwaitingPadPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gNav->menuRow, gNav->padColumn+1);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP_CANCEL);

		return;
	}
}

static void NavigateMouseBinding(const MenuItem* entry)
{
	if (GetNewNeedStateAnyP(kNeed_UIDelete)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_MIDDLE)))
	{
		gNav->idleTime = 0;
		gGamePrefs.keys[entry->kb][0].mouseButton = 0;
		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gNav->menuRow, 1);
		return;
	}

	if (GetNewNeedStateAnyP(kNeed_UIConfirm)
		|| (gNav->mouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		gNav->idleTime = 0;
		while (GetNeedStateAnyP(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		gNav->menuState = kMenuStateAwaitingMouseClick;
		MakeTextAtRowCol(Localize(STR_CLICK), gNav->menuRow, 1);
		return;
	}
}

static void NavigateMenu(void)
{
	GAME_ASSERT(gNav->style.isInteractive);

	if (GetNewNeedStateAnyP(kNeed_UIBack))
	{
		MenuCallback_Back(NULL);
	}

	if (GetNewNeedStateAnyP(kNeed_UIUp))
	{
		NavigateSettingEntriesVertically(-1);
	}

	if (GetNewNeedStateAnyP(kNeed_UIDown))
	{
		NavigateSettingEntriesVertically(1);
	}

	NavigateSettingEntriesMouseHover();

	const MenuItem* entry = &gNav->menu[gNav->menuRow];

	switch (entry->type)
	{
//		case kMenuItem_Action:
//			NavigateAction(entry);
//			break;

		case kMenuItem_Pick:
			NavigatePick(entry);
			break;

//		case kMenuItem_Submenu:
//			NavigateSubmenuButton(entry);
//			break;

		case kMenuItem_Cycler:
		case kMenuItem_CMRCycler:
			NavigateCycler(entry);
			break;

		case kMenuItem_KeyBinding:
			NavigateKeyBinding(entry);
			break;

		case kMenuItem_PadBinding:
			NavigatePadBinding(entry);
			break;

		case kMenuItem_MouseBinding:
			NavigateMouseBinding(entry);

		default:
			//DoFatalAlert("Not supposed to be hovering on this menu item!");
			break;
	}
}

/****************************/
/* INPUT BINDING CALLBACKS  */
/****************************/
#pragma mark - Input binding callbacks

static void UnbindScancodeFromAllRemappableInputNeeds(int16_t sdlScancode)
{
	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (gNav->menu[row].type != kMenuItem_KeyBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			if (binding->key[j] == sdlScancode)
			{
				binding->key[j] = 0;
				MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1);
			}
		}
	}
}

static void UnbindPadButtonFromAllRemappableInputNeeds(int8_t type, int8_t id)
{
	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (gNav->menu[row].type != kMenuItem_PadBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
		{
			if (binding->gamepad[j].type == type && binding->gamepad[j].id == id)
			{
				binding->gamepad[j].type = kInputTypeUnbound;
				binding->gamepad[j].id = 0;
				MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1);
			}
		}
	}
}

static void UnbindMouseButtonFromAllRemappableInputNeeds(int8_t id)
{
	for (int row = 0; row < gNav->numMenuEntries; row++)
	{
		if (gNav->menu[row].type != kMenuItem_MouseBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		if (binding->mouseButton == id)
		{
			binding->mouseButton = 0;
			MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, 1);
		}
	}
}

static void AwaitKeyPress(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetKeyBindingName(gNav->menuRow, gNav->keyColumn), gNav->menuRow, 1 + gNav->keyColumn);
		gNav->menuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gNav->menuRow);

	for (int16_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
	{
		if (GetNewKeyState(scancode))
		{
			UnbindScancodeFromAllRemappableInputNeeds(scancode);
			kb->key[gNav->keyColumn] = scancode;
			MakeTextAtRowCol(GetKeyBindingName(gNav->menuRow, gNav->keyColumn), gNav->menuRow, gNav->keyColumn+1);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
			return;
		}
	}
}

#if 0
static void AwaitPadPress(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE)
		|| SDL_GameControllerGetButton(gSDLController, SDL_CONTROLLER_BUTTON_START))
	{
		MakeTextAtRowCol(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, 1 + gNav->padColumn);
		gNav->menuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gNav->menuRow);

	if (!gSDLController)
		return;

	for (int8_t button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
	{
		switch (button)
		{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:			// prevent binding those
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				continue;
		}

		if (SDL_GameControllerGetButton(gSDLController, button))
		{
			UnbindPadButtonFromAllRemappableInputNeeds(kInputTypeButton, button);
			kb->gamepad[gNav->padColumn].type = kInputTypeButton;
			kb->gamepad[gNav->padColumn].id = button;
			MakeTextAtRowCol(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, gNav->padColumn+1);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
			return;
		}
	}

	for (int8_t axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		switch (axis)
		{
			case SDL_CONTROLLER_AXIS_LEFTX:				// prevent binding those
			case SDL_CONTROLLER_AXIS_LEFTY:
			case SDL_CONTROLLER_AXIS_RIGHTX:
			case SDL_CONTROLLER_AXIS_RIGHTY:
				continue;
		}

		int16_t axisValue = SDL_GameControllerGetAxis(gSDLController, axis);
		if (abs(axisValue) > kJoystickDeadZone_BindingThreshold)
		{
			int axisType = axisValue < 0 ? kInputTypeAxisMinus : kInputTypeAxisPlus;
			UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
			kb->gamepad[gNav->padColumn].type = axisType;
			kb->gamepad[gNav->padColumn].id = axis;
			MakeTextAtRowCol(GetPadBindingName(gNav->menuRow, gNav->padColumn), gNav->menuRow, gNav->padColumn+1);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP_CANCEL, STR_CONFIGURE_GAMEPAD_HELP);
			return;
		}
	}
}
#endif

static void AwaitMouseClick(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetMouseBindingName(gNav->menuRow), gNav->menuRow, 1);
		gNav->menuState = kMenuStateReady;
		gNav->idleTime = 0;
		PlayEffect(kSfxError);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gNav->menuRow);

	for (int8_t mouseButton = 0; mouseButton < NUM_SUPPORTED_MOUSE_BUTTONS; mouseButton++)
	{
		if (FlushMouseButtonPress(mouseButton))
		{
			UnbindMouseButtonFromAllRemappableInputNeeds(mouseButton);
			kb->mouseButton = mouseButton;
			MakeTextAtRowCol(GetMouseBindingName(gNav->menuRow), gNav->menuRow, 1);
			gNav->menuState = kMenuStateReady;
			gNav->idleTime = 0;
			PlayEffect(kSfxCycle);
			return;
		}
	}
}

/****************************/
/*    PAGE LAYOUT           */
/****************************/
#pragma mark - Page Layout

#if 0
static ObjNode* MakeDarkenPane(void)
{
	ObjNode* pane;

	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOFOG
				| STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_MOVEINPAUSE,
		.slot = SLOT_OF_DUMB + 100 - 1,
		.scale = 1,
	};

	pane = MakeNewObject(&def);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, 0};
	pane->Scale.y = gNav->style.darkenPaneScaleY;
	pane->MoveCall = MoveDarkenPane;

	return pane;
}
#endif

static void DeleteAllText(void)
{
	for (int row = 0; row < MAX_MENU_ROWS; row++)
	{
		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			if (gNav->menuObjects[row][col])
			{
				DeleteObject(gNav->menuObjects[row][col]);
				gNav->menuObjects[row][col] = nil;
			}
		}
	}
}

static ObjNode* MakeTextAtRowCol(const char* text, int row, int col)
{
	ObjNode* node = gNav->menuObjects[row][col];

	int miType = gNav->menu[row].type;

	if (node)
	{
		// Recycling existing text lets us keep the move call, color and specials
		TextMesh_Update(text, 0, node);
	}
	else
	{
		float startX = gNav->style.centeredText ? 0 : -170;

		NewObjectDefinitionType def =
		{
			.coord = (OGLPoint3D) { startX + gNav->menuColXs[col], gNav->menuRowYs[row], 0 },
			.scale = gNav->style.standardScale * kMenuItemHeightMultipliers[miType],
			.slot = SLOT_OF_DUMB + 100,
		};

		int alignment = gNav->style.centeredText? kTextMeshAlignCenter: kTextMeshAlignLeft;
		node = TextMesh_New(text, alignment, &def);
		node->SpecialRow = row;
		node->SpecialCol = col;
//		node->StatusBits |= STATUS_BIT_MOVEINPAUSE;
		gNav->menuObjects[row][col] = node;
	}

	if (!gNav->style.centeredText)
	{
		int paddedRightOff = ((gNav->menuColXs[col+1]-170) - node->Coord.x) / node->Scale.x;
		if (paddedRightOff > node->RightOff)
			node->RightOff = paddedRightOff;
	}

	if (gNav->style.uniformXExtent)
	{
		if (-gNav->style.uniformXExtent < node->LeftOff)
			node->LeftOff = -gNav->style.uniformXExtent;
		if (gNav->style.uniformXExtent > node->RightOff)
			node->RightOff = gNav->style.uniformXExtent;
	}

	return node;
}

static const char* GetMenuItemLabel(const MenuItem* entry)
{
	if (entry->rawText)
		return entry->rawText;
	else if (entry->generateText)
		return entry->generateText();
	else
		return Localize(entry->text);
}

static const char* GetCyclerValueText(int row)
{
	const MenuItem* entry = &gNav->menu[row];
	int index = GetValueIndexInCycler(entry, *entry->cycler.valuePtr);
	if (index >= 0)
		return Localize(entry->cycler.choices[index].text);
	return "VALUE NOT FOUND???";
}

static ObjNode* LayOutCyclerValueText(int row)
{
	ObjNode* node2 = MakeTextAtRowCol(GetCyclerValueText(row), row, 1);
	node2->MoveCall = MoveAction;
	return node2;
}

static void LayOutCycler(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s:", GetMenuItemLabel(entry));

	ObjNode* node1 = MakeTextAtRowCol(buf, row, 0);
	node1->MoveCall = MoveAction;
	node1->SpecialSweepTimer = sweepFactor;

	ObjNode* node2 = LayOutCyclerValueText(row);
	node2->SpecialSweepTimer = sweepFactor;
}

static void LayOutCMRCycler(int row, float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gNav->menu[row];

	snprintf(buf, bufSize, "%s: %s", GetMenuItemLabel(entry), GetCyclerValueText(row));

	ObjNode* node = MakeTextAtRowCol(buf, row, 0);
	node->MoveCall = MoveAction;
	node->SpecialSweepTimer = sweepFactor;
}

static void LayOutMenu(const MenuItem* menu)
{
	DECLARE_WORKBUF(buf, bufSize);

	// Remember we've been to this menu
	gNav->history[gNav->historyPos].menu = menu;

	gNav->menu			= menu;
	gNav->numMenuEntries	= 0;
	gNav->menuPick		= -1;
	gNav->idleTime	= 0;

	// Restore old row
	gNav->menuRow = gNav->history[gNav->historyPos].row;

	DeleteAllText();

	float totalHeight = 0;
	for (int row = 0; menu[row].type != kMenuItem_END_SENTINEL; row++)
	{
		totalHeight += kMenuItemHeightMultipliers[menu[row].type] * gNav->style.rowHeight;
	}

	float y = -0.1f + totalHeight/2.0f;

	float sweepFactor = 0.0f;

	for (int row = 0; menu[row].type != kMenuItem_END_SENTINEL; row++)
	{
		gNav->menuRowYs[row] = y;

		const MenuItem* entry = &menu[row];

		switch (entry->type)
		{
			case kMenuItem_Spacer:
				break;

			case kMenuItem_Title:
			{
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gNav->style.titleColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest
				break;
			}

			case kMenuItem_Subtitle:
			{
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				//label->ColorFilter = gNav->style.titleColor;
				label->ColorFilter = (OGLColorRGBA){ 0.7, .4, .2, 1 };
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest
				break;
			}

			case kMenuItem_Label:
			{
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = (OGLColorRGBA){ 0.7, .4, .2, 1 };//gNav->style.inactiveColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;
				break;
			}

//			case kMenuItem_Action:
			case kMenuItem_Pick:
//			case kMenuItem_Submenu:
			{
				ObjNode* node = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				node->MoveCall = MoveAction;
				node->SpecialSweepTimer = sweepFactor;
				node->SpecialMuted = !IsMenuItemSelectable(entry);
				break;
			}

			case kMenuItem_Cycler:
			{
				LayOutCycler(row, sweepFactor);
				break;
			}

			case kMenuItem_CMRCycler:
			{
				LayOutCMRCycler(row, sweepFactor);
				break;
			}

			case kMenuItem_KeyBinding:
			{
				snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gNav->style.inactiveColor2;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;

				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetKeyBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MoveKeyBinding;
					keyNode->SpecialSweepTimer = sweepFactor;
				}
				break;
			}

			case kMenuItem_PadBinding:
			{
				snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gNav->style.inactiveColor2;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;

				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetPadBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MovePadBinding;
					keyNode->SpecialSweepTimer = sweepFactor;
				}
				break;
			}

			case kMenuItem_MouseBinding:
			{
				snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gNav->style.inactiveColor2;
				label->MoveCall = MoveAction;
				label->SpecialSweepTimer = sweepFactor;

				ObjNode* keyNode = MakeTextAtRowCol(GetMouseBindingName(row), row, 1);
				keyNode->MoveCall = MoveMouseBinding;
				keyNode->SpecialSweepTimer = sweepFactor;
				break;
			}

			default:
				DoFatalAlert("Unsupported menu item type");
				break;
		}

		y -= kMenuItemHeightMultipliers[entry->type] * gNav->style.rowHeight;

		if (entry->type != kMenuItem_Spacer)
			sweepFactor -= .2f;

		gNav->numMenuEntries++;
		GAME_ASSERT(gNav->numMenuEntries < MAX_MENU_ROWS);
	}

	if (gNav->menuRow <= 0
		|| !IsMenuItemSelectable(&gNav->menu[gNav->menuRow]))	// we had selected this item when we last were in this menu, but it's been disabled since then
	{
		// Scroll down to first pickable entry
		gNav->menuRow = -1;
		NavigateSettingEntriesVertically(1);
	}
}

void LayoutCurrentMenuAgain(void)
{
	GAME_ASSERT(gNav->menu);
	LayOutMenu(gNav->menu);
}

int StartMenuTree(
		const MenuItem** menuTree,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(OGLSetupOutputType *))
{
//	int cursorStateBeforeMenu = SDL_ShowCursor(-1);
//	gStandardCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
//	gHandCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	SDL_ShowCursor(1);

		/* INITIALIZE MENU STATE */

	InitMenuNavigation();
	if (style)
		memcpy(&gNav->style, style, sizeof(*style));
	gNav->menuTree			= menuTree;
	gNav->rootMenu			= menuTree[0];
	gNav->menuState			= kMenuStateFadeIn;
	gNav->menuFadeAlpha		= 0;
	gNav->menuRow			= -1;

		/* LAY OUT MENU COMPONENTS */

	ObjNode* pane = nil;

#if 0
	if (gNav->style.darkenPane)
		pane = MakeDarkenPane();
#endif

	LayOutMenu(gNav->rootMenu);

		/* SHOW IN ANIMATED LOOP */

	while (gNav->menuState != kMenuStateOff)
	{
		UpdateInput();

		gNav->idleTime += gFramesPerSecondFrac;

		if (gNav->style.startButtonExits && gNav->style.canBackOutOfRootMenu && GetNewNeedStateAnyP(kNeed_UIStart))
			gNav->menuState = kMenuStateFadeOut;

		switch (gNav->menuState)
		{
			case kMenuStateFadeIn:
				gNav->menuFadeAlpha += gFramesPerSecondFrac * gNav->style.fadeInSpeed;
				if (gNav->menuFadeAlpha >= 1.0f)
				{
					gNav->menuFadeAlpha = 1.0f;
					gNav->menuState = kMenuStateReady;
				}
				break;

			case kMenuStateFadeOut:
				if (gNav->style.asyncFadeOut)
				{
					gNav->menuState = kMenuStateOff;		// exit loop
				}
				else
				{
					gNav->menuFadeAlpha -= gFramesPerSecondFrac * 2.0f;
					if (gNav->menuFadeAlpha <= 0.0f)
					{
						gNav->menuFadeAlpha = 0.0f;
						gNav->menuState = kMenuStateOff;
					}
				}
				break;

			case kMenuStateReady:
				if (gNav->style.isInteractive)
				{
					NavigateMenu();
				}
				else if (UserWantsOut())
				{
					MenuCallback_Back(NULL);
				}
				break;

			case kMenuStateAwaitingKeyPress:
				AwaitKeyPress();
				break;

			case kMenuStateAwaitingPadPress:
				//AwaitPadPress();
				IMPLEMENT_ME_SOFT();
				break;

			case kMenuStateAwaitingMouseClick:
				AwaitMouseClick();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		if (updateRoutine)
			updateRoutine();
		OGL_DrawScene(gGameViewInfoPtr, backgroundDrawRoutine);
	}


		/* FADE OUT */

	if (gNav->style.fadeOutSceneOnExit)
	{
		OGL_FadeOutScene(gGameViewInfoPtr, backgroundDrawRoutine, NULL);
	}



		/* CLEANUP */

	if (gNav->style.asyncFadeOut)
	{
		if (pane)
			pane->MoveCall = MoveAsyncFadeOutAndDelete;

		for (int row = 0; row < MAX_MENU_ROWS; row++)
		{
			for (int col = 0; col < MAX_MENU_COLS; col++)
			{
				if (gNav->menuObjects[row][col])
					gNav->menuObjects[row][col]->MoveCall = MoveAsyncFadeOutAndDelete;
			}
		}

		memset(gNav->menuObjects, 0, sizeof(gNav->menuObjects));
	}
	else
	{
		DeleteAllText();

		if (pane)
			DeleteObject(pane);
	}

	UpdateInput();
	MyFlushEvents();

	int finalPick = gNav->menuPick;
	DisposeMenuNavigation();

#if 0
	SDL_SetCursor(gStandardCursor);
	SDL_FreeCursor(gStandardCursor);
	SDL_FreeCursor(gHandCursor);
	gStandardCursor = nil;
	gHandCursor = nil;

	SDL_ShowCursor(cursorStateBeforeMenu);
#endif

	return finalPick;
}
