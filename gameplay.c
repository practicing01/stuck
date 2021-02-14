#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"
#include "gameplay.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "raymath.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

void RespawnPollen(void *data)
{
	struct Node *pollen = (struct Node *)data;
	(*pollen).visible = true;
}

void ScheduleTask( void (*task)(void *data), void *data,  float maxTime )
{
	struct Task *newTask = (struct Task *)malloc( sizeof(struct Task) );
	
	(*newTask).prev = NULL;
	(*newTask).next = NULL;
	
	(*newTask).task = task;
	(*newTask).data = data;
	(*newTask).maxTime = maxTime;
	(*newTask).elapsedTime = 0.0f;
	
	if ( (* (struct GameplayData *)moduleData).taskListStart == NULL )//no tasks
	{
		(* (struct GameplayData *)moduleData).taskListStart = newTask;
		(* (struct GameplayData *)moduleData).taskListEnd = newTask;
	}
	else//tasks waiting
	{
		(* (* (struct GameplayData *)moduleData).taskListEnd).next = newTask;
		(*newTask).prev = (* (struct GameplayData *)moduleData).taskListEnd;
	}
}

void PollTasks()
{
	struct Task *curTask = (* (struct GameplayData *)moduleData).taskListStart;
	
	while ( curTask != NULL )
	{
		(*curTask).elapsedTime += dt.deltaTime;
		
		if ( (*curTask).elapsedTime >= (*curTask).maxTime )
		{
			(*curTask).task( (*curTask).data );//call functor
			
			if ( (*curTask).prev != NULL )
			{
				if ( (*curTask).next != NULL )//recouple
				{
					(* (*curTask).prev).next = (*curTask).next;
					(* (*curTask).next).prev = (*curTask).prev;
					
					struct Task *dummy = curTask;
					curTask = (*curTask).next;
					free( dummy );
					continue;
				}
				else//last task
				{
					(* (*curTask).prev).next = NULL;
					(* (struct GameplayData *)moduleData).taskListEnd = (*curTask).prev;
					
					free( curTask );
					curTask = NULL;
					continue;
				}
			}
			else//first task
			{
				if ( (*curTask).next != NULL )
				{
					(* (*curTask).next).prev = NULL;
					(* (struct GameplayData *)moduleData).taskListStart = (*curTask).next;
					
					struct Task *dummy = curTask;
					curTask = (*curTask).next;
					free( dummy );
					continue;
				}
				else//only task
				{
					(* (struct GameplayData *)moduleData).taskListStart = NULL;
					(* (struct GameplayData *)moduleData).taskListEnd = NULL;
					
					free( curTask );
					curTask = NULL;
					continue;
				}
			}
		}
		
		curTask = (*curTask).next;
	}
}

struct Node* CheckNodeCollision(struct Node *node)
{
	struct Node *curNode = NULL;
	
	curNode = (* (struct GameplayData *)moduleData).colliders[ (int) ( (*node).colliderIndex.x ) ][ (int) ( (*node).colliderIndex.y ) ];
	
	while (curNode != NULL)
	{
		if ( curNode != node )
		{
			if ( CheckCollisionSpheres(	(*node).position, 0.5f,	(*curNode).position, 0.5f) )
			{
				return curNode;
			}
		}
		
		curNode = (*curNode).colliderNext;
	}
	
	return NULL;
}

void IndexCollider(struct Node *node)
{
	//#define COLLIDERINTERVAL 10
	//#define COLDIM 60//(TILESIZE/COLLIDERINTERVAL) * 3
	//float offset = TILESIZE * TILEFACTOR;
	//offset *= 1.5f;

	Vector2 index;
	index.x = ( ( (*node).position.x ) + COLLIDEROFFSET ) / (float)COLLIDERINTERVAL;
	index.y = ( ( (*node).position.z ) + COLLIDEROFFSET ) / (float)COLLIDERINTERVAL;
	
	index.x = Clamp( index.x, 0.0f, (float)(COLDIM-1) );
	index.y = Clamp( index.y, 0.0f, (float)(COLDIM-1) );
	
	//TraceLog(LOG_INFO, "position: %f %f index: %d %d", (*node).position.x, (*node).position.z, (int)(index.x), (int)(index.y));
	
	if ( (*node).colliderNext != NULL )//indexed already, has next
	{
		if ( (*node).colliderPrev != NULL )//decouple this and reconnect the other links
		{
			(* ((*node).colliderPrev) ).colliderNext = (*node).colliderNext;
			(* ((*node).colliderNext) ).colliderPrev = (*node).colliderPrev;
		}
		else//no prev, reset first index
		{
			(* ((*node).colliderNext) ).colliderPrev = NULL;
			(* (struct GameplayData *)moduleData).colliders[ (int) ( (*node).colliderIndex.x ) ][ (int) ( (*node).colliderIndex.y ) ] = (*node).colliderNext;
		}
	}
	else if ( (*node).colliderPrev != NULL )//indexed already, has prev
	{
		(* ((*node).colliderPrev) ).colliderNext = NULL;
	}
	else if ( (* (struct GameplayData *)moduleData).colliders[ (int) ( (*node).colliderIndex.x ) ][ (int) ( (*node).colliderIndex.y ) ] == node )
	{
		(* (struct GameplayData *)moduleData).colliders[ (int) ( (*node).colliderIndex.x ) ][ (int) ( (*node).colliderIndex.y ) ] = NULL;
	}
	
	(*node).colliderNext = NULL;
	(*node).colliderPrev = NULL;
	
	if ( (* (struct GameplayData *)moduleData).colliders[(int)(index.x)][(int)(index.y)] == NULL )//empty slot
	{
		(* (struct GameplayData *)moduleData).colliders[(int)(index.x)][(int)(index.y)] = node;
	}
	else//slot occupied
	{
		(* ( (* (struct GameplayData *)moduleData).colliders[(int)(index.x)][(int)(index.y)] ) ).colliderPrev = node;
		(*node).colliderNext = (* (struct GameplayData *)moduleData).colliders[(int)(index.x)][(int)(index.y)];
		
		(* (struct GameplayData *)moduleData).colliders[(int)(index.x)][(int)(index.y)] = node;
	}
	
	(*node).colliderIndex = index;
}

void DrawTiles()
{
	struct Tile *curNode;
	struct Tile *nextNode;
	curNode = (* (struct GameplayData *)moduleData).tileListStart;
		
	while (curNode != NULL)
	{
		Matrix rotation = MatrixIdentity();
		Matrix translation = MatrixTranslate( 
		(* (*curNode).floor).position.x,
		(* (*curNode).floor).position.y,
		(* (*curNode).floor).position.z );
		
		Matrix transform = MatrixMultiply( rotation, translation );
		
		(* (struct GameplayData *)moduleData).floorModels[ (* (*curNode).floor).modelIndex ].transform = transform;
		(* (struct GameplayData *)moduleData).buildingModels[ (* (*curNode).building).modelIndex ].transform = transform;
		
		DrawModel( (* (struct GameplayData *)moduleData).floorModels[ (* (*curNode).floor).modelIndex ], (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
		DrawModel( (* (struct GameplayData *)moduleData).buildingModels[ (* (*curNode).building).modelIndex ], (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
		
		//draw props
		if ( curNode == (* (struct GameplayData *)moduleData).curTile )
		{
			struct Node *curProp;
			curProp = (*curNode).propListStart;
			
			while (curProp != NULL)
			{
				translation = MatrixTranslate( 
				(*curProp).position.x,
				(*curProp).position.y,
				(*curProp).position.z );
				
				transform = MatrixMultiply( rotation, translation );
				
				(* (struct GameplayData *)moduleData).propModels[ (*curProp).modelIndex ].transform = transform;
				
				DrawModel( (* (struct GameplayData *)moduleData).propModels[ (*curProp).modelIndex ], (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
				
				curProp = (*curProp).next;
			}
		}
		
		//draw flowers
		if ( curNode == (* (struct GameplayData *)moduleData).curTile )
		{
			struct Node *curFlower;
			curFlower = (*curNode).flowerListStart;
			
			while (curFlower != NULL)
			{
				translation = MatrixTranslate( 
				(*curFlower).position.x,
				(*curFlower).position.y,
				(*curFlower).position.z );
				
				transform = MatrixMultiply( rotation, translation );
				
				(* (struct GameplayData *)moduleData).flowerModels[ (*curFlower).modelIndex ].transform = transform;
				
				DrawModel( (* (struct GameplayData *)moduleData).flowerModels[ (*curFlower).modelIndex ], (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
				
				curFlower = (*curFlower).next;
			}
		}
		
		//draw pollen
		if ( curNode == (* (struct GameplayData *)moduleData).curTile )
		{
			struct Node *curPollen;
			curPollen = (*curNode).pollenListStart;
			
			while (curPollen != NULL)
			{
				if ( (*curPollen).visible == true )
				{
					translation = MatrixTranslate( 
					(*curPollen).position.x,
					(*curPollen).position.y,
					(*curPollen).position.z );
					
					transform = MatrixMultiply( rotation, translation );
					
					(* (struct GameplayData *)moduleData).pollenModel.transform = transform;
					
					DrawModel( (* (struct GameplayData *)moduleData).pollenModel, (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
				}
				
				curPollen = (*curPollen).next;
			}
		}
		
		curNode = (*curNode).next;
	}

}

void RemoveTiles()
{
	struct Tile *curNode;
	struct Tile *nextNode;
	curNode = (* (struct GameplayData *)moduleData).tileListStart;
	
	while (curNode != NULL)
	{
		free( (*curNode).floor );
		free( (*curNode).building );
		
		//free props
		struct Node *curProp;
		struct Node *nextProp;
		curProp = (*curNode).propListStart;
		
		while (curProp != NULL)
		{
			nextProp = (*curProp).next;
		
			free(curProp);
			
			curProp = nextProp;
		}
		
		//free flowers
		struct Node *curFlower;
		struct Node *nextFlower;
		curFlower = (*curNode).flowerListStart;
		
		while (curFlower != NULL)
		{
			nextFlower = (*curFlower).next;
		
			free(curFlower);
			
			curFlower = nextFlower;
		}
		
		//free pollen
		struct Node *curPollen;
		struct Node *nextPollen;
		curPollen = (*curNode).pollenListStart;
		
		while (curPollen != NULL)
		{
			nextPollen = (*curPollen).next;
		
			free(curPollen);
			
			curPollen = nextPollen;
		}
		
		//free node
		nextNode = (*curNode).next;
		
		free(curNode);
		
		curNode = nextNode;
	}
	
	(* (struct GameplayData *)moduleData).tileListStart = NULL;
	(* (struct GameplayData *)moduleData).tileListEnd = NULL;
}

void GetFlowerHitY()
{
	for (int x = 0; x < MAXFLOWERS; x++)
	{
		RayHitInfo hit;
		Ray rayTest;

		rayTest.direction = (Vector3){ 0.0f, -1.0f, 0.0f };
		rayTest.position = (Vector3){ 0.0f, 100.0f, 0.0f };
		
		hit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).flowerModels[ x ] );
		
		(* (struct GameplayData *)moduleData).flowerHitY[ x ] = hit.position.y;
	}
}

void InitTiles()
{
	GetFlowerHitY();
	
	float propDensity = 40.0f;
	float halfTileSize = TILESIZE * 0.5f;
	float propInterval = TILESIZE / propDensity;
	float halfPropInterval = propInterval * 0.5f;
	
	float flowerDensity = 10.0f;
	float flowerInterval = TILESIZE / flowerDensity;
	float halfFlowerInterval = flowerInterval * 0.5f;
	
	struct Tile dummy;
	dummy.prev = NULL;
	dummy.next = NULL;
	
	for (int y = -TILEFACTOR; y <= TILEFACTOR; y++)
	{
		for (int x = -TILEFACTOR; x <= TILEFACTOR; x++)
		{
			struct Tile *newTile = (struct Tile *)malloc( sizeof(struct Tile) );
			
			(*newTile).prev = NULL;
			(*newTile).next = NULL;
			
			if (dummy.prev == NULL)
			{
				dummy.prev = newTile;
				dummy.next = newTile;
			}
			else
			{
				(*newTile).prev = dummy.next;
				(*dummy.next).next = newTile;
				dummy.next = newTile;
			}
			
			(*newTile).offset.x = x;
			(*newTile).offset.y = y;
			
			(*newTile).position.x = (float)x * TILESIZE;
			(*newTile).position.y = 0.0f;
			(*newTile).position.z = (float)y * TILESIZE;
			
			//create floor
			(*newTile).floor = (struct Node *)malloc( sizeof(struct Node) );
			(* (*newTile).floor).position = (*newTile).position;
			(* (*newTile).floor).type = FLOOR;
			(* (*newTile).floor).modelIndex = 0;//GetRandomValue(0, MAXFLOORS - 1);
			
			//create building
			(*newTile).building = (struct Node *)malloc( sizeof(struct Node) );
			(* (*newTile).building).position = (*newTile).position;
			(* (*newTile).building).type = BUILDING;
			(* (*newTile).building).modelIndex = GetRandomValue(0, MAXBUILDINGS - 1);
			
			//create props
			struct Node dummyProp;
			dummyProp.prev = NULL;
			dummyProp.next = NULL;
	
			for (int w = 0; w < propDensity; w++)
			{
				for (int z = 0; z < propDensity; z++)
				{
					struct Node *newProp = (struct Node *)malloc( sizeof(struct Node) );
			
					(*newProp).prev = NULL;
					(*newProp).next = NULL;
					
					if (dummyProp.prev == NULL)
					{
						dummyProp.prev = newProp;
						dummyProp.next = newProp;
						
						(*newTile).propListStart = newProp;
					}
					else
					{
						(*newProp).prev = dummyProp.next;
						(*dummyProp.next).next = newProp;
						dummyProp.next = newProp;
						
						(*newTile).propListEnd = newProp;
					}
					
					(*newProp).position.x = ( (*newTile).position.x - halfTileSize ) + ( propInterval * (float)z );
					(*newProp).position.y = 0.0f;
					(*newProp).position.z = ( (*newTile).position.z - halfTileSize ) + ( propInterval * (float)w );
					
					//randomly offset for variety.
					(*newProp).position.x += (float)GetRandomValue(-halfPropInterval, halfPropInterval);
					(*newProp).position.z += (float)GetRandomValue(-halfPropInterval, halfPropInterval);
					
					(*newProp).type = PROP;
					(*newProp).modelIndex = GetRandomValue(0, MAXPROPS - 1);
				}
			}
			
			//create flowers
			struct Node dummyFlower;
			dummyFlower.prev = NULL;
			dummyFlower.next = NULL;
			
			struct Node dummyPollen;
			dummyPollen.prev = NULL;
			dummyPollen.next = NULL;
	
			for (int w = 0; w < flowerDensity; w++)
			{
				for (int z = 0; z < flowerDensity; z++)
				{
					struct Node *newFlower = (struct Node *)malloc( sizeof(struct Node) );
					struct Node *newPollen = (struct Node *)malloc( sizeof(struct Node) );
			
					(*newFlower).prev = NULL;
					(*newFlower).next = NULL;
					
					(*newPollen).prev = NULL;
					(*newPollen).next = NULL;
					
					if (dummyFlower.prev == NULL)
					{
						dummyFlower.prev = newFlower;
						dummyFlower.next = newFlower;
						
						(*newTile).flowerListStart = newFlower;
						
						dummyPollen.prev = newPollen;
						dummyPollen.next = newPollen;
						
						(*newTile).pollenListStart = newPollen;
					}
					else
					{
						(*newFlower).prev = dummyFlower.next;
						(*dummyFlower.next).next = newFlower;
						dummyFlower.next = newFlower;
						
						(*newTile).flowerListEnd = newFlower;
						
						(*newPollen).prev = dummyPollen.next;
						(*dummyPollen.next).next = newPollen;
						dummyPollen.next = newPollen;
						
						(*newTile).pollenListEnd = newPollen;
					}
					
					(*newFlower).position.x = ( (*newTile).position.x - halfTileSize ) + ( flowerInterval * (float)z );
					(*newFlower).position.y = 0.0f;
					(*newFlower).position.z = ( (*newTile).position.z - halfTileSize ) + ( flowerInterval * (float)w );
					
					//randomly offset for variety.
					(*newFlower).position.x += (float)GetRandomValue(-halfFlowerInterval, halfFlowerInterval);
					(*newFlower).position.z += (float)GetRandomValue(-halfFlowerInterval, halfFlowerInterval);
					
					(*newFlower).type = FLOWER;
					(*newFlower).modelIndex = GetRandomValue(0, MAXFLOWERS - 1);
					
					(*newPollen).type = MISC;
					(*newPollen).position.x = (*newFlower).position.x;
					(*newPollen).position.y = (* (struct GameplayData *)moduleData).flowerHitY[ (*newFlower).modelIndex ];
					(*newPollen).position.z = (*newFlower).position.z;
					
					(*newPollen).colliderIndex = Vector2Zero();
					(*newPollen).colliderPrev = NULL;
					(*newPollen).colliderNext = NULL;
					IndexCollider( newPollen );
					(*newPollen).visible = true;
				}
			}
			
		}
	}
	
	(* (struct GameplayData *)moduleData).tileListStart = dummy.prev;
	(* (struct GameplayData *)moduleData).tileListEnd = dummy.next;
}

void PopulateModelCache(char *curDir, Model *models, Texture2D *textures, int *modelCount, int maxCount)
{
	(*modelCount) = 0;
	memset(models, '\0', sizeof(Model) * maxCount);
	
	int fileCount = 0;
    char **files = GetDirectoryFiles(curDir, &fileCount);
    
    for (int x = 0; x < fileCount; x++)
    {
		char filePath[1024];
		memset(filePath, '\0', sizeof(char) * 1024);
		strcpy(filePath, curDir );
		strcat(filePath, "/");
		strcat(filePath, files[x]);
		//TraceLog(LOG_INFO, filePath);
		
		if (DirectoryExists(filePath))
		{
			if (strcmp(files[x], ".") != 0 && strcmp(files[x], ".."))
			{
				//TraceLog(LOG_INFO, filePath);
				//PopulateModelCache(filePath, models, modelCount, maxCount);//bugged to recurse, not sure if my code or raylibs (get/clear Dir)
			}
		}
		else//not a directory
		{
			if (FileExists(filePath) && IsFileExtension(files[x], ".obj"))
			{
				if ( (*modelCount) >= maxCount )
				{
					TraceLog(LOG_INFO, "maxCount");
					break;
				}
				
				//TraceLog(LOG_INFO, filePath);
				//TraceLog(LOG_INFO, files[x]);
								
				*(models + (*modelCount) ) = LoadModel( filePath );
				
				char texFilePath[1024];
				memset(texFilePath, '\0', sizeof(char) * 1024);
				strcpy(texFilePath, curDir );
				strcat(texFilePath, "/");
				strcat(texFilePath, GetFileNameWithoutExt(files[x]) );
				strcat(texFilePath, ".png");
				
				*(textures + (*modelCount) ) = LoadTexture( texFilePath );
				
				SetMaterialTexture(
				&( ( *(models + (*modelCount) ) ).materials[0] ),
				MAP_DIFFUSE,
				*(textures + (*modelCount) ) );
				
				(*modelCount)++;
			}
		}
	}
	
	ClearDirectoryFiles();
}

void RemovePlayers()
{
	struct Player *curNode;
	struct Player *nextNode;
	curNode = (* (struct GameplayData *)moduleData).playerListStart;
	
	while (curNode != NULL)
	{
		free( (*curNode).anims );
		
		nextNode = (*curNode).next;
		
		free(curNode);
		
		curNode = nextNode;
	}
	
	(* (struct GameplayData *)moduleData).playerListStart = NULL;
	(* (struct GameplayData *)moduleData).playerListEnd = NULL;
}

void InitPlayers()
{
	struct Player dummy;
	dummy.prev = NULL;
	dummy.next = NULL;
	
	const char *workDir = GetWorkingDirectory();
	char curDir[1024];
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/players");
	
	(* (struct GameplayData *)moduleData).playerCount = 0;
	memset( (* (struct GameplayData *)moduleData).playerModels , '\0', sizeof(Model) * MAXPLAYERS);
	memset( (* (struct GameplayData *)moduleData).playerAnims , '\0', sizeof(ModelAnimation*) * MAXPLAYERS);
	memset( (* (struct GameplayData *)moduleData).playerTex , '\0', sizeof(Texture2D) * MAXPLAYERS);
	
	int fileCount = 0;
    char **files = GetDirectoryFiles(curDir, &fileCount);
    
    for (int x = 0; x < fileCount; x++)
    {
		char filePath[1024];
		memset(filePath, '\0', sizeof(char) * 1024);
		strcpy(filePath, curDir );
		strcat(filePath, "/");
		strcat(filePath, files[x]);
		//TraceLog(LOG_INFO, filePath);
		
		if (DirectoryExists(filePath))
		{
			if (strcmp(files[x], ".") != 0 && strcmp(files[x], ".."))
			{
				//TraceLog(LOG_INFO, filePath);
				//PopulateModelCache(filePath, models, modelCount, maxCount);//bugged to recurse, not sure if my code or raylibs (get/clear Dir)
			}
		}
		else//not a directory
		{
			if (FileExists(filePath) && IsFileExtension(files[x], ".iqm"))
			{
				if ( (* (struct GameplayData *)moduleData).playerCount >= MAXPLAYERS )
				{
					TraceLog(LOG_INFO, "maxCount");
					break;
				}
				
				//TraceLog(LOG_INFO, filePath);
				//TraceLog(LOG_INFO, files[x]);
								
				(* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ] = LoadModel( filePath );
				
				int animsCount = 0;
				(* (struct GameplayData *)moduleData).playerAnims[ (* (struct GameplayData *)moduleData).playerCount ] = LoadModelAnimations( filePath, &animsCount );
				
				char texFilePath[1024];
				memset(texFilePath, '\0', sizeof(char) * 1024);
				strcpy(texFilePath, curDir );
				strcat(texFilePath, "/");
				strcat(texFilePath, GetFileNameWithoutExt(files[x]) );
				//traditional
				strcat(texFilePath, ".png");
				
				(* (struct GameplayData *)moduleData).playerTex[ (* (struct GameplayData *)moduleData).playerCount ] = LoadTexture( texFilePath );
				
				SetMaterialTexture(
				&( (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0] ),
				MAP_DIFFUSE,
				(* (struct GameplayData *)moduleData).playerTex[ (* (struct GameplayData *)moduleData).playerCount ] );
				
				//pbr: didn't work or i did something wrong.
				/*LoadMaterialPBR( &( (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0] ), texFilePath );
				
				CreateLight(LIGHT_POINT, (Vector3){ LIGHT_DISTANCE, LIGHT_HEIGHT, 0.0f }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 255, 0, 0, 255 }, (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0].shader);
				CreateLight(LIGHT_POINT, (Vector3){ 0.0f, LIGHT_HEIGHT, LIGHT_DISTANCE }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 0, 255, 0, 255 }, (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0].shader);
				CreateLight(LIGHT_POINT, (Vector3){ -LIGHT_DISTANCE, LIGHT_HEIGHT, 0.0f }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 0, 0, 255, 255 }, (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0].shader);
				CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 0.0f, LIGHT_HEIGHT*2.0f, -LIGHT_DISTANCE }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 255, 0, 255, 255 }, (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0].shader);
				*///
				
				struct Player *newPlayer = (struct Player *)malloc( sizeof(struct Player) );
				
				(*newPlayer).node.modelIndex = (* (struct GameplayData *)moduleData).playerCount;
				(*newPlayer).node.type = PLAYER;
				(*newPlayer).state = IDLE;
				(*newPlayer).elapsedTime = 0.0f;
				(*newPlayer).curFrame = 0;
				(*newPlayer).node.position = (Vector3){0.0f, 0.1f, 10.0f};
				(*newPlayer).prevRot = MatrixRotateXYZ( Vector3Zero() );
			
				(*newPlayer).prev = NULL;
				(*newPlayer).next = NULL;
				
				if (dummy.prev == NULL)
				{
					dummy.prev = newPlayer;
					dummy.next = newPlayer;
				}
				else
				{
					(*newPlayer).prev = dummy.next;
					(*dummy.next).next = newPlayer;
					dummy.next = newPlayer;
				}
						
				char animFilePath[1024];
				memset(animFilePath, '\0', sizeof(char) * 1024);
				strcpy(animFilePath, curDir );
				strcat(animFilePath, "/");
				strcat(animFilePath, GetFileNameWithoutExt(files[x]) );
				strcat(animFilePath, ".txt");
				
				FILE *fp;
				char buff[255];
				memset(buff, '\0', sizeof(char) * 255);
				fp = fopen(animFilePath, "r");
				fgets(buff, 255, fp);
				TraceLog(LOG_INFO, buff);
				
				int animCount = atoi(buff);
				
				(*newPlayer).anims = (struct PlayerAnim *)malloc( sizeof(struct PlayerAnim) * animCount);
				
				for (int y = 0; y < animCount; y++)
				{
					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(*newPlayer).anims[y].startFrame = atoi(buff);

					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(*newPlayer).anims[y].endFrame = atoi(buff);
					
					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(*newPlayer).anims[y].id = atoi(buff);
					
					if ( (*newPlayer).anims[y].id < MAXANIMSTATES )
					{
						(*newPlayer).stateAnims[ (*newPlayer).anims[y].id ] = y;
					}
					
					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(*newPlayer).anims[y].timeInterval = atof(buff);
				}
				
				fclose(fp);
				
				( (* (struct GameplayData *)moduleData).playerCount ) += 1;

			}
		}
	}
	
	(* (struct GameplayData *)moduleData).playerListStart = dummy.prev;
	(* (struct GameplayData *)moduleData).playerListEnd = dummy.next;
	(* (struct GameplayData *)moduleData).curPlayer = dummy.prev;
	
	(* (* (struct GameplayData *)moduleData).curPlayer).node.colliderIndex = Vector2Zero();
	(* (* (struct GameplayData *)moduleData).curPlayer).node.colliderPrev = NULL;
	(* (* (struct GameplayData *)moduleData).curPlayer).node.colliderNext = NULL;
	
	IndexCollider( &( (* (* (struct GameplayData *)moduleData).curPlayer).node ) );
	
	ClearDirectoryFiles();
}

void ClampPlayerRot(float *rot)
{
	if (*rot > PI*2)
	{
		*rot -= PI*2;
	}
	return;
	if ( *rot < ROTMAXCLAMP * DEG2RAD )
	{
		*rot = ROTMINCLAMP * DEG2RAD;
	}
	else if ( *rot > ROTMINCLAMP * DEG2RAD )
	{
		*rot = ROTMAXCLAMP * DEG2RAD;
	}
}

void UpdatePlayerState()
{
	bool updateAnim = false;
	
	if ( IsKeyDown(KEY_A) )
	{
		(* (struct GameplayData *)moduleData).moveDir.x = -1.0f;
		
		(* (struct GameplayData *)moduleData).playerRotAngle -= ROTSPEED * dt.deltaTime;
		
		//ClampPlayerRot( &( (* (struct GameplayData *)moduleData).playerRotAngle ) );
	}
	else if ( IsKeyDown(KEY_D) )
	{
		(* (struct GameplayData *)moduleData).moveDir.x = 1.0f;
		
		(* (struct GameplayData *)moduleData).playerRotAngle += ROTSPEED * dt.deltaTime;
		
		//ClampPlayerRot( &( (* (struct GameplayData *)moduleData).playerRotAngle ) );
	}
	else
	{
		(* (struct GameplayData *)moduleData).moveDir.x = 0.0f;
	}
	
	if ( IsKeyDown(KEY_S) )
	{
		(* (struct GameplayData *)moduleData).moveDir.z = 1.0f;
	}
	else if ( IsKeyDown(KEY_W) )
	{
		(* (struct GameplayData *)moduleData).moveDir.z = -1.0f;
	}
	else
	{
		(* (struct GameplayData *)moduleData).moveDir.z = 0.0f;
	}
	
	if ( (* (struct GameplayData *)moduleData).moveDir.x != 0.0f || (* (struct GameplayData *)moduleData).moveDir.z != 0.0f )
	{
		updateAnim = true;
		(* (* (struct GameplayData *)moduleData).curPlayer).elapsedTime += dt.deltaTime;
	}
	
	if (updateAnim == true && (* (* (struct GameplayData *)moduleData).curPlayer).state == IDLE )
	{
		(* (* (struct GameplayData *)moduleData).curPlayer).elapsedTime = 0.0f;
		(* (* (struct GameplayData *)moduleData).curPlayer).state = RUN;
		
		(* (* (struct GameplayData *)moduleData).curPlayer).curFrame =
		(* (* (struct GameplayData *)moduleData).curPlayer).anims[ (* (* (struct GameplayData *)moduleData).curPlayer).stateAnims[ (* (* (struct GameplayData *)moduleData).curPlayer).state ] ].startFrame;
	}
	else if (updateAnim == false && (* (* (struct GameplayData *)moduleData).curPlayer).state == RUN )
	{
		(* (* (struct GameplayData *)moduleData).curPlayer).elapsedTime = 0.0f;
		(* (* (struct GameplayData *)moduleData).curPlayer).state = IDLE;
		
		(* (* (struct GameplayData *)moduleData).curPlayer).curFrame =
		(* (* (struct GameplayData *)moduleData).curPlayer).anims[ (* (* (struct GameplayData *)moduleData).curPlayer).stateAnims[ (* (* (struct GameplayData *)moduleData).curPlayer).state ] ].startFrame;
	}
	
	if
	( 
	
	(* (* (struct GameplayData *)moduleData).curPlayer).elapsedTime >=
	(* (* (struct GameplayData *)moduleData).curPlayer).anims[ (* (* (struct GameplayData *)moduleData).curPlayer).stateAnims[ (* (* (struct GameplayData *)moduleData).curPlayer).state ] ].timeInterval
	)
	{
		(* (* (struct GameplayData *)moduleData).curPlayer).elapsedTime = 0.0f;
		(* (* (struct GameplayData *)moduleData).curPlayer).curFrame += 1;
		
		if
		(
		
		(* (* (struct GameplayData *)moduleData).curPlayer).curFrame >
		(* (* (struct GameplayData *)moduleData).curPlayer).anims[ (* (* (struct GameplayData *)moduleData).curPlayer).stateAnims[ (* (* (struct GameplayData *)moduleData).curPlayer).state ] ].endFrame
		)
		{
			(* (* (struct GameplayData *)moduleData).curPlayer).curFrame =
			(* (* (struct GameplayData *)moduleData).curPlayer).anims[ (* (* (struct GameplayData *)moduleData).curPlayer).stateAnims[ (* (* (struct GameplayData *)moduleData).curPlayer).state ] ].startFrame;
		}
	}
	
	UpdateModelAnimation(
	(* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ],
	*( (* (struct GameplayData *)moduleData).playerAnims[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ] ),
	(* (* (struct GameplayData *)moduleData).curPlayer).curFrame);
}

void DrawPlayer()
{
	DrawModel( (* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ], (Vector3){0,0,0}, 1.0f, WHITE);
}

void CheckTileCollision()
{
	//temporary debugging code
	(* (struct GameplayData *)moduleData).curflower = NULL;
	struct Tile *curNode;
	struct Tile *nextNode;
	struct Tile *closestTile;
	curNode = (* (struct GameplayData *)moduleData).tileListStart;
	closestTile = curNode;
	
	float dist = Vector3Distance( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, (* (*curNode).floor).position );
		
	while (curNode != NULL)
	{
		float newDist = Vector3Distance( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, (* (*curNode).floor).position );
		
		if ( newDist < dist )
		{
			dist = newDist;
			closestTile = curNode;
		}
				
		curNode = (*curNode).next;
	}
	
	(* (struct GameplayData *)moduleData).curTile = closestTile;
	
	struct Node *curFlower;
	struct Node *nextFlower;
	struct Node *closestFlower;
	curFlower = (*closestTile).flowerListStart;
	closestFlower = curFlower;
	
	dist = Vector3Distance( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, (*curFlower).position );
		
	while (curFlower != NULL)
	{
		float newDist = Vector3Distance( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, (*curFlower).position );
		
		if ( newDist < dist )
		{
			dist = newDist;
			closestFlower = curFlower;
		}
				
		curFlower = (*curFlower).next;
	}
	
	(* (struct GameplayData *)moduleData).curflower = closestFlower;
}

void DebugDrawNormals(Model *model)
{
	for (int m = 0; m < (*model).meshCount; m++)
    {
        if ( (*model).meshes[m].normals != NULL)
        {
			for (int i = 0, v = 0; i < (*model).meshes[m].vertexCount; i++, v += 3)
			{
				Vector3 vertex;
				Vector3 normal;
				
				vertex.x = (*model).meshes[m].vertices[v];
				vertex.y = (*model).meshes[m].vertices[v + 1];
				vertex.z = (*model).meshes[m].vertices[v + 2];
				
				normal.x = (*model).meshes[m].normals[v];
				normal.y = (*model).meshes[m].normals[v + 1];
				normal.z = (*model).meshes[m].normals[v + 2];
				
				vertex = Vector3Transform(vertex, (*model).transform);
				normal = Vector3Transform(normal, (*model).transform);
				
				Vector3 offset = Vector3Scale( normal, 0.5f );
				offset = Vector3Add( vertex, offset );
                DrawLine3D(vertex, offset, GREEN);
			}
        }
    }
}

void MovePlayer()
{//TODO: forward projection collision detection to prevent tunneling.
	//TODO: use a colliders list instead of hardcoding floor/building/flowers.
	Vector3 normalEnd, offset;
	
	CheckTileCollision();
	//DebugDrawNormals( &( (* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ] ) );

	bool applyGrav = false;
	
	float zDir = ( (* (struct GameplayData *)moduleData).moveDir.z);
	
	Model *model = &( (* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ] );
	
	//translate model to world position
	Matrix translation = MatrixTranslate( 
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.x,
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y,
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.z );
	
	//rotate model to angle
	Matrix rotation = MatrixRotateXYZ( (Vector3){0.0f, DEG2RAD * ( (* (struct GameplayData *)moduleData).playerRotAngle ), 0.0f} );
	
	//reapply previous hit rotation
	rotation = MatrixMultiply( rotation, (* (* (struct GameplayData *)moduleData).curPlayer).prevRot );
	
	Matrix transform = MatrixMultiply(rotation, translation);
	
	(*model).transform = transform;
	
	//get backward direction and negate for forward direction.
	Vector3 forwardDir = (Vector3){transform.m8, transform.m9, transform.m10};
	//forwardDir = Vector3Negate(forwardDir);
	
	//move player
	if ( (* (struct GameplayData *)moduleData).moveDir.z != 0.0f )
	{	
		//scale forward direction by movement step and set direction by input
		Vector3 offset = Vector3Scale(forwardDir, (zDir * PLAYERSPEED) * dt.deltaTime);
		
		//add the offset to the world position to get the new position
		offset = Vector3Add(offset, (* (* (struct GameplayData *)moduleData).curPlayer).node.position);
		
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = offset;
		
		translation = MatrixTranslate( offset.x, offset.y, offset.z );
		
		(*model).transform = MatrixMultiply(rotation, translation);
		
		IndexCollider( &( (* (* (struct GameplayData *)moduleData).curPlayer).node ) );
		
		struct Node *pollenHit = NULL;
		pollenHit = CheckNodeCollision( &( (* (* (struct GameplayData *)moduleData).curPlayer).node ) );
		
		if ( pollenHit != NULL )
		{
			if ( (*pollenHit).visible == true )
			{
				(*pollenHit).visible = false;
				ScheduleTask( &RespawnPollen, pollenHit, 30.0f);
			}
		}
	}
	
	//raycast for normal alignment.
	RayHitInfo buildingDownHit, floorDownHit, buildingForwardHit, floorForwardHit, flowerDownHit, flowerForwardHit;
	Ray rayTest;
	Vector3 buildingHitNormal, floorHitNormal, hitNormal, buildingHitPos, floorHitPos, hitPos, flowerHitNormal, flowerHitPos;
	bool buildingHit = true;
	bool floorHit = true;
	bool flowerHit = true;
	
	//raise raycast point to above ground.
	Vector3 upVec = (Vector3){ (*model).transform.m4, (*model).transform.m5, (*model).transform.m6 };
	rayTest.direction = Vector3Negate( upVec );
	upVec = Vector3Scale( upVec, 0.5f );
	rayTest.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, upVec);
	
	//debug
	/*offset = Vector3Scale( rayTest.direction, DOWNRAYMAXDIST );
	normalEnd = Vector3Add( rayTest.position, offset );
	DrawLine3D(rayTest.position, normalEnd, GREEN);*/
	
	//transform building
	Matrix rot = MatrixIdentity();
	Matrix trans = MatrixTranslate(
	(* (* (* (struct GameplayData *)moduleData).curTile).building).position.x,
	(* (* (* (struct GameplayData *)moduleData).curTile).building).position.y,
	(* (* (* (struct GameplayData *)moduleData).curTile).building).position.z);

	(* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ].transform = MatrixMultiply(rot,trans);
	
	//raycast building
	buildingDownHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ] );
	
	//transform floor
	trans = MatrixTranslate(
	(* (* (* (struct GameplayData *)moduleData).curTile).floor).position.x,
	(* (* (* (struct GameplayData *)moduleData).curTile).floor).position.y,
	(* (* (* (struct GameplayData *)moduleData).curTile).floor).position.z);
	
	(* (struct GameplayData *)moduleData).floorModels[ (* (* (* (struct GameplayData *)moduleData).curTile).floor).modelIndex ].transform = MatrixMultiply(rot,trans);
	
	//raycast floor
	floorDownHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).floorModels[ (* (* (* (struct GameplayData *)moduleData).curTile).floor).modelIndex ] );
	
	//transform flower
	trans = MatrixTranslate(
	(* (* (struct GameplayData *)moduleData).curflower ).position.x,
	(* (* (struct GameplayData *)moduleData).curflower ).position.y,
	(* (* (struct GameplayData *)moduleData).curflower ).position.z);
	
	(* (struct GameplayData *)moduleData).flowerModels[ (* (* (struct GameplayData *)moduleData).curflower ).modelIndex ].transform = MatrixMultiply(rot,trans);
	
	//raycast flower
	flowerDownHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).flowerModels[ (* (* (struct GameplayData *)moduleData).curflower ).modelIndex ] );
	
	//forward
	upVec = Vector3Scale( upVec, 1.0f );
	rayTest.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, upVec);
	rayTest.direction = Vector3Negate( forwardDir );
	
	buildingForwardHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ] );
	floorForwardHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).floorModels[ (* (* (* (struct GameplayData *)moduleData).curTile).floor).modelIndex ] );
	flowerForwardHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).flowerModels[ (* (* (struct GameplayData *)moduleData).curflower ).modelIndex ] );
	
	//debug
	/*offset = Vector3Scale( rayTest.direction, FORWARDRAYMAXDIST );
	normalEnd = Vector3Add( rayTest.position, offset );
	DrawLine3D(rayTest.position, normalEnd, BLUE);*/
	//DrawSphere(rayTest.position, 0.2f, PINK);
	
	//clamp the range of the raycast
	if ( buildingDownHit.distance > DOWNRAYMAXDIST )
	{
		buildingDownHit.hit = false;
	}
	
	if ( floorDownHit.distance > DOWNRAYMAXDIST )
	{
		floorDownHit.hit = false;
	}
	
	if ( buildingForwardHit.distance > FORWARDRAYMAXDIST )
	{
		buildingForwardHit.hit = false;
	}
	
	if ( floorForwardHit.distance > FORWARDRAYMAXDIST )
	{
		floorForwardHit.hit = false;
	}
	
	if ( flowerDownHit.distance > DOWNRAYMAXDIST )
	{
		flowerDownHit.hit = false;
	}
	
	if ( flowerForwardHit.distance > FORWARDRAYMAXDIST )
	{
		flowerForwardHit.hit = false;
	}
	
	
	//check if building hit
	if ( buildingDownHit.hit == true && buildingForwardHit.hit == true)
	{
		//buildingHitNormal = Vector3Lerp( buildingDownHit.normal, buildingForwardHit.normal, 0.5f);
		//buildingHitPos = Vector3Lerp( buildingDownHit.position, buildingForwardHit.position, 0.5f);
		
		//prioritize walls
		buildingHitNormal = buildingForwardHit.normal;
		buildingHitPos = buildingForwardHit.position;
	}
	else if ( buildingDownHit.hit == true )
	{
		buildingHitNormal = buildingDownHit.normal;
		buildingHitPos = buildingDownHit.position;
	}
	else if ( buildingForwardHit.hit == true )
	{
		buildingHitNormal = buildingForwardHit.normal;
		buildingHitPos = buildingForwardHit.position;
	}
	else
	{
		buildingHit = false;
	}
	
	//check if floor hit
	if ( floorDownHit.hit == true && floorForwardHit.hit == true )
	{
		//floorHitNormal = Vector3Lerp( floorDownHit.normal, floorForwardHit.normal, 0.5f);
		//floorHitPos = Vector3Lerp( floorDownHit.position, floorForwardHit.position, 0.5f);
		
		//prioritize walls
		floorHitNormal = floorForwardHit.normal;
		floorHitPos = floorForwardHit.position;
	}
	else if ( floorDownHit.hit == true )
	{
		floorHitNormal = floorDownHit.normal;
		floorHitPos = floorDownHit.position;
	}
	else if ( floorForwardHit.hit == true )
	{
		floorHitNormal = floorForwardHit.normal;
		floorHitPos = floorForwardHit.position;
	}
	else
	{
		floorHit = false;
	}
	
	//check if flower hit
	if ( flowerDownHit.hit == true && flowerForwardHit.hit == true )
	{
		//flowerHitNormal = Vector3Lerp( flowerDownHit.normal, flowerForwardHit.normal, 0.5f);
		//flowerHitPos = Vector3Lerp( flowerDownHit.position, flowerForwardHit.position, 0.5f);
		
		//prioritize walls
		flowerHitNormal = flowerForwardHit.normal;
		flowerHitPos = flowerForwardHit.position;
	}
	else if ( flowerDownHit.hit == true )
	{
		flowerHitNormal = flowerDownHit.normal;
		flowerHitPos = flowerDownHit.position;
	}
	else if ( flowerForwardHit.hit == true )
	{
		flowerHitNormal = flowerForwardHit.normal;
		flowerHitPos = flowerForwardHit.position;
	}
	else
	{
		flowerHit = false;
	}
	
	if ( flowerHit && floorHit )
	{
		hitNormal = Vector3Lerp( flowerHitNormal, floorHitNormal, 0.5f);
		hitPos = Vector3Lerp( flowerHitPos, floorHitPos, 0.5f);
	}
	else if ( buildingHit && flowerHit )
	{
		hitNormal = Vector3Lerp( buildingHitNormal, flowerHitNormal, 0.5f);
		hitPos = Vector3Lerp( buildingHitPos, flowerHitPos, 0.5f);
	}
	else if ( buildingHit && floorHit )
	{
		hitNormal = Vector3Lerp( buildingHitNormal, floorHitNormal, 0.5f);
		hitPos = Vector3Lerp( buildingHitPos, floorHitPos, 0.5f);
	}
	else if ( flowerHit )
	{
		hitNormal = flowerHitNormal;
		
		hitPos = flowerHitPos;
	}
	else if ( buildingHit )
	{
		hitNormal = buildingHitNormal;
		
		hitPos = buildingHitPos;
	}
	else if ( floorHit )
	{
		hitNormal = floorHitNormal;
		
		hitPos = floorHitPos;
	}
	else
	{
		applyGrav = true;
	}
	
	//successful hit, set model rotation to normal direction.
	//TODO: fix player rotation being shifted on some normals.  the math or logic is wrong here.
	if ( buildingHit || floorHit || flowerHit )//TODO: fix transform NAN when reaching edge of cube. parallel normal & ray maybe.
	{	
		Vector3 up = (Vector3){0.0f, 1.0f, 0.0f};
		//thanks to Mauricio Cele Lopez Belon and his answer to the question here: https://stackoverflow.com/questions/63212143/calculating-object-rotation-based-on-plane-normal/63255665#63255665
		Vector3 axis = Vector3Normalize( Vector3CrossProduct( up, hitNormal ) );
		float angle = acosf( Vector3DotProduct( up, hitNormal ) );
		
		Matrix hitM = MatrixRotate( axis, angle );
		rotation = MatrixRotateXYZ( (Vector3){0.0f, DEG2RAD * ( (* (struct GameplayData *)moduleData).playerRotAngle ), 0.0f} );
		rotation = MatrixMultiply( rotation, hitM );
		
		translation = MatrixTranslate(hitPos.x, hitPos.y, hitPos.z);
		(*model).transform = MatrixMultiply(rotation, translation);
		
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = hitPos;
		(* (* (struct GameplayData *)moduleData).curPlayer).prevRot = hitM;
		
		//debug
		/*offset = Vector3Scale( hitNormal, DOWNRAYMAXDIST );
		normalEnd = Vector3Add( hitPos, offset );
		DrawLine3D(hitPos, normalEnd, PURPLE);*/
	}
	
	if (applyGrav)
	{
		Quaternion rotQ = QuaternionFromMatrix( (* (* (struct GameplayData *)moduleData).curPlayer).prevRot );
		Quaternion upQ = QuaternionFromAxisAngle( (Vector3){1.0f, 0.0f, 0.0f}, 0.0f);
		Quaternion resultQ = QuaternionNlerp( rotQ, upQ, 0.1f );
		Matrix result = QuaternionToMatrix( resultQ );
		(* (* (struct GameplayData *)moduleData).curPlayer).prevRot = result;
		
		Vector3 downVec = (Vector3){ ( result.m4 ), ( result.m5 ), ( result.m6 ) };
		offset = Vector3Scale(downVec, GRAVITY * dt.deltaTime);
		
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, offset );
		
		if ( (* (* (struct GameplayData *)moduleData).curPlayer).node.position.y < 0.0f )//clamp to ground
		{
			(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y = 0.0f;
			
			//(* (* (struct GameplayData *)moduleData).curPlayer).prevRot = MatrixIdentity();
		}
		
		translation = MatrixTranslate( 
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position.x,
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y,
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position.z );
		
		rotation = MatrixRotateXYZ( (Vector3){0.0f, DEG2RAD * ( (* (struct GameplayData *)moduleData).playerRotAngle ), 0.0f} );
		
		rotation = MatrixMultiply( rotation, result );
		
		(*model).transform = MatrixMultiply(rotation, translation);
	}
}

void GameplayInit()
{
	SetWindowSize(1024, 768);
	
	dt.elapsedTime = 0.0f;
	ModuleLoop = GameplayLoop;
	
	struct GameplayData *data = (struct GameplayData *)malloc(sizeof(struct GameplayData));
	moduleData = data;
	
	for (int y = 0; y < COLDIM; y++)
	{
		for (int x = 0; x < COLDIM; x++)
		{
			(*data).colliders[x][y] = NULL;
		}
	}
	
	const char *workDir = GetWorkingDirectory();
	char curDir[1024];
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/buildings");
	PopulateModelCache(curDir, (*data).buildingModels, (*data).buildingTex, &( (*data).buildingCount ), MAXBUILDINGS);
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/floors");
	PopulateModelCache(curDir, (*data).floorModels, (*data).floorTex, &( (*data).floorCount ), MAXFLOORS);
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/flowers");
	PopulateModelCache(curDir, (*data).flowerModels, (*data).flowerTex, &( (*data).flowerCount ), MAXFLOWERS);
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/props");
	PopulateModelCache(curDir, (*data).propModels, (*data).propTex, &( (*data).propCount ), MAXPROPS);
	
	//special cases:
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/misc/pollen.obj");
	(*data).pollenModel = LoadModel( curDir );
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/misc/pollen.png");
	(*data).pollenTex = LoadTexture( curDir );
	
	SetMaterialTexture(
	&( (*data).pollenModel.materials[0] ),
	MAP_DIFFUSE,
	(*data).pollenTex );
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/misc/cloud.obj");
	(*data).cloudModel = LoadModel( curDir );
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/misc/cloud.png");
	(*data).cloudTex = LoadTexture( curDir );
	
	SetMaterialTexture(
	&( (*data).cloudModel.materials[0] ),
	MAP_DIFFUSE,
	(*data).cloudTex );
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/misc/droplet.obj");
	(*data).dropletModel = LoadModel( curDir );
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/misc/droplet.png");
	(*data).dropletTex = LoadTexture( curDir );
	
	SetMaterialTexture(
	&( (*data).dropletModel.materials[0] ),
	MAP_DIFFUSE,
	(*data).dropletTex );
	
	(*data).tileListStart = NULL;
	(*data).tileListEnd = NULL;
	InitTiles();
	
	(*data).playerListStart = NULL;
	(*data).playerListEnd = NULL;
	(*data).curPlayer = NULL;
	InitPlayers();
	
	(*data).taskListStart = NULL;
	(*data).taskListEnd = NULL;
	
	(*data).moveDir.x = 0.0f;(*data).moveDir.y = 0.0f;(*data).moveDir.z = 0.0f;
	(*data).playerRotAngle = 0.0f;
	
	drawNodesCam.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    drawNodesCam.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    drawNodesCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    drawNodesCam.fovy = 45.0f;
    drawNodesCam.type = CAMERA_PERSPECTIVE;
    
    //debug camera
    //SetCameraModeEditor(drawNodesCam, CAMERA_FIRST_PERSON);
    //SetCameraMoveControls(KEY_W, KEY_S, KEY_D, KEY_A, KEY_E, KEY_Q);
    
    SetCameraMode( drawNodesCam, CAMERA_THIRD_PERSON );
}

void GameplayExit()
{
	RemoveTiles();
	RemovePlayers();
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).buildingCount; x++)
	{
		UnloadModel( (* (struct GameplayData *)moduleData).buildingModels[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).floorCount; x++)
	{
		UnloadModel( (* (struct GameplayData *)moduleData).floorModels[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).flowerCount; x++)
	{
		UnloadModel( (* (struct GameplayData *)moduleData).flowerModels[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).propCount; x++)
	{
		UnloadModel( (* (struct GameplayData *)moduleData).propModels[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).playerCount; x++)
	{
		//pbr
		//UnloadMaterial( (* (struct GameplayData *)moduleData).playerModels[x].materials[0] );
		UnloadModel( (* (struct GameplayData *)moduleData).playerModels[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).playerCount; x++)
	{
		UnloadModelAnimation( * ( (* (struct GameplayData *)moduleData).playerAnims[x] ) );
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).playerCount; x++)
	{
		UnloadTexture( (* (struct GameplayData *)moduleData).playerTex[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).buildingCount; x++)
	{
		UnloadTexture( (* (struct GameplayData *)moduleData).buildingTex[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).floorCount; x++)
	{
		UnloadTexture( (* (struct GameplayData *)moduleData).floorTex[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).flowerCount; x++)
	{
		UnloadTexture( (* (struct GameplayData *)moduleData).flowerTex[x]);
	}
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).propCount; x++)
	{
		UnloadTexture( (* (struct GameplayData *)moduleData).propTex[x]);
	}
	
	//special cases
	UnloadModel( (* (struct GameplayData *)moduleData).pollenModel );
	UnloadTexture( (* (struct GameplayData *)moduleData).pollenTex );
	UnloadModel( (* (struct GameplayData *)moduleData).cloudModel );
	UnloadTexture( (* (struct GameplayData *)moduleData).cloudTex );
	UnloadModel( (* (struct GameplayData *)moduleData).dropletModel );
	UnloadTexture( (* (struct GameplayData *)moduleData).dropletTex );
}

void GameplayLoop()
{
	//debug camera
	//UpdateEditorCamera(&drawNodesCam);
	
	//pbr
	/*float cameraPos[3] = { drawNodesCam.position.x, drawNodesCam.position.y, drawNodesCam.position.z };
    SetShaderValue(
    (* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ].materials[0].shader,
    (* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ].materials[0].shader.locs[LOC_VECTOR_VIEW],
    cameraPos,
    UNIFORM_VEC3);*/

	PollTasks();
	
	UpdatePlayerState();
	MovePlayer();
	
	drawNodesCam.target = (* (* (struct GameplayData *)moduleData).curPlayer).node.position;
	UpdateCamera( &drawNodesCam );
	
	BeginDrawing();
	
	BeginMode3D(drawNodesCam);

	ClearBackground(RAYWHITE);
	
	//temp for debugging drawing:
	//MovePlayer();
	
	DrawTiles();
	
	DrawPlayer();
	
	EndMode3D();
	
	//
	/*DrawText(TextFormat("player pos: %f %f %f",
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.x,
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y,
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.z),
	190, 250, 20, YELLOW);*/
	
	EndDrawing();
	
	if (WindowShouldClose())
	{
		ExitGame();
	}
}

//debug camera

void ToggleCursor()
{
	if(IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))
	{
		if (IsCursorHidden())
		{
			EnableCursor();
			previousMousePosition = GetMousePosition();
		}
		else
		{
			DisableCursor();
			SetMousePosition(previousMousePosition.x, previousMousePosition.y);
		}
	}
}

void SetCameraModeEditor(Camera camera, int mode)
{
    Vector3 v1 = camera.position;
    Vector3 v2 = camera.target;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;

    EDITORCAMERA.targetDistance = sqrtf(dx*dx + dy*dy + dz*dz);   // Distance to target

    // Camera angle calculation
    EDITORCAMERA.angle.x = atan2f(dx, dz);                        // Camera angle in plane XZ (0 aligned with Z, move positive CCW)
    EDITORCAMERA.angle.z = atan2f(dy, sqrtf(dx*dx + dz*dz));      // Camera angle in plane XY (0 aligned with X, move positive CW)

    EDITORCAMERA.playerEyesPosition = camera.position.y;          // Init player eyes position to camera Y position

    // Lock cursor for first person and third person cameras
    previousMousePosition = GetMousePosition();
    if ((mode == CAMERA_FIRST_PERSON) || (mode == CAMERA_THIRD_PERSON)) DisableCursor();
    else EnableCursor();

    EDITORCAMERA.mode = mode;
}

void UpdateEditorCamera(Camera3D *camera)
{
	bool direction[6] = { IsKeyDown(EDITORCAMERA.moveControl[MOVE_FRONT]),
                          IsKeyDown(EDITORCAMERA.moveControl[MOVE_BACK]),
                          IsKeyDown(EDITORCAMERA.moveControl[MOVE_RIGHT]),
                          IsKeyDown(EDITORCAMERA.moveControl[MOVE_LEFT]),
                          IsKeyDown(EDITORCAMERA.moveControl[MOVE_UP]),
                          IsKeyDown(EDITORCAMERA.moveControl[MOVE_DOWN]) };
                          
    static Vector2 previousMousePosition = { 0.0f, 0.0f };
    Vector2 mousePosition = GetMousePosition();

    Vector2 mousePositionDelta = { 0.0f, 0.0f };
    mousePositionDelta.x = mousePosition.x - previousMousePosition.x;
    mousePositionDelta.y = mousePosition.y - previousMousePosition.y;
    
    previousMousePosition = mousePosition;
                          
	camera->position.x += (sinf(EDITORCAMERA.angle.x)*direction[MOVE_BACK] -
						   sinf(EDITORCAMERA.angle.x)*direction[MOVE_FRONT] -
						   cosf(EDITORCAMERA.angle.x)*direction[MOVE_LEFT] +
						   cosf(EDITORCAMERA.angle.x)*direction[MOVE_RIGHT])* 1.0f;
	
	camera->position.y += (sinf(EDITORCAMERA.angle.z)*direction[MOVE_FRONT] -
						   sinf(EDITORCAMERA.angle.z)*direction[MOVE_BACK] +
						   1.0f*direction[MOVE_UP] - 1.0f*direction[MOVE_DOWN])* 1.0f;
	
	camera->position.z += (cosf(EDITORCAMERA.angle.x)*direction[MOVE_BACK] -
						   cosf(EDITORCAMERA.angle.x)*direction[MOVE_FRONT] +
						   sinf(EDITORCAMERA.angle.x)*direction[MOVE_LEFT] -
						   sinf(EDITORCAMERA.angle.x)*direction[MOVE_RIGHT])* 1.0f;
	
	// Camera orientation calculation
	EDITORCAMERA.angle.x += (mousePositionDelta.x*-CAMERA_MOUSE_MOVE_SENSITIVITY);
	EDITORCAMERA.angle.z += (mousePositionDelta.y*-CAMERA_MOUSE_MOVE_SENSITIVITY);
	
	// Angle clamp
	if (EDITORCAMERA.angle.z > CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD) EDITORCAMERA.angle.z = CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD;
	else if (EDITORCAMERA.angle.z < CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD) EDITORCAMERA.angle.z = CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD;
	
	// Recalculate camera target considering translation and rotation
	Matrix translation = MatrixTranslate(0, 0, (EDITORCAMERA.targetDistance/CAMERA_FREE_PANNING_DIVIDER));
	Matrix rotation = MatrixRotateXYZ((Vector3){ PI*2 - EDITORCAMERA.angle.z, PI*2 - EDITORCAMERA.angle.x, 0 });
	Matrix transform = MatrixMultiply(translation, rotation);
	
	camera->target.x = camera->position.x - transform.m12;
	camera->target.y = camera->position.y - transform.m13;
	camera->target.z = camera->position.z - transform.m14;
}
//

//pbr
static void LoadMaterialPBR(Material *mat, char *path)
{
	Color albedo = (Color){ 255, 255, 255, 255 };
	float metalness = 1.0f;
	float roughness = 1.0f;
	
    (*mat) = LoadMaterialDefault();   // Initialize material to default

    // Load PBR shader (requires several maps)
#if defined(PLATFORM_DESKTOP)
    (*mat).shader = LoadShader("shaders/glsl330/pbr.vs", "shaders/glsl330/pbr.fs");
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    (*mat).shader = LoadShader("shaders/glsl100/pbr.vs", "shaders/glsl100/pbr.fs");
#endif

    // Get required locations points for PBR material
    // NOTE: Those location names must be available and used in the shader code
    (*mat).shader.locs[LOC_MAP_ALBEDO] = GetShaderLocation((*mat).shader, "albedo.sampler");
    (*mat).shader.locs[LOC_MAP_METALNESS] = GetShaderLocation((*mat).shader, "metalness.sampler");
    (*mat).shader.locs[LOC_MAP_NORMAL] = GetShaderLocation((*mat).shader, "normals.sampler");
    (*mat).shader.locs[LOC_MAP_ROUGHNESS] = GetShaderLocation((*mat).shader, "roughness.sampler");
    (*mat).shader.locs[LOC_MAP_OCCLUSION] = GetShaderLocation((*mat).shader, "occlusion.sampler");
    //(*mat).shader.locs[LOC_MAP_EMISSION] = GetShaderLocation((*mat).shader, "emission.sampler");
    //(*mat).shader.locs[LOC_MAP_HEIGHT] = GetShaderLocation((*mat).shader, "height.sampler");
    (*mat).shader.locs[LOC_MAP_IRRADIANCE] = GetShaderLocation((*mat).shader, "irradianceMap");
    (*mat).shader.locs[LOC_MAP_PREFILTER] = GetShaderLocation((*mat).shader, "prefilterMap");
    (*mat).shader.locs[LOC_MAP_BRDF] = GetShaderLocation((*mat).shader, "brdfLUT");

    // Set view matrix location
    (*mat).shader.locs[LOC_MATRIX_MODEL] = GetShaderLocation((*mat).shader, "matModel");
    //(*mat).shader.locs[LOC_MATRIX_VIEW] = GetShaderLocation((*mat).shader, "view");
    (*mat).shader.locs[LOC_VECTOR_VIEW] = GetShaderLocation((*mat).shader, "viewPos");

    //MAP_ALBEDO
    char texFilePath[1024];
	memset(texFilePath, '\0', sizeof(char) * 1024);
	strcpy(texFilePath, path );
	strcat(texFilePath, ".png");
	(*mat).maps[MAP_ALBEDO].texture = LoadTexture(texFilePath);
	//MAP_NORMAL
	memset(texFilePath, '\0', sizeof(char) * 1024);
	strcpy(texFilePath, path );
	strcat(texFilePath, "N");
	strcat(texFilePath, ".png");
	(*mat).maps[MAP_NORMAL].texture = LoadTexture(texFilePath);
    //MAP_METALNESS
	memset(texFilePath, '\0', sizeof(char) * 1024);
	strcpy(texFilePath, path );
	strcat(texFilePath, "M");
	strcat(texFilePath, ".png");
	(*mat).maps[MAP_METALNESS].texture = LoadTexture(texFilePath);
    //MAP_ROUGHNESS
	memset(texFilePath, '\0', sizeof(char) * 1024);
	strcpy(texFilePath, path );
	strcat(texFilePath, "R");
	strcat(texFilePath, ".png");
	(*mat).maps[MAP_ROUGHNESS].texture = LoadTexture(texFilePath);
    //MAP_OCCLUSION
	memset(texFilePath, '\0', sizeof(char) * 1024);
	strcpy(texFilePath, path );
	strcat(texFilePath, "O");
	strcat(texFilePath, ".png");
	(*mat).maps[MAP_OCCLUSION].texture = LoadTexture(texFilePath);
    
    // Set textures filtering for better quality
    SetTextureFilter((*mat).maps[MAP_ALBEDO].texture, FILTER_BILINEAR);
    SetTextureFilter((*mat).maps[MAP_NORMAL].texture, FILTER_BILINEAR);
    SetTextureFilter((*mat).maps[MAP_METALNESS].texture, FILTER_BILINEAR);
    SetTextureFilter((*mat).maps[MAP_ROUGHNESS].texture, FILTER_BILINEAR);
    SetTextureFilter((*mat).maps[MAP_OCCLUSION].texture, FILTER_BILINEAR);
    
    // Enable sample usage in shader for assigned textures
    SetShaderValue((*mat).shader, GetShaderLocation((*mat).shader, "albedo.useSampler"), (int[1]){ 1 }, UNIFORM_INT);
    SetShaderValue((*mat).shader, GetShaderLocation((*mat).shader, "normals.useSampler"), (int[1]){ 1 }, UNIFORM_INT);
    SetShaderValue((*mat).shader, GetShaderLocation((*mat).shader, "metalness.useSampler"), (int[1]){ 1 }, UNIFORM_INT);
    SetShaderValue((*mat).shader, GetShaderLocation((*mat).shader, "roughness.useSampler"), (int[1]){ 1 }, UNIFORM_INT);
    SetShaderValue((*mat).shader, GetShaderLocation((*mat).shader, "occlusion.useSampler"), (int[1]){ 1 }, UNIFORM_INT);

    int renderModeLoc = GetShaderLocation((*mat).shader, "renderMode");
    SetShaderValue((*mat).shader, renderModeLoc, (int[1]){ 0 }, UNIFORM_INT);

    // Set up material properties color
    (*mat).maps[MAP_ALBEDO].color = albedo;
    (*mat).maps[MAP_NORMAL].color = (Color){ 128, 128, 255, 255 };
    (*mat).maps[MAP_METALNESS].value = metalness;
    (*mat).maps[MAP_ROUGHNESS].value = roughness;
    (*mat).maps[MAP_OCCLUSION].value = 1.0f;
    (*mat).maps[MAP_EMISSION].value = 0.5f;
    (*mat).maps[MAP_HEIGHT].value = 0.5f;
    
    // Generate cubemap from panorama texture
    //--------------------------------------------------------------------------------------------------------
    Texture2D panorama = LoadTexture("shaders/dresden_square_1k.hdr");
    // Load equirectangular to cubemap shader
#if defined(PLATFORM_DESKTOP)
    Shader shdrCubemap = LoadShader("shaders/glsl330/cubemap.vs", "shaders/glsl330/cubemap.fs");
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    Shader shdrCubemap = LoadShader("shaders/glsl100/cubemap.vs", "shaders/glsl100/cubemap.fs");
#endif
    SetShaderValue(shdrCubemap, GetShaderLocation(shdrCubemap, "equirectangularMap"), (int[1]){ 0 }, UNIFORM_INT);
    TextureCubemap cubemap = GenTextureCubemap(shdrCubemap, panorama, CUBEMAP_SIZE, UNCOMPRESSED_R32G32B32);
    UnloadTexture(panorama);
    UnloadShader(shdrCubemap);
    //--------------------------------------------------------------------------------------------------------
    
    // Generate irradiance map from cubemap texture
    //--------------------------------------------------------------------------------------------------------
    // Load irradiance (GI) calculation shader
#if defined(PLATFORM_DESKTOP)
    Shader shdrIrradiance = LoadShader("shaders/glsl330/skybox.vs", "shaders/glsl330/irradiance.fs");
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    Shader shdrIrradiance = LoadShader("shaders/glsl100/skybox.vs", "shaders/glsl100/irradiance.fs");
#endif
    SetShaderValue(shdrIrradiance, GetShaderLocation(shdrIrradiance, "environmentMap"), (int[1]){ 0 }, UNIFORM_INT);
    (*mat).maps[MAP_IRRADIANCE].texture = GenTextureIrradiance(shdrIrradiance, cubemap, IRRADIANCE_SIZE);
    UnloadShader(shdrIrradiance);
    //--------------------------------------------------------------------------------------------------------
    
    // Generate prefilter map from cubemap texture
    //--------------------------------------------------------------------------------------------------------
    // Load reflection prefilter calculation shader
#if defined(PLATFORM_DESKTOP)
    Shader shdrPrefilter = LoadShader("shaders/glsl330/skybox.vs", "shaders/glsl330/prefilter.fs");
#else
    Shader shdrPrefilter = LoadShader("shaders/glsl100/skybox.vs", "shaders/glsl100/prefilter.fs");
#endif
    SetShaderValue(shdrPrefilter, GetShaderLocation(shdrPrefilter, "environmentMap"), (int[1]){ 0 }, UNIFORM_INT);
    (*mat).maps[MAP_PREFILTER].texture = GenTexturePrefilter(shdrPrefilter, cubemap, PREFILTERED_SIZE);
    UnloadTexture(cubemap);
    UnloadShader(shdrPrefilter);
    //--------------------------------------------------------------------------------------------------------
    
    // Generate BRDF (bidirectional reflectance distribution function) texture (using shader)
    //--------------------------------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
    Shader shdrBRDF = LoadShader("shaders/glsl330/brdf.vs", "shaders/glsl330/brdf.fs");
#else
    Shader shdrBRDF = LoadShader("shaders/glsl100/brdf.vs", "shaders/glsl100/brdf.fs");
#endif
    (*mat).maps[MAP_BRDF].texture = GenTextureBRDF(shdrBRDF, BRDF_SIZE);
    UnloadShader(shdrBRDF);
    //--------------------------------------------------------------------------------------------------------

}
