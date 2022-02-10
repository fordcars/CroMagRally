/****************************/
/*   	SELECT VEHICLE.C 	*/
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
#include "sobjtypes.h"
#include "selectvehicle.h"
#include "network.h"
#include "player.h"
#include "sprites.h"
#include "mobjtypes.h"
#include "bg3d.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	FSSpec			gDataSpec;
extern	KeyMap 			gKeyMap,gNewKeys;
extern	short			gMyNetworkPlayerNum,gNumRealPlayers,gNumLocalPlayers,gCurrentPlayerNum;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean			gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	PrefsType		gGamePrefs;
extern	OGLPoint3D		gCoord;
extern  MOPictureObject 	*gBackgoundPicture;
extern	Boolean					gNetGameInProgress,gIsNetworkClient;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gDelta;
extern	PlayerInfoType	gPlayerInfo[];
extern	SavePlayerType	gPlayerSaveData;
extern	int				gTrackNum,gGameMode;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupVehicleSelectScreen(short whichPlayer);
static Boolean DoVehicleSelectControls(short whichPlayer, Boolean allowAborting);
static void DrawVehicleSelectCallback(OGLSetupOutputType *info);
static void FreeVehicleSelectArt(void);
static void MoveCarModel(ObjNode *theNode);
static void MakeVehicleName(void);



/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	VEHICLESELECT_SObjType_Arrow_LeftOn,
	VEHICLESELECT_SObjType_Arrow_RightOn,

	VEHICLESELECT_SObjType_Meter1,
	VEHICLESELECT_SObjType_Meter2,
	VEHICLESELECT_SObjType_Meter3,
	VEHICLESELECT_SObjType_Meter4,
	VEHICLESELECT_SObjType_Meter5,
	VEHICLESELECT_SObjType_Meter6,
	VEHICLESELECT_SObjType_Meter7,
	VEHICLESELECT_SObjType_Meter8
};


#define	LEFT_ARROW_X	-.6
#define RIGHT_ARROW_X	.6
#define	ARROW_Y			.2
#define	ARROW_SCALE		.5

#define	CAR_Y			70

#define	VEHICLE_IMAGE_Y		.3
#define	VEHICLE_IMAGE_SCALE	1.2

#define	PARAMETERS_X		-.7
#define	PARAMETERS_Y		-.4
#define	PARAMETERS_SCALE	.5

#define	LINE_SPACING		.14f

#define	NAME_Y			.6


/*********************/
/*    VARIABLES      */
/*********************/

static int		gSelectedVehicleIndex;
static ObjNode	*gVehicleObj, *gMeterIcon[NUM_VEHICLE_PARAMETERS];
static ObjNode	*gVehicleName;

static int		gNumVehiclesToChooseFrom;

static Str31 	gParmStr[NUM_VEHICLE_PARAMETERS] = {"SPEED", "ACCELERATION", "TRACTION", "SUSPENSION"};

static Str31	gCarNames[MAX_CAR_TYPES] = {"MAMMOTH MOBILE", "BONE BUGGY", "GEODE CRUISER", "LOGMOBILE", "TURTLE TANK", "HOT ROCK",
											"TROJAN HORSE", "OBELISK",
											"CATAPULT", "CHARIOT"};

int		gVehicleParameters[MAX_CAR_TYPES+1][NUM_VEHICLE_PARAMETERS];					// parameter values 0..7
int		gDefaultVehicleParameters[MAX_CAR_TYPES+1][NUM_VEHICLE_PARAMETERS] =					// parameter values 0..7
{

		/* MAMMOTH */

	4,							// speed
	2,							// acceleration
	4,							// traction
	7,							// suspension

		/* BONE BUGGY */

	5,							// speed
	4,							// acceleration
	4,							// traction
	2,							// suspension


		/* GEODE */

	3,							// speed
	5,							// acceleration
	6,							// traction
	7,							// suspension

		/* LOG */

	6,							// speed
	5,							// acceleration
	2,							// traction
	3,							// suspension

		/* TURTLE */

	3,							// speed
	6,							// acceleration
	4,							// traction
	3,							// suspension

		/* ROCK */

	4,							// speed
	3,							// acceleration
	5,							// traction
	2,							// suspension

		/* TROJAN HORSE */

	4,							// speed
	5,							// acceleration
	6,							// traction
	4,							// suspension

		/* OBELISK */

	5,							// speed
	3,							// acceleration
	6,							// traction
	7,							// suspension

		/* CATAPULT */

	6,							// speed
	7,							// acceleration
	5,							// traction
	4,							// suspension

		/* CHARIOT */

	7,							// speed
	7,							// acceleration
	4,							// traction
	3,							// suspension

		/* SUBMARINE */

	0,							// speed
	7,							// acceleration
	0,							// traction
	0,							// suspension


};



/******************* DO MULTIPLAYER VEHICLE SELECTIONS ********************/

void DoMultiPlayerVehicleSelections(void)
{
	if (gNetGameInProgress)
	{
		DoCharacterSelectScreen(gMyNetworkPlayerNum, false);		// get player's sex
		DoVehicleSelectScreen(gMyNetworkPlayerNum, false);			// get this player's vehicle
		PlayerBroadcastVehicleType();								// tell other net players about my type
		GetVehicleSelectionFromNetPlayers();						// get types from other net players
	}
	else
	{
		for (gCurrentPlayerNum = 0; gCurrentPlayerNum < gNumLocalPlayers; gCurrentPlayerNum++)
		{
			DoCharacterSelectScreen(gCurrentPlayerNum, false);		// get player's sex
			DoVehicleSelectScreen(gCurrentPlayerNum, false);		// do it for each local player (split screen mode)
		}
	}


}


/********************** DO VEHICLE SELECT SCREEN **************************/
//
// Return true if user aborts.
//

Boolean DoVehicleSelectScreen(short whichPlayer, Boolean allowAborting)
{
	if (gTrackNum == TRACK_NUM_ATLANTIS)		// dont select this on atlantis
		return(false);

	SetupVehicleSelectScreen(whichPlayer);
	MakeFadeEvent(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (DoVehicleSelectControls(whichPlayer, allowAborting))
			break;


			/**************/
			/* DRAW STUFF */
			/**************/

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawVehicleSelectCallback);
	}

			/***********/
			/* CLEANUP */
			/***********/

	GammaFadeOut();
	FreeVehicleSelectArt();

	if (gSelectedVehicleIndex == -1)										// see if user bailed
		return(true);


		/* SET VEHICLE TYPE SELECTED */

	gPlayerInfo[whichPlayer].vehicleType = gSelectedVehicleIndex;

	return(false);
}


/********************* SETUP VEHICLESELECT SCREEN **********************/

static void SetupVehicleSelectScreen(short whichPlayer)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
OGLColorRGBA		ambientColor = { .4, .4, .4, 1 };
OGLColorRGBA		fillColor1 = { 1.0, 1.0, 1.0, 1 };
OGLColorRGBA		fillColor2 = { .5, .5, .5, 1 };
OGLVector3D			fillDirection1 = { .9, -.7, -1 };
OGLVector3D			fillDirection2 = { -1, -.2, -.5 };
int					i;
u_long				n;

	gVehicleName = nil;
	gSelectedVehicleIndex = 0;

#if DEMO
	gNumVehiclesToChooseFrom = 6;							// only let do main cars
#else
	i = gPlayerSaveData.numAgesCompleted & AGE_MASK_AGE;
	if (i > 2)												// prevent extra cars after winning Iron Age
		i = 2;
	gNumVehiclesToChooseFrom = 6 + (i * 2);					// set # cars we can pick from

	if (gNumRealPlayers > 1)							// let use any car in mutliplayer mode
		gNumVehiclesToChooseFrom = 10;

#endif
			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.fov 				= 1.1;
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.camera.from[0].z		= -700;
	viewDef.camera.from[0].y		= 250;

	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= fillColor1;
	viewDef.lights.fillColor[1] 	= fillColor2;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);



				/************/
				/* LOAD ART */
				/************/


			/* MAKE BACKGROUND PICTURE OBJECT */

	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:VehicleSelectScreen", &spec))
		DoFatalAlert("SetupVehicleSelectScreen: background pict not found.");

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
	if (!gBackgoundPicture)
		DoFatalAlert("SetupVehicleSelectScreen: MO_CreateNewObjectOfType failed");


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:vehicleselect.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_VEHICLESELECTSCREEN, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:wallfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:carselect.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_CARSELECT, gGameViewInfoPtr);

			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* VEHICLE MODEL */

	gNewObjectDefinition.group 		= MODEL_GROUP_CARSELECT;
	gNewObjectDefinition.type 		= gSelectedVehicleIndex;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= CAR_Y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveCarModel;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	gVehicleObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* VEHICLE NAME */

	MakeVehicleName();



			/* PARAMETER STRINGS */

	gNewObjectDefinition.coord.x 	= PARAMETERS_X;
	gNewObjectDefinition.coord.y 	= PARAMETERS_Y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = PARAMETERS_SCALE;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	for (i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{

		MakeFontStringObject(gParmStr[i], &gNewObjectDefinition, gGameViewInfoPtr, false);
		gNewObjectDefinition.coord.y 	-= LINE_SPACING;
	}

			/* METER ICONS */

	gNewObjectDefinition.group 		= SPRITE_GROUP_VEHICLESELECTSCREEN;
	gNewObjectDefinition.coord.x 	= PARAMETERS_X + 1.2;
	gNewObjectDefinition.coord.y 	= PARAMETERS_Y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = PARAMETERS_SCALE;

	for (i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
	{
		n = gVehicleParameters[gSelectedVehicleIndex][i];
		if (n > 7)
			n = 7;

		gNewObjectDefinition.type 	= VEHICLESELECT_SObjType_Meter1 + n;

		gMeterIcon[i] = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);
		gNewObjectDefinition.coord.y 	-= LINE_SPACING;
	}

			/* SEE IF DOING 2-PLAYER LOCALLY */

	if (gNumLocalPlayers > 1)
	{
		static Str31 playerStr[2] = {"PLAYER 1", "PLAYER 2"};
		ObjNode	*newObj;

		gNewObjectDefinition.coord.x 	= 0;
		gNewObjectDefinition.coord.y 	= .8;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = .5;
		gNewObjectDefinition.slot 		= SPRITE_SLOT;
		newObj = MakeFontStringObject(playerStr[whichPlayer], &gNewObjectDefinition, gGameViewInfoPtr, true);

		newObj->ColorFilter.r = .5;
		newObj->ColorFilter.g = .3;
		newObj->ColorFilter.b = .2;
	}
}


/******************** MAKE VEHICLE NAME ************************/

static void MakeVehicleName(void)
{
	if (gVehicleName)
	{
		DeleteObject(gVehicleName);
		gVehicleName = nil;
	}

	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= NAME_Y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .6;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	gVehicleName = MakeFontStringObject(gCarNames[gSelectedVehicleIndex], &gNewObjectDefinition, gGameViewInfoPtr, true);

	gVehicleName->ColorFilter.r = .3;
	gVehicleName->ColorFilter.g = .5;
	gVehicleName->ColorFilter.b = .2;
}



/********************** FREE VEHICLESELECT ART **********************/

static void FreeVehicleSelectArt(void)
{
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
//	FlushEventQueue(GetMainEventQueue());
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}


/***************** DRAW VEHICLESELECT CALLBACK *******************/

static void DrawVehicleSelectCallback(OGLSetupOutputType *info)
{

			/* DRAW BACKGROUND */

	MO_DrawObject(gBackgoundPicture, info);


			/**************************/
			/* DRAW THE SCROLL ARROWS */
			/**************************/

			/* LEFT ARROW */

	if (gSelectedVehicleIndex > 0)
	{
		DrawSprite(SPRITE_GROUP_VEHICLESELECTSCREEN, VEHICLESELECT_SObjType_Arrow_LeftOn,
					LEFT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
	}




			/* RIGHT ARROW */

	if (gSelectedVehicleIndex < (gNumVehiclesToChooseFrom-1))
	{
		DrawSprite(SPRITE_GROUP_VEHICLESELECTSCREEN, VEHICLESELECT_SObjType_Arrow_RightOn,
					RIGHT_ARROW_X, ARROW_Y, ARROW_SCALE, 0, 0, info);
	}


	DrawObjects(info);
}




/***************** DO VEHICLESELECT CONTROLS *******************/

static Boolean DoVehicleSelectControls(short whichPlayer, Boolean allowAborting)
{
int		i,n;
short	p;


	if (gNetGameInProgress)										// if net game, then use P0 controls
		p = 0;
	else
		p = whichPlayer;										// otherwise, use P0 or P1 controls depending.


		/* SEE IF ABORT */

	if (allowAborting)
	{
		if (GetNewKeyState_Real(kKey_Pause))
		{
			gSelectedVehicleIndex = -1;
			return(true);
		}
	}

		/* SEE IF SELECT THIS ONE */

	if (GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState(kKey_MakeSelection_P2) || GetNewKeyState_Real(KEY_RETURN))
	{
		PlayEffect_Parms(EFFECT_SELECTCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE * 2/3);
		return(true);
	}


		/* SEE IF CHANGE SELECTION */

	if (GetNewKeyState_Real(KEY_LEFT) && (gSelectedVehicleIndex > 0))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex--;
		gVehicleObj->Type = gSelectedVehicleIndex;
		ResetDisplayGroupObject(gVehicleObj);
		MakeVehicleName();

		for (i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		{
			n = gVehicleParameters[gSelectedVehicleIndex][i];
			if (n > 7)
				n = 7;
			ModifySpriteObjectFrame(gMeterIcon[i], VEHICLESELECT_SObjType_Meter1 + n, gGameViewInfoPtr);
		}
	}
	else
	if (GetNewKeyState_Real(KEY_RIGHT) && (gSelectedVehicleIndex < (gNumVehiclesToChooseFrom-1)))
	{
		PlayEffect(EFFECT_SELECTCLICK);
		gSelectedVehicleIndex++;
		gVehicleObj->Type = gSelectedVehicleIndex;
		ResetDisplayGroupObject(gVehicleObj);
		MakeVehicleName();

		for (i = 0; i < NUM_VEHICLE_PARAMETERS; i++)
		{
			n = gVehicleParameters[gSelectedVehicleIndex][i];
			if (n > 7)
				n = 7;
			ModifySpriteObjectFrame(gMeterIcon[i], VEHICLESELECT_SObjType_Meter1 + n, gGameViewInfoPtr);
		}
	}

	return(false);
}





/****************** MOVE CAR MODEL *********************/

static void MoveCarModel(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * 1.5f;
	UpdateObjectTransforms(theNode);
}








