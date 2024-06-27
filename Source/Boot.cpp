// CRO-MAG RALLY ENTRY POINT
// (C) 2022 Iliyas Jorio
// This file is part of Cro-MagRally. https://github.com/jorio/cromagrally

// Using SDL from C++ on 3DS DOES NOT WORK!
// No idea why, but it causes bery weird UB. I spent too much time on this.
#ifndef __3DS__
	#include <SDL.h>
#endif

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"

#include <iostream>
#include <cstring>

extern "C"
{
	#include "game.h"
	#include "version.h"
}

int main(int argc, char** argv)
{
//  THIS CAUSES THE ISSUE???????????????
// 	int				returnCode				= 0;
// 	std::string		finalErrorMessage		= "";
// 	bool			showFinalErrorMessage	= false;
		OGL_DrawScene(0);
	return 0;
}
