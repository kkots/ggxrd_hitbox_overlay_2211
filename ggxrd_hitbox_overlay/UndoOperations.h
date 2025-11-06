#pragma once
#include "Entity.h"
#include <vector>
#include "SortedSprite.h"
#include <memory>
#include "Settings.h"
#include "ThreadUnsafeSharedPtr.h"
#include "DrawHitboxArrayParams.h"

// add sprite (done), select sprite (don't record), select whom to edit (don't record), delete sprite (done), rename sprite (done),
// change anim seq name (done), change anim seq frame (done),
// select boxes (don't record)
// reorder layers (done)
// delete boxes (done)
// add new box (done)
// move or resize boxes with mouse
// move boxes with keyboard
// resize boxes using coordinates fields

enum UndoOperationType {
	UNDO_OPERATION_TYPE_YOU_FUCKED_UP,
	UNDO_OPERATION_TYPE_DELETE_HITBOXES,
	UNDO_OPERATION_TYPE_UNDO_DELETE_HITBOXES,
	UNDO_OPERATION_TYPE_ADD_SPRITE,
	UNDO_OPERATION_TYPE_DELETE_SPRITE,
	UNDO_OPERATION_TYPE_RENAME_SPRITE,
	UNDO_OPERATION_TYPE_SET_ANIM,
	UNDO_OPERATION_TYPE_REORDER_LAYERS,
	UNDO_OPERATION_TYPE_UNDO_REORDER_LAYERS,
	UNDO_OPERATION_TYPE_ADD_BOX,
	UNDO_OPERATION_TYPE_MOVE_RESIZE_BOXES,
	UNDO_OPERATION_TYPE_LAST
};

extern const char* undoOperationName[UNDO_OPERATION_TYPE_LAST];

class UndoOperationBase {
public:
	UndoOperationType type = UNDO_OPERATION_TYPE_YOU_FUCKED_UP;
	virtual ~UndoOperationBase() = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const = 0;
	void fillFromEnt(Entity ent, UndoOperationType type);
	virtual bool combine(const UndoOperationBase& other) { return false; }
	void restoreView();
protected:
	friend class SetAnimOperation;
	UndoOperationBase() = default;
	UndoOperationBase(const UndoOperationBase& other);
	UndoOperationBase(Entity ent);
	Entity backupEntity;
	int bbscrIndexInAswEng;
	bool isPawn;
	char spriteName[32];
	std::vector<int> selectedHitboxes;
	FPACSecondaryData* secondaryData;
	FPAC* fpac;
	BYTE* bbscrFunc;
	FName animSeqName;
	int currentFrame;
	HitboxList hitboxList;
	DWORD engineTick;
	bool combineOk(const UndoOperationBase& other, UndoOperationType requiredType) const;
	static int find(const std::vector<int>& array, int elem);
	void refresh() const;
	mutable SortedSprite* sortedSprite;
	mutable BYTE* jonbin;
};

class UndoDeleteLayersOperation : public UndoOperationBase {
public:
	UndoDeleteLayersOperation(const UndoOperationBase& other);
	virtual ~UndoDeleteLayersOperation() override = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	friend class DeleteLayersOperation;
	std::vector<EditedHitbox> oldLayers;
	std::vector<Hitbox> oldHitboxes;
	std::vector<int> oldSelection;
	short oldCounts[17];
	DWORD oldHitboxesData[17];  // offsets from the start of jonbin
	DWORD oldJonbinSize;
};

class DeleteLayersOperation : public UndoOperationBase {
public:
	DeleteLayersOperation(const UndoOperationBase& other);
	DeleteLayersOperation();
	void fill();
	virtual ~DeleteLayersOperation() override = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	friend class UndoDeleteLayersOperation;
	friend class AddBoxOperation;
	std::vector<int> boxesToDelete;
};

class AddSpriteOperation : public UndoOperationBase {
public:
	AddSpriteOperation(const UndoOperationBase& other);
	AddSpriteOperation();
	void fill();
	virtual ~AddSpriteOperation() override = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	friend class DeleteSpriteOperation;
	char oldSpriteName[32];
	char newSpriteName[32];
	DWORD insertionIndex;
	DWORD newSpriteHash;
	DWORD oldSpriteHash;
};

class DeleteSpriteOperation : public UndoOperationBase {
public:
	DeleteSpriteOperation(const UndoOperationBase& other);
	DeleteSpriteOperation();
	void fill();
	virtual ~DeleteSpriteOperation() override = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	friend class AddSpriteOperation;
	char oldSpriteName[32];
	char newSpriteName[32];
	char oldSpriteToSelectAfterDelete[32];
};

class RenameSpriteOperation : public UndoOperationBase {
public:
	RenameSpriteOperation(const UndoOperationBase& other);
	RenameSpriteOperation();
	void fill(const char (&newSpriteName)[32]);
	virtual ~RenameSpriteOperation() override = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	char newSpriteName[32];
};

class SetAnimOperation : public UndoOperationBase {
public:
	SetAnimOperation(const UndoOperationBase& other);
	SetAnimOperation();
	virtual ~SetAnimOperation() override = default;
	void fill(FName newAnimSeqName, int frame = 0);
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
	virtual bool combine(const UndoOperationBase& other) override;
	static void determineFrameFromEntity(Entity ent, FName* animSeqName, int* currentFrame, int* maxFrame);
private:
	FName oldAnimSeqName;
	int oldMaxFrame;
	int oldFrame;
	FName newAnimSeqName;
	int frame;
	int maxFrame;
};

class ReorderLayersOperation : public UndoOperationBase {
public:
	ReorderLayersOperation();
	ReorderLayersOperation(const UndoOperationBase& other);
	virtual ~ReorderLayersOperation() override = default;
	void fill(int destinationIndex);
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	int destinationIndex;
	std::vector<EditedHitbox> oldLayers;
	std::vector<Hitbox> oldHitboxes;
};

class UndoReorderLayersOperation : public UndoOperationBase {
public:
	UndoReorderLayersOperation();
	UndoReorderLayersOperation(const UndoOperationBase& other);
	void fill(std::vector<EditedHitbox>&& newLayers);
	virtual ~UndoReorderLayersOperation() override = default;
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	friend class ReorderLayersOperation;
	std::vector<EditedHitbox> newLayers;
	std::vector<Hitbox> newHitboxes;
	std::vector<EditedHitbox> oldLayers;
	std::vector<Hitbox> oldHitboxes;
};

class AddBoxOperation : public UndoOperationBase {
public:
	AddBoxOperation();
	AddBoxOperation(const UndoOperationBase& other);
	virtual ~AddBoxOperation() override = default;
	void fill(HitboxType typeToAdd, DrawHitboxArrayParams& params, int left, int top, int right, int bottom);
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
private:
	HitboxType typeToAdd;
	DrawHitboxArrayParams params;
	int left;
	int top;
	int right;
	int bottom;
};

class MoveResizeBoxesOperation : public UndoOperationBase {
public:
	MoveResizeBoxesOperation();
	MoveResizeBoxesOperation(const UndoOperationBase& other);
	virtual ~MoveResizeBoxesOperation() override = default;
	void fill(std::vector<Hitbox>&& oldData, Hitbox* newData, int count);
	virtual bool perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const override;
	void update(Hitbox* data, int count);
	bool combineOk(Entity ent, DWORD engineTick, int timerRange, UndoOperationType requiredType, int hitboxCount) const;
private:
	std::vector<Hitbox> oldData;
	std::vector<Hitbox> newData;
};
