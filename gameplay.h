#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"

#define MAXBUILDINGS 5
#define MAXFLOORS 1
#define MAXPLAYERS 1
#define MAXPROPS 8
#define MAXCOLLECTIBLES 1
#define MAXNPCS 8
#define TILESIZE 200.0f
#define MAXANIMSTATES 2
#define ROTMINCLAMP 180.0f
#define ROTMAXCLAMP -180.0f
#define DOWNRAYMAXDIST 1.0f
#define FORWARDRAYMAXDIST 1.0f
#define PLAYERSPEED 5.0f
#define ROTSPEED 180.0f
#define GRAVITY -1.0f

enum NodeType {BUILDING, PROP, COLLECTABLE, NPC, PLAYER, FLOOR};
enum PlayerState {IDLE, RUN};

struct Node
{
	enum NodeType type;
	int modelIndex;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	int collisionMask;//node belongs to this mask.
	int colliderMask;//node collides with this mask.
	
	struct Node *prev, *next;
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
	struct Node *floor;
	struct Node *propList, *propListStart, *propListEnd;
	struct Node *collectibleList, *collectibleListStart, *collectibleListEnd;
	struct Node *npcList, *npcListStart, *npcListEnd;
	struct Trigger trigger;
	
	Vector2 offset;
	Vector3 position;
	
	struct Tile *prev, *next;
};

struct PlayerAnim
{
	int startFrame;
	int endFrame;
	int id;
	float timeInterval;
};

struct Player
{
	struct Node node;
	enum PlayerState state;
	int curFrame;
	float elapsedTime;
	struct PlayerAnim *anims;
	int stateAnims[MAXANIMSTATES];
	Matrix prevRot;
	
	struct Player *prev, *next;
};

struct GameplayData
{
	Vector3 moveDir;
	float playerRotAngle;
	
	int buildingCount;
	Model buildingModels[MAXBUILDINGS];
	Texture2D buildingTex[MAXBUILDINGS];
	int floorCount;
	Model floorModels[MAXFLOORS];
	Texture2D floorTex[MAXFLOORS];
	int playerCount;
	Model playerModels[MAXPLAYERS];
	ModelAnimation *playerAnims[MAXPLAYERS];
	Texture2D playerTex[MAXPLAYERS];
	int propCount;
	Model propModels[MAXPROPS];
	int collectibleCount;
	Model collectibleModels[MAXCOLLECTIBLES];
	int npcCount;
	Model npcModels[MAXNPCS];
	
	struct Tile *tileListStart, *tileListEnd, *curTile;
	
	struct Player *playerListStart, *playerListEnd, *curPlayer;
	
	Camera3D camera;
};

void DebugDrawNormals(Model *model);
void ClampPlayerRot(float *rot);
void DrawPlayer();
void MovePlayer();
void UpdatePlayerState();
void InitPlayers();
void RemovePlayers();
void PopulateModelCache(char *curDir, Model *models, Texture2D *textures, int *modelCount, int maxCount);
void CheckTileCollision();
void InitTiles();
void RemoveTiles();
void DrawTiles();

void GameplayInit();
void GameplayExit();
void GameplayLoop();


//debug camera
void ToggleCursor();

Camera3D drawNodesCam;

typedef struct {
    unsigned int mode;              // Current camera mode
    float targetDistance;           // Camera distance from position to target
    float playerEyesPosition;       // Player eyes position from ground (in meters)
    Vector3 angle;                  // Camera angle in plane XZ

    // Camera movement control keys
    int moveControl[6];             // Move controls (CAMERA_FIRST_PERSON)
    int smoothZoomControl;          // Smooth zoom control key
    int altControl;                 // Alternative control key
    int panControl;                 // Pan view control key
} CameraData;

static CameraData EDITORCAMERA = {        // Global CAMERA state context
    .mode = 0,
    .targetDistance = 0,
    .playerEyesPosition = 1.85f,
    .angle = { 0 },
    .moveControl = { 'W', 'S', 'D', 'A', 'E', 'Q' },
    .smoothZoomControl = 341,       // raylib: KEY_LEFT_CONTROL
    .altControl = 342,              // raylib: KEY_LEFT_ALT
    .panControl = 2                 // raylib: MOUSE_MIDDLE_BUTTON
};

typedef enum {
    MOVE_FRONT = 0,
    MOVE_BACK,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN
} CameraMove;

Vector2 previousMousePosition;

float PLAYER_MOVEMENT_SENSITIVITY;
#define CAMERA_MOUSE_MOVE_SENSITIVITY 0.003f
#define CAMERA_FIRST_PERSON_MIN_CLAMP 89.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP -89.0f
#define CAMERA_FREE_PANNING_DIVIDER 5.1f

void SetCameraModeEditor(Camera camera, int mode);
void UpdateEditorCamera(Camera3D *camera);
void UpdateEditorCameraCustom(Camera3D *camera);
//

#endif
