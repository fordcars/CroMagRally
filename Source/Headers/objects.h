//
// Object.h
//
#include "ogl_support.h"

#define INVALID_NODE_FLAG	0xdeadbeef			// put into CType when node is deleted


#define	PLAYER_SLOT		200
#define	ENEMY_SLOT		(PLAYER_SLOT+10)
#define	SLOT_OF_DUMB	3000
#define	SPRITE_SLOT		(SLOT_OF_DUMB+100)
#define	FENCE_SLOT		(SPRITE_SLOT-2)
#define	PARTICLE_SLOT	(SPRITE_SLOT-1)

enum
{
	SKELETON_GENRE,
	DISPLAY_GROUP_GENRE,
	SPRITE_GENRE,
	CUSTOM_GENRE,
	EVENT_GENRE,
	FONTSTRING_GENRE
};


enum
{
	SHADOW_TYPE_CIRCULAR,
	SHADOW_TYPE_CAR_MAMMOTH,
	SHADOW_TYPE_CAR_BONE,
	SHADOW_TYPE_CAR_GEODE,
	SHADOW_TYPE_CAR_LOG,
	SHADOW_TYPE_CAR_TURTLE,
	SHADOW_TYPE_CAR_ROCK,

	SHADOW_TYPE_CAR_TROJAN,
	SHADOW_TYPE_CAR_OBELISK,

	SHADOW_TYPE_CAR_CATAPULT,
	SHADOW_TYPE_CAR_CHARIOT,

	SHADOW_TYPE_CAR_SUBMARINE
};


#define	DEFAULT_GRAVITY		5000.0f


//========================================================

extern	void InitObjectManager(void);
extern	ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef);
extern	void MoveObjects(void);
void DrawObjects(OGLSetupOutputType *setupInfo);

extern	void DeleteAllObjects(void);
extern	void DeleteObject(ObjNode	*theNode);
extern	void DetachObject(ObjNode *theNode);
extern	void GetObjectInfo(ObjNode *theNode);
extern	void UpdateObject(ObjNode *theNode);
extern	ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef);
extern	void AttachGeometryToDisplayGroupObject(ObjNode *theNode, MetaObjectPtr geometry);
extern	void CreateBaseGroup(ObjNode *theNode);
extern	void UpdateObjectTransforms(ObjNode *theNode);
extern	void SetObjectTransformMatrix(ObjNode *theNode);
extern	void DisposeObjectBaseGroup(ObjNode *theNode);
extern	void ResetDisplayGroupObject(ObjNode *theNode);
void AttachObject(ObjNode *theNode);

extern	void MoveStaticObject(ObjNode *theNode);

extern	void CalcNewTargetOffsets(ObjNode *theNode, float scale);

//===================


extern	void AllocateCollisionBoxMemory(ObjNode *theNode, short numBoxes);
extern	void CalcObjectBoxFromNode(ObjNode *theNode);
extern	void CalcObjectBoxFromGlobal(ObjNode *theNode);
extern	void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back);
extern	void UpdateShadow(ObjNode *theNode);
extern	void CullTestAllObjects(void);
ObjNode	*AttachShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ, Boolean checkBlockers);
void CreateCollisionBoxFromBoundingBox(ObjNode *theNode, float tweakXZ, float tweakY);
void CreateCollisionBoxFromBoundingBox_Maximized(ObjNode *theNode);
void CreateCollisionBoxFromBoundingBox_Rotated(ObjNode *theNode, float tweakXZ, float tweakY);
extern	void StopObjectStreamEffect(ObjNode *theNode);
extern	void KeepOldCollisionBoxes(ObjNode *theNode);






