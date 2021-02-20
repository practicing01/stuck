#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "raylib.h"
#include "moduleLoop.h"
#include "stuck.h"

#define MAXBUILDINGS 4
#define MAXFLOORS 1
#define MAXPLAYERS 1
#define MAXPROPS 5
#define MAXCOLLECTIBLES 1
#define MAXNPCS 1
#define MAXFLOWERS 3
#define TILESIZE 200.0f
#define TILEFACTOR 1
#define MAXANIMSTATES 2
#define ROTMINCLAMP 180.0f
#define ROTMAXCLAMP -180.0f
#define DOWNRAYMAXDIST 1.0f
#define FORWARDRAYMAXDIST 1.0f
#define PLAYERSPEED 10.0f
#define ROTSPEED 360.0f
#define GRAVITY -1.0f
#define COLLIDERINTERVAL 10
#define COLDIM 60//(TILESIZE/COLLIDERINTERVAL) * 3
#define COLLIDEROFFSET 300.0f//(TILESIZE * TILEFACTOR) * 1.5f
#define MAXNPCCOUNT 25
#define NPCSPEED 0.1f

//pbr
#define CUBEMAP_SIZE        1024        // Cubemap texture size
#define IRRADIANCE_SIZE       32        // Irradiance texture size
#define PREFILTERED_SIZE     256        // Prefiltered HDR environment texture size
#define BRDF_SIZE            512        // BRDF LUT texture size
static void LoadMaterialPBR(Material *mat, char *path);
#define PLATFORM_DESKTOP

enum NodeType {BUILDING, PROP, FLOWER, NPCS, PLAYER, FLOOR, MISC};
enum PlayerState {IDLE, RUN};

//debug
int taskCounter;
struct Task
{
	float elapsedTime, maxTime;
	void *data;
	void (*task)(void *data);
	
	struct Task *prev, *next;
	
	//debug
	int id;
};

struct Trigger//shift tiles when player is colliding with just one.
{
	BoundingBox bounds;
	bool colliding;
	Vector3 scale;
	struct Node *collidersListStart, *collidersListEnd, *curCollider;
};

struct Node//todo node should contain animation data.
{
	enum NodeType type;
	int modelIndex;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	int collisionMask;//node belongs to this mask.
	int colliderMask;//node collides with this mask.
	struct Trigger trigger;
	bool visible;
	
	struct Node *prev, *next;
	
	struct Node *colliderPrev, *colliderNext;
	Vector2 colliderIndex;
};

struct Tile
{
	struct Node *building;
	struct Node *floor;
	struct Node *propListStart, *propListEnd, *curProp;
	struct Node *flowerListStart, *flowerListEnd, *curFlower;
	struct Node *pollenListStart, *pollenListEnd, *curPollen;
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

struct NPC
{
	struct Node node;
	int curFrame;
	float elapsedTime;
	Vector3 dest;
	float elapsedLerp;
	Matrix rotation;
	
	struct NPC *prev, *next;
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
	Texture2D propTex[MAXPROPS];
	int flowerCount;
	Model flowerModels[MAXFLOWERS];
	Texture2D flowerTex[MAXFLOWERS];
	float flowerHitY[MAXFLOWERS];
	int npcCount;
	Model npcModels[MAXNPCS];
	Texture2D npcTex[MAXNPCS];
	ModelAnimation *NPCAnims[MAXNPCS];
	
	struct Tile *tileListStart, *tileListEnd, *curTile;
	
	struct Player *playerListStart, *playerListEnd, *curPlayer;
	
	struct Node *curflower;
	
	Camera3D camera;
	
	//special cases
	Model pollenModel;
	Texture2D pollenTex;
	Model cloudModel;
	Texture2D cloudTex;
	Model dropletModel;
	Texture2D dropletTex;
	
	struct Node *colliders[COLDIM][COLDIM];
	struct Task *taskListStart, *taskListEnd;
	
	struct NPC *npcListStart, *npcListEnd;
	struct NPC *npcPoolStart, *npcPoolEnd;
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
void GetFlowerHitY();
void IndexCollider(struct Node *node);
struct Node* CheckNodeCollision(struct Node *node);
void ScheduleTask( void (*task)(void *data), void *data, float maxTime );
void PollTasks();
void RespawnPollen(void *data);
void InitNPCS();
void RemoveNPCS();
void DrawNPCS();
void SpawnNPC();
void ProcessNPCS();

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
