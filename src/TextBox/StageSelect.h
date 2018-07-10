
#ifndef _STAGESELECT_H
#define _STAGESELECT_H

#define NUM_TELEPORTER_SLOTS		8

struct Teleporter_Slot
{
	int slotno;		// which slot # this is (first param to PS+)
	int scriptno;	// which script is run when selected (2nd param to PS+)
};

class TB_StageSelect
{
public:
	TB_StageSelect();
	
	void ResetState();
	void SetVisible(bool enable);
	
	void SetSlot(int slotno, int scriptno);
	void ClearSlots();
	bool GetSlotByIndex(int index, int *slotno_out=NULL, int *scriptno_out=NULL);
	int CountActiveSlots();
	
	bool IsVisible();
	void Draw();
	
private:
	void HandleInput();
	void MoveSelection(int dir);
	void UpdateText();
	
	bool fVisible;
	int fSlots[NUM_TELEPORTER_SLOTS];		// scripts used for slots
	
	int fWarpY;
	
	int fSelectionIndex;
	int fSelectionFrame;
	
	bool fMadeSelection;
};


#endif
