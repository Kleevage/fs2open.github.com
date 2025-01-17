/*
 * Created by Ian "Goober5000" Warfield for the FreeSpace2 Source Code Project.
 * You may not sell or otherwise commercially exploit the source or things you
 * create based on the source.
 */ 



#ifndef _GLOBALS_H
#define _GLOBALS_H


// from parselo.h
#define	PATHNAME_LENGTH			192
#define	NAME_LENGTH				32
#define	SEXP_LENGTH				128
#define	DATE_LENGTH				32
#define	TIME_LENGTH				16
#define	DATE_TIME_LENGTH		48
#define	NOTES_LENGTH			1024
#define	MULTITEXT_LENGTH		4096
#define	FILESPEC_LENGTH			64
#define	MESSAGE_LENGTH			512
#define TRAINING_MESSAGE_LENGTH	512
#define CONDITION_LENGTH		64

// from missionparse.h
#define MISSION_DESC_LENGTH		512

// from player.h
#define CALLSIGN_LEN					(MAX_FILENAME_LEN - 4 - 1)		//	shortened from 32 to allow .json to be attached without exceeding MAX_FILENAME_LEN
#define SHORT_CALLSIGN_PIXEL_W	80		// max width of short_callsign[] in pixels

// from ship.h
#define MAX_SHIPS					500		// max number of ship instances there can be.DTP; bumped from 200 to 400, then to 500 in 2022
#define SHIPS_LIMIT					500		// what MAX_SHIPS will be at release time (for error checking in debug mode); dtp Bumped from 200 to 400, then to 500 in 2022

// from missionparse.h and then redefined to the same value in sexp.h
#define TOKEN_LENGTH	32

#define MAX_SHIP_CLASSES		500

#define MAX_WINGS				75

#define MAX_SHIPS_PER_WING			6

#define MAX_STARTING_WINGS			3	// number of wings player can start a mission with
#define MAX_SQUADRON_WINGS			5	// number of wings in squadron (displayed on HUD)

#define MAX_TVT_TEAMS				2	// number of teams in a TVT game
#define	MAX_TVT_WINGS_PER_TEAM		1 	// number of wings per team in a TVT game
#define MAX_TVT_WINGS		MAX_TVT_TEAMS * MAX_TVT_WINGS_PER_TEAM	// number of wings in a TVT game

// from model.h
#define MAX_SHIP_PRIMARY_BANKS		3
#define MAX_SHIP_SECONDARY_BANKS	4
#define MAX_SHIP_WEAPONS			(MAX_SHIP_PRIMARY_BANKS+MAX_SHIP_SECONDARY_BANKS)

// Goober5000 - moved from hudescort.cpp
// size of complete escort list, including all those wanting to get onto list but without space
#define MAX_COMPLETE_ESCORT_LIST	20
             
// from weapon.h
#define MAX_WEAPONS	3000		//Increased from 2000 to 3000 in 2022

#define MAX_WEAPON_TYPES				500


// from model.h

#define MAX_MODEL_TEXTURES	64

#define MAX_POLYGON_MODELS  300

// object.h
#define MAX_OBJECTS			5000	//Increased from 3500 to 5000 in 2022	

// from weapon.h (and beam.h)
#define MAX_BEAM_SECTIONS				5

#endif	// _GLOBALS_H
