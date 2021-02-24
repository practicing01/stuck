#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"

struct Butt
{
	Rectangle rect;
	int defautFontSize;
	int curFontSize;
	char text[256];
	Vector2 textPos;
};

struct MenuData
{
	struct Butt playButt;
	struct Butt exitButt;
	
	Texture2D tex;
};

void MenuInit();
void MenuExit();
void MenuLoop();

#endif
