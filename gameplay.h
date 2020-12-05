#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"

enum NodeType {BUILDING, PROP, COLLECTABLE, NPC, PLAYER};

#define MAXBUILDINGS 4
#define MAXFLOORS 1
#define MAXPLAYERS 1
#define MAXPROPS 8
#define MAXCOLLECTIBLES 1
#define MAXNPCS 8

struct Node
{
	enum NodeType type;
	int modelIndex;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	int collisionMask;//node belongs to this mask.
	int colliderMask;//node collides with this mask.
};

struct Trigger//shift tiles when player is colliding with just one.
{
	BoundingBox bounds;
	bool colliding;
	Vector3 scale;
	struct Node *collidersList, *collidersListStart, *collidersListEnd;
};

struct Tile
{
	struct Node *building;
	struct Node *propList, *propListStart, *propListEnd;
	struct Node *collectibleList, *collectibleListStart, *collectibleListEnd;
	struct Node *npcList, *npcListStart, *npcListEnd;
	struct Trigger trigger;
};

struct GameplayData
{
	int buildingCount;
	Model buildingModels[MAXBUILDINGS];
	int floorCount;
	Model floorModels[MAXFLOORS];
	int playerCount;
	Model playerModels[MAXPLAYERS];
	int propCount;
	Model propModels[MAXPROPS];
	int collectibleCount;
	Model collectibleModels[MAXCOLLECTIBLES];
	int npcCount;
	Model npcModels[MAXNPCS];
	
	struct Tile *tileList, *tileListStart, *tileListEnd;
	struct Tile *tileListSwap, *tileListSwapStart, *tileListSwapEnd;
};

void PopulateModelCache(char *curDir, Model *models, int *modelCount, int maxCount);

void GameplayInit();
void GameplayExit();
void GameplayLoop();

#endif
