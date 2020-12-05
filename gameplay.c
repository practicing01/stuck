#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"
#include "gameplay.h"
#include <string.h>
#include <stdlib.h>

void PopulateModelCache(char *curDir, Model *models, int *modelCount, int maxCount)
{
	(*modelCount) = 0;
	memset(models, '\0', sizeof(Model) * maxCount);
	
	int fileCount = 0;
    char **files = GetDirectoryFiles(curDir, &fileCount);
    
    for (int x = 0; x < fileCount; x++)
    {
		char filePath[1024];
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

void GameplayInit()
{
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
}

void GameplayExit()
{
	for (int x = 0; x < (* (struct GameplayData *)moduleData).buildingCount; x++)
	{
		UnloadModel( (* (struct GameplayData *)moduleData).buildingModels[x]);
	}
}

void GameplayLoop()
{
        
	BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("GameplayLoop", 190, 200, 20, LIGHTGRAY);
	EndDrawing();
	
	if (WindowShouldClose())
	{
		ExitGame();
	}
}
