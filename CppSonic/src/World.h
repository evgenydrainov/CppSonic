#pragma once

#include "Objects.h"

#include "TileSet.h"
#include "TileMap.h"

#define MAX_OBJECTS 1000

enum {
	INPUT_RIGHT = 1,
	INPUT_UP    = 1 << 1,
	INPUT_LEFT  = 1 << 2,
	INPUT_DOWN  = 1 << 3,
	INPUT_A     = 1 << 4,
	INPUT_B     = 1 << 5,

	INPUT_JUMP = INPUT_A | INPUT_B
};

struct World;
extern World* world;

struct SensorResult {
	bool found;
	int dist;
	Tile tile;
	int tile_x;
	int tile_y;
};

struct World {
	Player player;
	Object* objects;
	int object_count;
	instance_id next_id;

	float camera_x;
	float camera_y;
	float camera_lock;

	uint32_t input;
	uint32_t input_press;
	uint32_t input_release;

	TileSet tileset;
	TileMap tilemap;

	int target_w;
	int target_h;

	bool debug;

	void Init();
	void Quit();
	void Update(float delta);
	void Draw(float delta);

	Object* CreateObject(ObjType type);

	void UpdatePlayer(Player* p, float delta);

	void GetGroundSensorsPositions(Player* p, float* sensor_a_x, float* sensor_a_y, float* sensor_b_x, float* sensor_b_y);
	void GetPushSensorsPositions  (Player* p, float* sensor_e_x, float* sensor_e_y, float* sensor_f_x, float* sensor_f_y);

	SensorResult SensorCheckDown (float x, float y, int layer);
	SensorResult SensorCheckRight(float x, float y, int layer);
	SensorResult SensorCheckUp   (float x, float y, int layer);
	SensorResult SensorCheckLeft (float x, float y, int layer);

	SensorResult GroundSensorCheck(Player* p, float x, float y);
	SensorResult PushSensorECheck (Player* p, float x, float y);
	SensorResult PushSensorFCheck (Player* p, float x, float y);

	void GroundSensorCollision(Player* p);
	void PushSensorCollision  (Player* p);

	bool AreGroundSensorsActive(Player* p);
	bool IsPushSensorEActive   (Player* p);
	bool IsPushSensorFActive   (Player* p);

	void load_objects(const char* fname);
};
