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
		DrawModel( (* (struct GameplayData *)moduleData).floorModels[ (* (*curNode).floor).modelIndex ], (* (*curNode).floor).position, 1.0f, WHITE);
		DrawModel( (* (struct GameplayData *)moduleData).buildingModels[ (* (*curNode).building).modelIndex ], (* (*curNode).building).position, 1.0f, WHITE);
		
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
	
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
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
		}
	}
	
	(* (struct GameplayData *)moduleData).tileListStart = dummy.prev;
	(* (struct GameplayData *)moduleData).tileListEnd = dummy.next;
	
	//TraceLog(LOG_INFO, "inited tiles");
}

void PopulateModelCache(char *curDir, Model *models, int *modelCount, int maxCount)
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
			if (FileExists(filePath) && IsFileExtension(files[x], ".glb"))
			{
				if ( (*modelCount) >= maxCount )
				{
					TraceLog(LOG_INFO, "maxCount");
					break;
				}
				
				//TraceLog(LOG_INFO, filePath);
				//TraceLog(LOG_INFO, files[x]);
								
				*(models + (*modelCount) ) = LoadModel( filePath );
				
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
		
		(* (struct GameplayData *)moduleData).playerRotAngle -= 5.0f * dt.deltaTime;
		
		ClampPlayerRot( &( (* (struct GameplayData *)moduleData).playerRotAngle ) );
	}
	else if ( IsKeyDown(KEY_RIGHT) )
	{
		(* (struct GameplayData *)moduleData).moveDir.x = 1.0f;
		
		(* (struct GameplayData *)moduleData).playerRotAngle += 5.0f * dt.deltaTime;
		
		ClampPlayerRot( &( (* (struct GameplayData *)moduleData).playerRotAngle ) );
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
	DrawModel( (* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ], (* (* (struct GameplayData *)moduleData).curPlayer).node.position, 1.0f, WHITE);
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

void MovePlayer()//TODO: figure out why player/model sometimes doesn't load.
{//TODO: forward projection collision detection to prevent tunneling.
	CheckTileCollision();
	
	float zDir = ( (* (struct GameplayData *)moduleData).moveDir.z);
	
	Model *model = &( (* (struct GameplayData *)moduleData).playerModels[ (* (* (struct GameplayData *)moduleData).curPlayer).node.modelIndex ] );
	
	//translate model to world position
	Matrix translation = MatrixTranslate( 
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.x,
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.y,
	(* (* (struct GameplayData *)moduleData).curPlayer).node.position.z );
	
	//rotate model to angle
	Matrix rotation = MatrixRotateXYZ( (Vector3){0.0f, (* (struct GameplayData *)moduleData).playerRotAngle, 0.0f} );
	Quaternion playerRot = QuaternionFromMatrix( rotation );
	
	Matrix transform = MatrixMultiply(rotation, translation);
	
	(*model).transform = transform;
	
	//move player
	if ( (* (struct GameplayData *)moduleData).moveDir.z != 0.0f )
	{
		//get backward direction and negate for forward direction.
		Vector3 forwardDir = (Vector3){transform.m8, transform.m9, transform.m10};
		forwardDir = Vector3Negate(forwardDir);
		
		//scale forward direction by movement step and set direction by input
		Vector3 offset = Vector3Scale(forwardDir, zDir * 0.1f);
		
		//add the offset to the world position to get the new position
		offset = Vector3Add(offset, (* (* (struct GameplayData *)moduleData).curPlayer).node.position);
		
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = offset;
		
		translation = MatrixTranslate( offset.x, offset.y, offset.z );
		
		(*model).transform = MatrixMultiply(rotation, translation);
	}
	
	//raycast for normal alignment.
	RayHitInfo buildingDownHit, floorDownHit;
	Ray rayTest;
	Vector3 buildingHitNormal, floorHitNormal, hitNormal, hitPos;
	bool buildingHit = true;
	bool floorHit = true;
	
	//raise raycast point to above ground.
	Vector3 upVec = (Vector3){ (*model).transform.m4, (*model).transform.m5, (*model).transform.m6 };
	upVec = Vector3Scale( upVec, 2.0f );
	rayTest.position = Vector3Add( (* (* (struct GameplayData *)moduleData).curPlayer).node.position, upVec);
	
	DrawSphere(rayTest.position, 0.1f, GREEN);
	
	//down
	rayTest.direction.x = -( (*model).transform.m4 );
	rayTest.direction.y = -( (*model).transform.m5 );
	rayTest.direction.z = -( (*model).transform.m6 );
	
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
	
	//clamp the range of the raycast
	if ( buildingDownHit.distance > RAYMAXDIST )
	{
		buildingDownHit.hit = false;
	}
	
	if ( floorDownHit.distance > RAYMAXDIST )
	{
		floorDownHit.hit = false;
	}
	
	//check if building hit
	if ( buildingDownHit.hit == true )
	{
		buildingHitNormal = buildingDownHit.normal;
	}
	else
	{
		buildingHit = false;
	}
	
	//check if floor hit
	if ( floorDownHit.hit == true )
	{
		floorHitNormal = floorDownHit.normal;
	}
	else
	{
		floorHit = false;
	}
	
	if ( buildingHit )
	{
		hitNormal = buildingHitNormal;
		
		hitPos = buildingDownHit.position;
	}
	else if ( floorHit )
	{
		hitNormal = floorHitNormal;
		
		hitPos = floorDownHit.position;
	}
	else
	{
		//not touching anything. not facing anything.  dafuq, jumping maybe?
	}
	
	//todo flowers.

	//successful hit, set model rotation to normal direction.
	if ( buildingHit || floorHit )
	{
		Quaternion hitRot = QuaternionFromEuler(hitNormal.x, hitNormal.y, hitNormal.z);
		
		playerRot = QuaternionMultiply( hitRot, playerRot );
		rotation = QuaternionToMatrix(playerRot);
		
		translation = MatrixTranslate(hitPos.x, hitPos.y, hitPos.z);
		transform = MatrixMultiply(rotation, translation);
		(*model).transform = transform;
		
		(* (* (struct GameplayData *)moduleData).curPlayer).node.position = hitPos;
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
	PopulateModelCache(curDir, (*data).buildingModels, &( (*data).buildingCount ), MAXBUILDINGS);
	
	memset(curDir, '\0', sizeof(char) * 1024);
	strcpy(curDir, workDir );
	strcat(curDir, "/art/gfx/floors");
	PopulateModelCache(curDir, (*data).floorModels, &( (*data).floorCount ), MAXFLOORS);
	
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
