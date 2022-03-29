//
// Sound2.h
//


#define		BASE_EFFECT_RESOURCE	10000

#define		FULL_CHANNEL_VOLUME		kFullVolume
#define		NORMAL_CHANNEL_RATE		0x10000



enum
{
	SONG_WIN,
	SONG_DESERT,
	SONG_JUNGLE,
	SONG_THEME,
	SONG_ATLANTIS,
	SONG_CHINA,
	SONG_CRETE,
	SONG_EUROPE,
	SONG_ICE,
	SONG_VIKING,
	SONG_EGYPT,
	MAX_SONGS
};

enum
{
	SOUND_BANK_MAIN 			= 0,
	SOUND_BANK_MENU				= 1,
	SOUND_BANK_LEVELSPECIFIC	= 2,
	SOUND_BANK_CAVEMAN			= 3,

	MAX_SOUND_BANKS
};



/***************** EFFECTS *************************/
// This table must match gEffectsTable
//

enum
{
		/* DEFAULT */

	EFFECT_BADSELECT = 0,
	EFFECT_SKID3,
	EFFECT_ENGINE,
	EFFECT_SKID,
	EFFECT_CRASH,
	EFFECT_BOOM,
	EFFECT_NITRO,
	EFFECT_ROMANCANDLE_LAUNCH,
	EFFECT_ROMANCANDLE_FALL,
	EFFECT_SKID2,
	EFFECT_SELECTCLICK,
	EFFECT_GETPOW,
	EFFECT_CANNON,
	EFFECT_CRASH2,
	EFFECT_SPLASH,
	EFFECT_BIRDCAW,
	EFFECT_SNOWBALL,
	EFFECT_MINE,

			/* DESERT */

	EFFECT_DUSTDEVIL,

			/* JUNGLE */

	EFFECT_BLOWDART,

			/* ICE */

	EFFECT_HITSNOW,


			/* EUROPE */

	EFFECT_CATAPULT,

			/* CHINA */

	EFFECT_GONG,

			/* EGYPT */

	EFFECT_SHATTER,

			/* ATLANTIS */

	EFFECT_ZAP,
	EFFECT_TORPEDOFIRE,
	EFFECT_HUM,
	EFFECT_BUBBLES,

			/* STONEHENGE */

	EFFECT_CHANT,

			/* CAVEMAN */

	EFFECT_READY,
	EFFECT_SET,
	EFFECT_GO,
	EFFECT_THROW1,
	EFFECT_THROW2,
	EFFECT_THROW3,
	EFFECT_THATSALL,
	EFFECT_GOODJOB,
	EFFECT_REDTEAMWINS,
	EFFECT_GREENTEAMWINS,
	EFFECT_LAP2,
	EFFECT_FINALLAP,
	EFFECT_NICESHOT,
	EFFECT_GOTTAHURT,
	EFFECT_1st,
	EFFECT_2nd,
	EFFECT_3rd,
	EFFECT_4th,
	EFFECT_5th,
	EFFECT_6th,
	EFFECT_OHYEAH,
	EFFECT_NICEDRIVING,
	EFFECT_WOAH,
	EFFECT_YOUWIN,
	EFFECT_YOULOSE,
	EFFECT_POW_BONEBOMB,
	EFFECT_POW_OIL,
	EFFECT_POW_NITRO,
	EFFECT_POW_PIGEON,
	EFFECT_POW_CANDLE,
	EFFECT_POW_ROCKET,
	EFFECT_POW_TORPEDO,
	EFFECT_POW_FREEZE,
	EFFECT_POW_MINE,
	EFFECT_POW_SUSPENSION,
	EFFECT_POW_INVISIBILITY,
	EFFECT_POW_STICKYTIRES,
	EFFECT_ARROWHEAD,
	EFFECT_COMPLETED,
	EFFECT_INCOMPLETE,
	EFFECT_COSTYA,
	EFFECT_WATCHIT
};



/**********************/
/* SOUND BANK TABLES  */
/**********************/
//
// These are simple enums for equating the sound effect #'s in each sound
// bank's rez file.
//

/******** SOUND_BANK_MAIN *********/

enum
{
	SOUND_DEFAULT_BADSELECT = 0,
	SOUND_DEFAULT_SKID3,
	SOUND_DEFAULT_ENGINE,
	SOUND_DEFAULT_SKID,
	SOUND_DEFAULT_CRASH,
	SOUND_DEFAULT_SELECTCLICK,
	SOUND_DEFAULT_GETPOW,
	SOUND_DEFAULT_BOOM,
	SOUND_DEFAULT_NITRO,
	SOUND_DEFAULT_ROMANCANDLELAUNCH,
	SOUND_DEFAULT_ROMANCANDLEFALL,
	SOUND_DEFAULT_SKID2,
	SOUND_DEFAULT_CANNON,
	SOUND_DEFAULT_CRASH2,
	SOUND_DEFAULT_SPLASH,
	SOUND_DEFAULT_BIRDCAW,
	SOUND_DEFAULT_SNOWBALL,
	SOUND_DEFAULT_MINE
};


/******** SOUND_BANK: DESERT *********/

enum
{
	SOUND_DESERT_DUSTDEVIL = 0
};

/******** SOUND_BANK: JUNGLE *********/

enum
{
	SOUND_JUNGLE_BLOWDART = 0
};


/******** SOUND_BANK: ICE *********/

enum
{
	SOUND_ICE_HITSNOW = 0
};


/******** SOUND_BANK: EUROPE *********/

enum
{
	SOUND_EUROPE_CATAPULT = 0
};

/******** SOUND_BANK: CHINA *********/

enum
{
	SOUND_CHINA_GONG = 0
};


/******** SOUND_BANK: ATLANTIS *********/

enum
{
	SOUND_ATLANTIS_ZAP = 0,
	SOUND_ATLANTIS_TORPEDOFIRE,
	SOUND_ATLANTIS_HUM,
	SOUND_ATLANTIS_BUBBLES
};


/******** SOUND_BANK: EGYPT *********/

enum
{
	SOUND_EGYPT_SHATTER = 0
};

/******** SOUND_BANK: STONEHENGE *********/

enum
{
	SOUND_STONEHENGE_CHANT = 0
};



/******** SOUND_BANK: CAVEMAN *********/

enum
{
	SOUND_CAVEMAN_READY = 0,
	SOUND_CAVEMAN_SET,
	SOUND_CAVEMAN_GO,
	SOUND_CAVEMAN_THROW1,
	SOUND_CAVEMAN_THROW2,
	SOUND_CAVEMAN_THROW3,
	SOUND_CAVEMAN_THATSALL,
	SOUND_CAVEMAN_GOODJOB,
	SOUND_CAVEMAN_REDTEAMWINS,
	SOUND_CAVEMAN_GREENTEAMWINS,
	SOUND_CAVEMAN_LAP2,
	SOUND_CAVEMAN_FINALLAP,
	SOUND_CAVEMAN_NICESHOT,
	SOUND_CAVEMAN_GOTTAHURT,
	SOUND_CAVEMAN_1st,
	SOUND_CAVEMAN_2nd,
	SOUND_CAVEMAN_3rd,
	SOUND_CAVEMAN_4th,
	SOUND_CAVEMAN_5th,
	SOUND_CAVEMAN_6th,
	SOUND_CAVEMAN_OHYEAH,
	SOUND_CAVEMAN_NICEDRIVING,
	SOUND_CAVEMAN_WOAH,
	SOUND_CAVEMAN_YOUWIN,
	SOUND_CAVEMAN_YOULOSE,
	SOUND_CAVEMAN_BONEBOMB,
	SOUND_CAVEMAN_OIL,
	SOUND_CAVEMAN_NITRO,
	SOUND_CAVEMAN_PIGEON,
	SOUND_CAVEMAN_CANDLE,
	SOUND_CAVEMAN_ROCKET,
	SOUND_CAVEMAN_TORPEDO,
	SOUND_CAVEMAN_FREEZE,
	SOUND_CAVEMAN_MINE,
	SOUND_CAVEMAN_SUSPENSION,
	SOUND_CAVEMAN_INVISIBILITY,
	SOUND_CAVEMAN_TIRES,
	SOUND_CAVEMAN_ARROWHEAD,
	SOUND_CAVEMAN_COMPLETE,
	SOUND_CAVEMAN_INCOMPLETE,
	SOUND_CAVEMAN_COSTYA,
	SOUND_CAVEMAN_WATCHIT
};



//===================== PROTOTYPES ===================================


void	InitSoundTools(void);
void ShutdownSound(void);

void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
extern	short PlayEffect(short effectNum);
short PlayEffect_Parms3D(short effectNum, const OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust);
void	ToggleMusic(void);
extern void	DoSoundMaintenance(void);
extern	void LoadSoundBank(FSSpec *spec, long bankNum);
extern	void WaitEffectsSilent(void);
extern	void DisposeSoundBank(short bankNum);
extern	void DisposeAllSoundBanks(void);
short PlayEffect_Parms(short effectNum, uint32_t leftVolume, uint32_t rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, uint32_t leftVol, uint32_t rightVol);
short PlayEffect3D(short effectNum, const OGLPoint3D *where);
void Update3DSoundChannel(short effectNum, short *channel, const OGLPoint3D *where);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(OGLSetupOutputType *setupInfo);
void ChangeChannelRate(short channel, long rateMult);
void StopAChannelIfEffectNum(short *channelNum, short effectNum);


void PlayAnnouncerSound(short effectNum, Boolean override, float delay);





