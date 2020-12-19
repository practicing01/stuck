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
	
	BeginMode3D(drawNodesCam);
		ClearBackground(RAYWHITE);
		
		while (curNode != NULL)
		{
			DrawModel( (* (struct GameplayData *)moduleData).floorModels[ (* (*curNode).floor).modelIndex], (* (*curNode).floor).position, 1.0f, WHITE);
			DrawModel( (* (struct GameplayData *)moduleData).buildingModels[ (* (*curNode).building).modelIndex], (* (*curNode).building).position, 1.0f, WHITE);
			
			curNode = (*curNode).next;
		}
	
	EndMode3D();
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
				if ( (* (struct GameplayData *)moduleData).playerCount >= MAXPLAYERS )
				{
					TraceLog(LOG_INFO, "maxCount");
					break;
				}
				
				//TraceLog(LOG_INFO, filePath);
				//TraceLog(LOG_INFO, files[x]);
								
				*( (* (struct GameplayData *)moduleData).playerModels + (* (struct GameplayData *)moduleData).playerCount ) = LoadModel( filePath );
				
				( (* (struct GameplayData *)moduleData).playerCount ) ++;
				
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
				
				struct Player *newPlayer = (struct Player *)malloc( sizeof(struct Player) );
			
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
				
				(*newPlayer).anims = (struct PlayerAnim *)malloc( sizeof(struct PlayerAnim) * animCount);
				(*newPlayer).node.modelIndex = ((* (struct GameplayData *)moduleData).playerCount) - 1;
				
				for (int y = 0; y < animCount; y++)
				{
					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(* ( (*newPlayer).anims + y) ).startFrame = atoi(buff);

					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(* ( (*newPlayer).anims + y) ).endFrame = atoi(buff);
					
					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(* ( (*newPlayer).anims + y) ).id = atoi(buff);
					
					if ( (* ( (*newPlayer).anims + y) ).id < MAXANIMSTATES )
					{
						(*newPlayer).stateAnims[ (* ( (*newPlayer).anims + y) ).id ] = y;
					}
					
					memset(buff, '\0', sizeof(char) * 255);
					fgets(buff, 255, fp);
					(* ( (*newPlayer).anims + y) ).timeInterval = atof(buff);
				}
				
				fclose(fp);
				
			}
		}
	}
	
	(* (struct GameplayData *)moduleData).playerListStart = dummy.prev;
	(* (struct GameplayData *)moduleData).playerListEnd = dummy.next;
	
	ClearDirectoryFiles();
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
	InitPlayers();
	
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
}

void GameplayLoop()
{
	UpdateEditorCamera(&drawNodesCam);
	
	BeginDrawing();
	
	DrawTiles();
	
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
