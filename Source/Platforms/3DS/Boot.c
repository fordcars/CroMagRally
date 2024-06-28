// CRO-MAG RALLY ENTRY POINT for 3DS
// (C) 2022 Iliyas Jorio
// This file is part of Cro-MagRally. https://github.com/jorio/cromagrally

#include "Pomme.h"
#include "game.h"
#include "version.h"

FSSpec gDataSpec;
CommandLineOptions gCommandLine;
int gCurrentAntialiasingLevel = 0;

void GameMain(void);

static void FindGameData()
{
	gDataSpec = PommeHostPathToFSSpec("romfs:", "Skeletons");
	FSSpec someDataFileSpec;
	OSErr iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Brog.bg3d", &someDataFileSpec);
	if (iErr)
	{
		printf("Cannot find data spec: %s\n", gDataSpec.cName);
	}
}

static void ParseCommandLine(int argc, char** argv)
{
	memset(&gCommandLine, 0, sizeof(gCommandLine));
	gCommandLine.vsync = 1;
}

static void Boot(int argc, char** argv)
{
	ParseCommandLine(argc, argv);

	// Start our "machine"
	PommeInit();

	// Load game prefs before starting
	InitDefaultPrefs();

	// Find path to game data folder
	FindGameData();
}

int main(int argc, char** argv)
{
	Boot(argc, argv);
	GameMain();
	PommeShutdown();

	printf("DONE\n");
	while(true){}

	return 0;
}