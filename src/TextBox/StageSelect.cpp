
/*
        The stage-select dialog when using the
        teleporter in Arthur's House.
*/

#include "StageSelect.h"

#include "../Utils/Logger.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../nx.h"
#include "../sound/SoundManager.h"
#include "../tsc.h"
using namespace NXE::Graphics;
#include "../autogen/sprites.h"

#define WARP_X (Renderer::getInstance()->screenWidth / 2) - 32
#define WARP_Y (Renderer::getInstance()->screenHeight / 2) - 74

#define WARP_Y_START (WARP_Y + 8)
#define WARP_Y_SPEED 1

#define LOCS_Y (WARP_Y + 16)
#define LOCS_SPACING 8

TB_StageSelect::TB_StageSelect()
{
  ClearSlots();
}

/*
void c------------------------------() {}
*/

void TB_StageSelect::ResetState()
{
  fVisible = false;
}

void TB_StageSelect::SetVisible(bool enable)
{
  fVisible = enable;
  fWarpY   = WARP_Y_START;

  game.frozen = enable;
  textbox.SetFlags(TB_CURSOR_NEVER_SHOWN, enable);
  textbox.SetFlags(TB_LINE_AT_ONCE, enable);
  textbox.SetFlags(TB_VARIABLE_WIDTH_CHARS, enable);

  fSelectionIndex = 0;

  if (enable)
  {
    fMadeSelection = false;
    textbox.ClearText();
    UpdateText();
  }
}

bool TB_StageSelect::IsVisible()
{
  return fVisible;
}

/*
void c------------------------------() {}
*/

void TB_StageSelect::Tick(void)
{
  if (!fVisible)
    return;

  // handle user input
  HandleInput();

  Draw();
}

void TB_StageSelect::Draw(void)
{
  if (!fVisible)
    return;

  // draw "- WARP -" text
  fWarpY -= WARP_Y_SPEED;
  if (fWarpY < WARP_Y)
    fWarpY = WARP_Y;

  Renderer::getInstance()->sprites.drawSprite(WARP_X, fWarpY, SPR_TEXT_WARP, 0);

  // draw teleporter locations
  int nslots        = CountActiveSlots();
  int total_spacing = ((nslots - 1) * LOCS_SPACING);
  int total_width   = total_spacing + (nslots * Renderer::getInstance()->sprites.sprites[SPR_STAGEIMAGE].w);
  int x             = (Renderer::getInstance()->screenWidth / 2) - (total_width / 2);

  for (int i = 0; i < nslots; i++)
  {
    int sprite;
    GetSlotByIndex(i, &sprite, NULL);

    Renderer::getInstance()->sprites.drawSprite(x, LOCS_Y, SPR_STAGEIMAGE, sprite);

    if (i == fSelectionIndex)
    {
      fSelectionFrame ^= 1;
      Renderer::getInstance()->sprites.drawSprite(x, LOCS_Y, SPR_SELECTOR_ITEMS, fSelectionFrame);
    }

    x += (Renderer::getInstance()->sprites.sprites[SPR_STAGEIMAGE].w + LOCS_SPACING);
  }
}

/*
void c------------------------------() {}
*/

void TB_StageSelect::HandleInput()
{
  if (textbox.YesNoPrompt.IsVisible() || fMadeSelection)
    return;

  if (justpushed(LEFTKEY))
  {
    MoveSelection(LEFT);
  }
  else if (justpushed(RIGHTKEY))
  {
    MoveSelection(RIGHT);
  }

  // when user picks a location return the new script to execute
  if (justpushed(ACCEPT_BUTTON))
  {
    int scriptno;
    if (!GetSlotByIndex(fSelectionIndex, NULL, &scriptno))
    {
      LOG_DEBUG("StageSelect: starting activation script %d", scriptno);
      game.tsc->JumpScript(scriptno, TSC::ScriptPages::SP_MAP);
    }
    else
    { // dismiss "no permission to teleport"
      game.tsc->StopScripts();
    }

    fMadeSelection = true;
  }
  else if (justpushed(DECLINE_BUTTON))
  {
    game.tsc->JumpScript(0);
  }
}

void TB_StageSelect::MoveSelection(int dir)
{
  int numslots = CountActiveSlots();
  if (numslots == 0)
    return;

  if (dir == RIGHT)
  {
    if (++fSelectionIndex >= numslots)
      fSelectionIndex = 0;
  }
  else
  {
    if (--fSelectionIndex < 0)
      fSelectionIndex = (numslots - 1);
  }
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
  UpdateText();
}

// updates the text by running the appropriate script
// from StageSelect.tsc
void TB_StageSelect::UpdateText()
{
  int scriptno;

  if (GetSlotByIndex(fSelectionIndex, NULL, &scriptno))
  { // no permission to teleport
    scriptno = 0;
  }
  else
  {
    scriptno %= 1000;
  }

  game.tsc->JumpScript(scriptno + 1000, TSC::ScriptPages::SP_STAGESELECT);
}

/*
void c------------------------------() {}
*/

// set teleporter slot "slotno" to run script "scriptno" when selected.
// this adds the slot to the menu if scriptno is nonzero and removes it if zero.
// the parameters here map directory to the <PS+ in the script.
void TB_StageSelect::SetSlot(int slotno, int scriptno)
{
  if (slotno >= 0 && slotno < NUM_TELEPORTER_SLOTS)
  {
    fSlots[slotno] = scriptno;
  }
  else
  {
    LOG_WARN("StageSelect::SetSlot: invalid slotno %d", slotno);
  }
}

void TB_StageSelect::ClearSlots()
{
  for (int i = 0; i < NUM_TELEPORTER_SLOTS; i++)
    fSlots[i] = -1;
}

// return the slotno and scriptno associated with the n'th enabled teleporter slot,
// where n = index.
// i.e. passing 1 for index returns the 2nd potential teleporter destination.
// if index is higher than the number of active teleporter slots, returns nonzero.
bool TB_StageSelect::GetSlotByIndex(int index, int *slotno_out, int *scriptno_out)
{
  if (index >= 0)
  {
    int slots_found = 0;

    for (int i = 0; i < NUM_TELEPORTER_SLOTS; i++)
    {
      if (fSlots[i] != -1)
      {
        if (++slots_found > index)
        {
          if (slotno_out)
            *slotno_out = i;
          if (scriptno_out)
            *scriptno_out = fSlots[i];
          return 0;
        }
      }
    }
  }

  if (slotno_out)
    *slotno_out = -1;
  if (scriptno_out)
    *scriptno_out = -1;
  return 1;
}

int TB_StageSelect::CountActiveSlots()
{
  int count = 0;

  for (int i = 0; i < NUM_TELEPORTER_SLOTS; i++)
  {
    if (fSlots[i] != -1)
      count++;
  }

  return count;
}
