/****************************/
/*     SETTINGS.C           */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"
#include "menu.h"

static void OnPickLanguage(const MenuItem* mi)
{
	LoadLocalizedStrings(gGamePrefs.language);
	LayoutCurrentMenuAgain();
}

static void OnToggleFullscreen(const MenuItem* mi)
{
	SetFullscreenMode(true);
}

static void OnToggleMusic(const MenuItem* mi)
{
	if ((!gMuteMusicFlag) != gGamePrefs.music)
	{
		ToggleMusic();
	}
}

static void OnAdjustMusicVolume(const MenuItem* mi)
{
	UpdateGlobalVolume();
}

static void OnAdjustSFXVolume(const MenuItem* mi)
{
	UpdateGlobalVolume();
}

static void OnPickResetKeyboardBindings(const MenuItem* mi)
{
	MyFlushEvents();
	ResetDefaultKeyboardBindings();
	PlayEffect(EFFECT_BOOM);
	LayoutCurrentMenuAgain();
}

static void OnPickResetGamepadBindings(const MenuItem* mi)
{
	MyFlushEvents();
	ResetDefaultGamepadBindings();
	PlayEffect(EFFECT_BOOM);
	LayoutCurrentMenuAgain();
}

const MenuItem gSettingsMenuTree[] =
{
	{.id='sett'},
	{kMIPick, STR_CONFIGURE_KEYBOARD, .next='keyb', .customHeight=.75f },
	{kMIPick, STR_CONFIGURE_GAMEPAD, .next='gpad', .customHeight=.75f},
	{
			kMICycler1, STR_MUSIC,
		.callback=OnAdjustMusicVolume,
		.cycler=
		{
			.valuePtr=&gGamePrefs.musicVolumePercent,
			.choices=
			{
				{STR_VOLUME_000, 0},
				{STR_VOLUME_020, 20},
				{STR_VOLUME_040, 40},
				{STR_VOLUME_060, 60},
				{STR_VOLUME_080, 80},
				{STR_VOLUME_100, 100},
			}
		},
		.customHeight=.75f
	},
	{
			kMICycler1, STR_SFX,
		.callback=OnAdjustSFXVolume,
		.cycler=
		{
			.valuePtr=&gGamePrefs.sfxVolumePercent,
			.choices=
			{
				{STR_VOLUME_000, 0},
				{STR_VOLUME_020, 20},
				{STR_VOLUME_040, 40},
				{STR_VOLUME_060, 60},
				{STR_VOLUME_080, 80},
				{STR_VOLUME_100, 100},
			}
		},
		.customHeight=.75f
	},
	{
			kMICycler1, STR_FULLSCREEN,
		.callback=OnToggleFullscreen,
		.cycler=
		{
			.valuePtr=&gGamePrefs.fullscreen,
			.choices={ {STR_OFF, 0}, {STR_ON, 1} },
		},
		.customHeight=.75f
	},
	{
			kMICycler1, STR_LANGUAGE, .cycler=
		{
			.valuePtr=&gGamePrefs.language, .choices=
			{
				{STR_LANGUAGE_NAME, LANGUAGE_ENGLISH},
				{STR_LANGUAGE_NAME, LANGUAGE_FRENCH},
				{STR_LANGUAGE_NAME, LANGUAGE_GERMAN},
				{STR_LANGUAGE_NAME, LANGUAGE_SPANISH},
				{STR_LANGUAGE_NAME, LANGUAGE_ITALIAN},
				{STR_LANGUAGE_NAME, LANGUAGE_SWEDISH},
			}
		},
		.callback=OnPickLanguage,
		.customHeight=.75f
	},

	{ .id='keyb' },
	{kMISpacer, .customHeight=.2f },
	{kMISubtitle, STR_CONFIGURE_KEYBOARD_HELP, .customHeight=.5f },
	{kMISpacer, .customHeight=.4f },
	{kMIKeyBinding, .inputNeed=kNeed_Forward },
	{kMIKeyBinding, .inputNeed=kNeed_Backward },
	{kMIKeyBinding, .inputNeed=kNeed_Left },
	{kMIKeyBinding, .inputNeed=kNeed_Right },
	{kMIKeyBinding, .inputNeed=kNeed_Brakes },
	{kMIKeyBinding, .inputNeed=kNeed_ThrowForward },
	{kMIKeyBinding, .inputNeed=kNeed_ThrowBackward },
	{kMIKeyBinding, .inputNeed=kNeed_CameraMode },
	{kMIKeyBinding, .inputNeed=kNeed_RearView },
	{kMISpacer, .customHeight=.25f },
	{kMIPick, STR_RESET_KEYBINDINGS, .callback=OnPickResetKeyboardBindings, .customHeight=.5f },

	{ .id='gpad' },
	{kMISpacer, .customHeight=.2f },
	{kMISubtitle,         STR_CONFIGURE_GAMEPAD_HELP, .customHeight=.5f },
	{kMISpacer, .customHeight=.4f },
	{kMIPadBinding, .inputNeed=kNeed_Forward },
	{kMIPadBinding, .inputNeed=kNeed_Backward },
	{kMIPadBinding, .inputNeed=kNeed_Left },
	{kMIPadBinding, .inputNeed=kNeed_Right },
	{kMIPadBinding, .inputNeed=kNeed_Brakes },
	{kMIPadBinding, .inputNeed=kNeed_ThrowForward },
	{kMIPadBinding, .inputNeed=kNeed_ThrowBackward },
	{kMIPadBinding, .inputNeed=kNeed_CameraMode },
	{kMIPadBinding, .inputNeed=kNeed_RearView },
	{kMISpacer, .customHeight=.25f },
	{kMIPick, STR_RESET_KEYBINDINGS, .callback=OnPickResetGamepadBindings, .customHeight=.5f },

	{ .id=0 },
};

void RegisterSettingsMenu(const MenuItem* mi)
{
	RegisterMenu(gSettingsMenuTree);
}