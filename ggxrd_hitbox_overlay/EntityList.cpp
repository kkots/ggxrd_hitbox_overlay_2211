#include "pch.h"
#include "EntityList.h"
#include "Game.h"
#include "logging.h"

EntityList entityList;

void EntityList::onEndSceneStart() {

	log(fputs("Trying to get entity count\n", logfile));
	count = *(const int*)(*aswEngine + 0xB4);
	log(fputs("Trying to get entity list\n", logfile));
	list = (const char**)(*aswEngine + 0x1FC);
	log(fputs("Trying to get entity slots\n", logfile));
	slots = (const char**)(*aswEngine + 0xC8);

}
