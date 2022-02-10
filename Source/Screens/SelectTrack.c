/****************************/
/*   	SELECT TRACK.C 	    */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "miscscreens.h"
#include "objects.h"
#include "window.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include "ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "skeletonobj.h"
#include "terrain.h"
#include "camera.h"
#include "sobjtypes.h"
#include "selecttrack.h"
#include "network.h"
#include "player.h"
#include "sprites.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec			gDataSpec;
extern	KeyMap 			gKeyMap,gNewKeys;
extern	short			gNumRealPlayers,gNumLocalPlayers;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean			gSongPlayingFlag,gDisableAnimSounds,gSongPlayingFlag;
extern	OGLPoint3D		gCoord;
extern  MOPictureObject 	*gBackgoundPicture;
extern	Boolean					gNetGameInProgress,gIsNetworkClient;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gDelta;
extern	int				gTrackNum, gGameMode;
extern	SavePlayerType	gPlayerSaveData;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupTrackSelectScreen(void);
static Boolean DoTrackSelectControls(void);
static void DrawTrackSelectCallback(OGLSetupOutputType *info);
static void FreeTrackSelectArt(void);
static void MakeTrackName(void);



/****************************/
/*    CONSTANTS             */
/****************************/


enum
{
	TRACKSELECT_SObjType_Padlock,

	TRACKSELECT_SObjType_Arrow_LeftOn,
	TRACKSELECT_SObjType_Arrow_RightOn,

	TRACKSELECT_SObjType__Level0,			// stone age
	TRACKSELECT_SObjType__Level1,
	TRACKSELECT_SObjType__Level2,

	TRACKSELECT_SObjType__Level3,			// bronze age
	TRACKSELECT_SObjType__Level4,
	TRACKSELECT_SObjType__Level5,

	TRACKSELECT_SObjType__Level6,			// iron age
	TRACKSELECT_SObjType__Level7,
	TRACKSELECT_SObjType__Level8,

	TRACKSELECT_SObjType__Arena0,			// stone henge
	TRACKSELECT_SObjType__Arena1,
	TRACKSELECT_SObjType__Arena2,			// aztec
	TRACKSELECT_SObjType__Arena3,			// maze
	TRACKSELECT_SObjType__Arena4,
	TRACKSELECT_SObjType__Arena5,
	TRACKSELECT_SObjType__Arena6,
	TRACKSELECT_SObjType__Arena7
};


#define	LEFT_ARROW_X	-.65
#define RIGHT_ARROW_X	.65
#define	ARROW_Y			0
#define	ARROW_SCALE		.5

#define	LEVEL_IMAGE_X		-.01
#define	LEVEL_IMAGE_Y		-.15
#define	LEVEL_IMAGE_SCALE	.58


#define	NUM_PRACTICE_TRACKS		9
#define	NUM_BATTLE_TRACKS		8

/*********************/
/*    VARIABLES      */
/*********************/

static int		gSelectedTrackIndex;
static ObjNode	*gTrackImageIcon, *gTrackName = nil;

const Str31	gTrackNames[] =
{
	"DESERT",
	"JUNGLE",
	"GLACIERS",

	"CRETE",
	"THE GREAT WALL",
	"GIZA",

	"MEDIEVAL",
	"VIKING VILLAGE",
	"ATLANTIS",
};


const Str31	gTrackNamesMP[] =
{
	"STONEHENGE",
	"AZTEC CITY",
	"COLISEUM",
	"MAZE",
	"CELTIC",
	"TAR PITS",
	"MOUNT CONGE",
	"RAMPS",
};


static	short	gNumTracksInSelection;
static 	short	gBaseTrack;

const short gNumTracksUnlocked[] = {3,6,9,9};


/********************** SELECT PRACTICE TRACK **************************/
//
// Return true if user wants to back up a menu
//

Boolean SelectSingleTrack(void)
{
	SetupTrackSelectScreen();
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (DoTrackSelectControls())
			break;


			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawTrackSelectCallback);
	}


			/***********/
			/* CLEANUP */
			/***********/

	GammaFadeOut();
	FreeTrackSelectArt();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);

	if (gSelectedTrackIndex == -1)								// see if bail
		return(true);

		/* CONVERT THE INDEX INTO A GLOGAL TRACK NUMBER */

	gTrackNum = gBaseTrack + gSelectedTrackIndex;				// set global track # to play
	return(false);
}


/********************* SETUP TRACKSELECT SCREEN **********************/

static void SetupTrackSelectScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;


	gSelectedTrackIndex = 0;
	gTrackName = nil;


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef, gCoverWindow);
	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);



				/************/
				/* LOAD ART */
				/************/

			/* MAKE BACKGROUND PICTURE OBJECT */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:TrackSelectScreen", &spec))
		DoFatalAlert("SetupTrackSelectScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("SetupTrackSelectScreen: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
		case	GAME_MODE_MULTIPLAYERRACE:
#if DEMO
				gNumTracksInSelection = 2; 		// only 2 tracks in DEMO
#else
				gNumTracksInSelection = NUM_PRACTICE_TRACKS;
#endif
				gBaseTrack = 0;
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:trackselect.sprites", &spec);
				break;

		default:
#if DEMO
				gNumTracksInSelection = 2;
#else
				gNumTracksInSelection = NUM_BATTLE_TRACKS;
#endif
				gBaseTrack = 9;
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:trackselectMP.sprites", &spec);
	}
	LoadSpriteFile(&spec, SPRITE_GROUP_TRACKSELECTSCREEN, gGameViewInfoPtr);


	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:wallfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);



			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* TRACK IMAGE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_TRACKSELECTSCREEN;
	gNewObjectDefinition.type 		= TRACKSELECT_SObjType__Level0 + gSelectedTrackIndex;
	gNewObjectDefinition.coord.x 	= LEVEL_IMAGE_X;
	gNewObjectDefinition.coord.y 	= LEVEL_IMAGE_Y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = LEVEL_IMAGE_SCALE;

	gTrackImageIcon = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);



			/* TRACK NAME */

	MakeTrackName();

}

/******************** MAKE TRACK NAME ************************/

static void MakeTrackName(void)
{
	if (gTrackName)
	{
		DeleteObject(gTrackName);
		gTrackName = nil;
	}

	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= .7;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .7;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
		case	GAME_MODE_MULTIPLAYERRACE:
				gTrackName = MakeFontStringObject(gTrackNames[gSelectedTrackIndex], &gNewObjectDefinition, gGameViewInfoPtr, true);
				break;

		default:
				gTrackName = MakeFontStringObject(gTrackNamesMP[gSelectedTrackIndex], &gNewObjectDefinition, gGameViewInfoPtr, true);
	}

}




/********************** FREE TRACKSELECT ART **********************/

static void FreeTrackSelectArt(void)
{
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
}



/***************** DRAW TRACKSELECT CALLBACK *******************/

static void DrawTrackSelectCallback(OGLSetupOutputType *info)
{
short				highestUnlocked;


			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);


			/**************************/
			/* DRAW THE SCROLL ARROWS */
			/**************************/

			/* LEFT ARROW */

	if (gSelectedTrackIndex > 0)
	{
		DrawSprite(SPRITE_GROUP_TRACKSELECTSCREEN, TRACKSELECT_SObjType_Arrow_LeftOn,
					LEFT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
	}




			/* RIGHT ARROW */

	if (gSelectedTrackIndex < (gNumTracksInSelection-1))
	{
		DrawSprite(SPRITE_GROUP_TRACKSELECTSCREEN, TRACKSELECT_SObjType_Arrow_RightOn,
					RIGHT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
	}


	DrawObjects(info);


		/**********************/
		/* SEE IF DRAW LOCKED */
		/**********************/

	switch(gGameMode)
	{
		case	GAME_MODE_PRACTICE:
				highestUnlocked = gNumTracksUnlocked[gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE]-1;
				break;

		default:
				highestUnlocked = 1000;
	}


	if (gSelectedTrackIndex > highestUnlocked)
	{
		DrawSprite(SPRITE_GROUP_TRACKSELECTSCREEN, TRACKSELECT_SObjType_Padlock,
					0, 0, .5, 0, 0, info);
	}
}




/***************** DO TRACKSELECT CONTROLS *******************/

static Boolean DoTrackSelectControls(void)
{
short				highestUnlocked;

	if (gGameMode ==	GAME_MODE_PRACTICE)
		highestUnlocked = gNumTracksUnlocked[gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE]-1;
	else
		highestUnlocked = 1000;



		/* SEE IF ABORT */

	if (GetNewKeyState_Real(kKey_Pause))
	{
		gSelectedTrackIndex = -1;
		return(true);
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState_Real(KEY_RETURN))
	{
		if (gSelectedTrackIndex > highestUnlocked)
		{
			PlayEffect(EFFECT_BADSELECT);
		}
		else
		{
			PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
			return(true);
		}
	}


		/* SEE IF CHANGE SELECTION */

	if (GetNewKeyState_Real(KEY_LEFT) && (gSelectedTrackIndex > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedTrackIndex--;
		ModifySpriteObjectFrame(gTrackImageIcon, TRACKSELECT_SObjType__Level0 + gSelectedTrackIndex, gGameViewInfoPtr);
		MakeTrackName();
	}
	else
	if (GetNewKeyState_Real(KEY_RIGHT) && (gSelectedTrackIndex < (gNumTracksInSelection-1)))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedTrackIndex++;
		ModifySpriteObjectFrame(gTrackImageIcon, TRACKSELECT_SObjType__Level0 + gSelectedTrackIndex, gGameViewInfoPtr);
		MakeTrackName();
	}



	return(false);
}















