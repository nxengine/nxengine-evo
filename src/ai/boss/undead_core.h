#ifndef _UNDEADCORE_BOSS_H
#define _UNDEADCORE_BOSS_H

#include "../../object.h"
#include "../../stageboss.h"

#define NUM_ROTATORS		4
enum BBoxes { BB_UPPER, BB_BACK, BB_LOWER, BB_TARGET, NUM_BBOXES };

enum UD_Faces
{
	FACE_SKULL,
	FACE_TEETH,
	FACE_MOUTH,
	FACE_MOUTH_LIT,
	FACE_NONE
};

class UDCoreBoss : public StageBoss
{
public:
	void OnMapEntry();
	void OnMapExit();
	void Run();
	void RunAftermove();

private:
	bool RunDefeated();
	void SpawnFaceSmoke();
	void SpawnPellet(int dir);
	void RunHurtFlash(int timer);

	void run_front(Object *o);
	void run_face(Object *o);
	void run_back(Object *o);
	
	void move_bboxes();
	void set_bbox_shootable(bool enable);
	
	Object *create_rotator(int angle, int front);
	void run_rotator(Object *o);
	void SetRotatorStates(int newstate);
	
	Object *main;
	Object *front, *back;
	Object *face;
	
	Object *rotator[NUM_ROTATORS];
	Object *bbox[NUM_BBOXES];
	
};

void onspawn_ud_minicore_idle(Object *o);
void ai_udmini_platform(Object *o);
void ai_ud_pellet(Object *o);
void ai_ud_smoke(Object *o);
void ai_ud_spinner(Object *o);
void ai_ud_spinner_trail(Object *o);
void ai_ud_blast(Object *o);


#endif
