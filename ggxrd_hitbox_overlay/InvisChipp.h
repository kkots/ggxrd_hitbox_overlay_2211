#pragma once
#include "Entity.h"
class InvisChipp
{
public:
	void onEndSceneStart();
	bool isP1InvisChipp() const;
	bool isP2InvisChipp() const;
	bool needToHide(char team) const;
	bool needToHide(const Entity& ent) const;
private:
	bool p1IsInvisChipp = false;
	bool p2IsInvisChipp = false;
	bool determineInvisChipp(const char* aswData);
};

extern InvisChipp invisChipp;
