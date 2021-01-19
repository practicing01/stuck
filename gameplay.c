#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"
#include "gameplay.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "raymath.h"

void DrawTiles()
{
	struct Tile *curNode;
	struct Tile *nextNode;
	curNode = (* (struct GameplayData *)moduleData).tileListStart;
		
	while (curNode != NULL)
	{
		Matrix rotation = MatrixRotateXYZ( Vector3Zero() );
		Matrix translation = MatrixTranslate( 
		(* (*curNode).floor).position.x,
		(* (*curNode).floor).position.y,
		(* (*curNode).floor).position.z );
		
		Matrix transform = MatrixMultiply( rotation, translation );
		
		(* (struct GameplayData *)moduleData).floorModels[ (* (*curNode).floor).modelIndex ].transform = transform;
		(* (struct GameplayData *)moduleData).buildingModels[ (* (*curNode).building).modelIndex ].transform = transform;
		
		DrawModel( (* (struct GameplayData *)moduleData).floorModels[ (* (*curNode).floor).modelIndex ], (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
		DrawModel( (* (struct GameplayData *)moduleData).buildingModels[ (* (*curNode).building).modelIndex ], (Vector3){0.0f,0.0f,0.0f}, 1.0f, WHITE);
		
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
		
		nextNode = (*curNode).next;
		
		free(curNode);
		
		curNode = nextNode;
	}
	
	(* (struct GameplayData *)moduleData).tileListStart = NULL;
	(* (struct GameplayData *)moduleData).tileListEnd = NULL;
}

void InitTiles()
{
	struct Tile dummy;
	dummy.prev = NULL;
	dummy.next = NULL;
	
	for (int y = 0; y <= 0; y++)//for (int y = -1; y <= 1; y++)
	{
		for (int x = 0; x <= 0; x++)//for (int x = -1; x <= 1; x++)
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
			
			//debug
			break;
		}
		//debug
		break;
	}
	
	(* (struct GameplayData *)moduleData).tileListStart = dummy.prev;
	(* (struct GameplayData *)moduleData).tileListEnd = dummy.next;
	
	//TraceLog(LOG_INFO, "inited tiles");
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
				strcat(texFilePath, ".png");
				
				(* (struct GameplayData *)moduleData).playerTex[ (* (struct GameplayData *)moduleData).playerCount ] = LoadTexture( texFilePath );
				
				SetMaterialTexture(
				&( (* (struct GameplayData *)moduleData).playerModels[ (* (struct GameplayData *)moduleData).playerCount ].materials[0] ),
				MAP_DIFFUSE,
				(* (struct GameplayData *)moduleData).playerTex[ (* (struct GameplayData *)moduleData).playerCount ] );
				
				struct Player *newPlayer = (struct Player *)malloc( sizeof(struct Player) );
				
				(*newPlayer).node.modelIndex = (* (struct GameplayData *)moduleData).playerCount;
				(*newPlayer).node.type = PLAYER;
				(*newPlayer).state = IDLE;
				(*newPlayer).elapsedTime = 0.0f;
				(*newPlayer).curFrame = 0;
				(*newPlayer).node.position = (Vector3){0.0f, 0.1f, 5.0f};
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
	
	if ( IsKeyDown(KEY_LEFT) )
	{
		(* (struct GameplayData *)moduleData).moveDir.x = -1.0f;
		
		(* (struct GameplayData *)moduleData).playerRotAngle -= ROTSPEED * dt.deltaTime;
		
		//ClampPlayerRot( &( (* (struct GameplayData *)moduleData).playerRotAngle ) );
	}
	else if ( IsKeyDown(KEY_RIGHT) )
	{
		(* (struct GameplayData *)moduleData).moveDir.x = 1.0f;
		
		(* (struct GameplayData *)moduleData).playerRotAngle += ROTSPEED * dt.deltaTime;
		
		//ClampPlayerRot( &( (* (struct GameplayData *)moduleData).playerRotAngle ) );
	}
	else
	{
		(* (struct GameplayData *)moduleData).moveDir.x = 0.0f;
	}
	
	if ( IsKeyDown(KEY_DOWN) )
	{
		(* (struct GameplayData *)moduleData).moveDir.z = 1.0f;
	}
	else if ( IsKeyDown(KEY_UP) )
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
	//
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
	Vector3 normalEnd, offset;
	
	CheckTileCollision();
	DebugDrawNormals( &( (* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ] ) );

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
	}
	
	//raycast for normal alignment.
	RayHitInfo buildingDownHit, floorDownHit, buildingForwardHit, floorForwardHit;
	Ray rayTest;
	Vector3 buildingHitNormal, floorHitNormal, hitNormal, buildingHitPos, floorHitPos, hitPos;
	bool buildingHit = true;
	bool floorHit = true;
	
	//raise raycast point to above ground.
	Vector3 upVec = (Vector3){ (*model).transform.m4, (*model).transform.m5, (*model).transform.m6 };
	rayTest.direction = Vector3Negate( upVec );
	upVec = Vector3Scale( upVec, 0.5f );
	rayTest.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, upVec);
	
	//debug
	offset = Vector3Scale( rayTest.direction, DOWNRAYMAXDIST );
	normalEnd = Vector3Add( rayTest.position, offset );
	DrawLine3D(rayTest.position, normalEnd, GREEN);
	
	Matrix rot = MatrixIdentity();
	Matrix trans = MatrixTranslate(
	(* (* (* (struct GameplayData *)moduleData).curTile).building).position.x,
	(* (* (* (struct GameplayData *)moduleData).curTile).building).position.y,
	(* (* (* (struct GameplayData *)moduleData).curTile).building).position.z);

	(* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ].transform = MatrixMultiply(rot,trans);
	
	buildingDownHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ] );
	
	trans = MatrixTranslate(
	(* (* (* (struct GameplayData *)moduleData).curTile).floor).position.x,
	(* (* (* (struct GameplayData *)moduleData).curTile).floor).position.y,
	(* (* (* (struct GameplayData *)moduleData).curTile).floor).position.z);
	
	(* (struct GameplayData *)moduleData).floorModels[ (* (* (* (struct GameplayData *)moduleData).curTile).floor).modelIndex ].transform = MatrixMultiply(rot,trans);
	
	floorDownHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).floorModels[ (* (* (* (struct GameplayData *)moduleData).curTile).floor).modelIndex ] );
	
	//forward
	upVec = Vector3Scale( upVec, 1.0f );
	rayTest.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, upVec);
	rayTest.direction = Vector3Negate( forwardDir );
	
	buildingForwardHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).buildingModels[ (* (* (* (struct GameplayData *)moduleData).curTile).building).modelIndex ] );
	floorForwardHit = GetCollisionRayModel(rayTest, (* (struct GameplayData *)moduleData).floorModels[ (* (* (* (struct GameplayData *)moduleData).curTile).floor).modelIndex ] );
	
	//debug
	offset = Vector3Scale( rayTest.direction, FORWARDRAYMAXDIST );
	normalEnd = Vector3Add( rayTest.position, offset );
	DrawLine3D(rayTest.position, normalEnd, BLUE);
	//DrawSphere(rayTest.position, 0.2f, PINK);
	
	//TODO: upcast
	
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
	
	//check if both hit
	if ( buildingHit && floorHit )
	{
		hitNormal = Vector3Lerp( buildingHitNormal, floorHitNormal, 0.5f);
		hitPos = Vector3Lerp( buildingHitPos, floorHitPos, 0.5f);
		
		//prioritize buildings
		//hitNormal = buildingHitNormal;
		//hitPos = buildingHitPos;
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
	
	//todo flowers.

	//successful hit, set model rotation to normal direction.
	if ( buildingHit || floorHit )
	{
		Vector3 cross = Vector3CrossProduct( (Vector3){0.0f, 1.0f, 0.0f}, hitNormal );
		Vector3 axis = Vector3Normalize( cross );
		float angle = asinf( Vector3Length( cross ) );
		Matrix hitM = MatrixRotate( axis, angle );
		rotation = MatrixRotateXYZ( (Vector3){0.0f, DEG2RAD * ( (* (struct GameplayData *)moduleData).playerRotAngle ), 0.0f} );
		rotation = MatrixMultiply( rotation, hitM );
		
		translation = MatrixTranslate(hitPos.x, hitPos.y, hitPos.z);
		(*model).transform = MatrixMultiply(rotation, translation);
		
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = hitPos;
		(* (* (struct GameplayData *)moduleData).curPlayer).prevRot = hitM;
		
		//debug
		offset = Vector3Scale( hitNormal, DOWNRAYMAXDIST );
		normalEnd = Vector3Add( hitPos, offset );
		DrawLine3D(hitPos, normalEnd, PURPLE);
	}
	
	if (applyGrav)
	{//TODO: gradual rotation back to 0;
		//Vector3 downVec = (Vector3){ ( (*model).transform.m4 ), ( (*model).transform.m5 ), ( (*model).transform.m6 ) };
		Vector3 downVec = (Vector3){0.0f, 1.0f, 0.0f};
		offset = Vector3Scale(downVec, GRAVITY * dt.deltaTime);
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, offset );
		
		translation = MatrixTranslate( 
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position.x,
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y,
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position.z );
		
		rotation = MatrixRotateXYZ( (Vector3){0.0f, DEG2RAD * ( (* (struct GameplayData *)moduleData).playerRotAngle ), 0.0f} );
		
		if ( (* (* (struct GameplayData *)moduleData).curPlayer).node.position.y < 0.0f )//clamp to ground
		{
			(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y = 0.0f;
			
			(* (* (struct GameplayData *)moduleData).curPlayer).prevRot = MatrixRotateXYZ( (Vector3){0.0f, 0.0f, 0.0f} );
		}
		
		rotation = MatrixMultiply( rotation, (* (* (struct GameplayData *)moduleData).curPlayer).prevRot );
		
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
	
	(*data).tileListStart = NULL;
	(*data).tileListEnd = NULL;
	InitTiles();
	
	(*data).playerListStart = NULL;
	(*data).playerListEnd = NULL;
	(*data).curPlayer = NULL;
	InitPlayers();
	
	(*data).moveDir.x = 0.0f;(*data).moveDir.y = 0.0f;(*data).moveDir.z = 0.0f;
	(*data).playerRotAngle = 0.0f;
	
	drawNodesCam.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    drawNodesCam.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    drawNodesCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    drawNodesCam.fovy = 45.0f;
    drawNodesCam.type = CAMERA_PERSPECTIVE;
    
    SetCameraModeEditor(drawNodesCam, CAMERA_FIRST_PERSON);
    
    SetCameraMoveControls(KEY_W, KEY_S, KEY_D, KEY_A, KEY_E, KEY_Q);
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
	
	for (int x = 0; x < (* (struct GameplayData *)moduleData).playerCount; x++)
	{
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
}

void GameplayLoop()
{
	UpdateEditorCamera(&drawNodesCam);
	UpdatePlayerState();
	//MovePlayer();
	
	BeginDrawing();
	
	BeginMode3D(drawNodesCam);

	ClearBackground(RAYWHITE);
	
	//temp for debugging:
	MovePlayer();
	
	DrawTiles();
	
	DrawPlayer();
	
	EndMode3D();

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
