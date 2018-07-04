
// TSC script parser & executor

#include "nx.h"
#include "tsc.h"
#include <map>
#include <vector>
#include <fstream>
#include "inventory.h"
#include "ObjManager.h"
#include "niku.h"
#include "sound/sound.h"
#include "sound/org.h"
#include "common/misc.h"
#include "common/stat.h"
#include "game.h"
#include "playerstats.h"
#include "debug.h"
#include "ai/sym/smoke.h"
#include "map.h"
#include "player.h"
#include "endgame/credits.h"
#include "screeneffect.h"
#include "settings.h"
#include "console.h"
#include "ResourceManager.h"

// which textbox options are enabled by the "<TUR" script command.
#define TUR_PARAMS		(TB_LINE_AT_ONCE | TB_VARIABLE_WIDTH_CHARS | TB_CURSOR_NEVER_SHOWN)

/*
void c------------------------------() {}
*/

struct TSCCommandTable
{
	const char *mnemonic;
	int nparams;
};
const TSCCommandTable cmd_table[] =
{
	{"AE+", 0},
	{"AM+", 2},
	{"AM-", 1},
	{"AMJ", 2},
	{"ANP", 3},
	{"BOA", 1},
	{"BSL", 1},
	{"CAT", 0},
	{"CIL", 0},
	{"CLO", 0},
	{"CLR", 0},
	{"CMP", 3},
	{"CMU", 1},
	{"CNP", 3},
	{"CPS", 0},
	{"CRE", 0},
	{"CSS", 0},
	{"DNA", 1},
	{"DNP", 1},
	{"ECJ", 2},
	{"END", 0},
	{"EQ+", 1},
	{"EQ-", 1},
	{"ESC", 0},
	{"EVE", 1},
	{"FAC", 1},
	{"FAI", 1},
	{"FAO", 1},
	{"FL+", 1},
	{"FL-", 1},
	{"FLA", 0},
	{"FLJ", 2},
	{"FMU", 0},
	{"FOB", 2},
	{"FOM", 1},
	{"FON", 2},
	{"FRE", 0},
	{"GIT", 1},
	{"HMC", 0},
	{"INI", 0},
	{"INP", 3},
	{"IT+", 1},
	{"IT-", 1},
	{"ITJ", 2},
	{"KEY", 0},
	{"LDP", 0},
	{"LI+", 1},
	{"ML+", 1},
	{"MLP", 0},
	{"MM0", 0},
	{"MNA", 0},
	{"MNP", 4},
	{"MOV", 2},
	{"MP+", 1},
	{"MPJ", 1},
	{"MS2", 0},
	{"MS3", 0},
	{"MSG", 0},
	{"MYB", 1},
	{"MYD", 1},
	{"NCJ", 2},
	{"NOD", 0},
	{"NUM", 1},
	{"PRI", 0},
	{"PS+", 2},
	{"QUA", 1},
	{"RMU", 0},
	{"SAT", 0},
	{"SIL", 1},
	{"SK+", 1},
	{"SK-", 1},
	{"SKJ", 2},
	{"SLP", 0},
	{"SMC", 0},
	{"SMP", 2},
	{"SNP", 4},
	{"SOU", 1},
	{"SPS", 0},
	{"SSS", 1},
	{"STC", 0},
	{"SVP", 0},
	{"TAM", 3},
	{"TRA", 4},
	{"TUR", 0},
	{"UNI", 1},
	{"UNJ", 1},
	{"WAI", 1},
	{"WAS", 0},
	{"XX1", 1},
	{"YNJ", 1},
	{"ZAM", 0},
	{"ACH", 1},
};

unsigned char codealphabet[] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123+-" };
unsigned char letter_to_code[256];
unsigned char mnemonic_lookup[32*32*32];


static int MnemonicToIndex(const char *str)
{
int l1, l2, l3;

	l1 = letter_to_code[(uint8_t)str[0]];
	l2 = letter_to_code[(uint8_t)str[1]];
	l3 = letter_to_code[(uint8_t)str[2]];
	if (l1==0xff || l2==0xff || l3==0xff) return -1;
	
	return (l1 << 10) | (l2 << 5) | l3;
}

static void GenLTC(void)
{
int i;
uint8_t ch;

	memset(letter_to_code, 0xff, sizeof(letter_to_code));
	for(i=0;;i++)
	{
		if (!(ch = codealphabet[i])) break;
		letter_to_code[ch] = i;
	}
	
	memset(mnemonic_lookup, 0xff, sizeof(mnemonic_lookup));
	for(i=0;i<OP_COUNT;i++)
	{
		mnemonic_lookup[MnemonicToIndex(cmd_table[i].mnemonic)] = i;
	}
}

static int MnemonicToOpcode(char *str)
{
	int index = MnemonicToIndex(str);
	if (index != -1)
	{
		index = mnemonic_lookup[index];
		if (index != 0xff) return index;
	}
	
	staterr("MnemonicToOpcode: No such command '%s'", str);
	return -1;
}

/*
void c------------------------------() {}
*/

TSC::TSC()
{

}

TSC::~TSC()
{

}

bool TSC::Init(void)
{
	GenLTC();
	_curscript.running = false;
	
	// load the "common" TSC scripts available to all maps
	if (!Load(ResourceManager::getInstance()->getLocalizedPath("Head.tsc"), ScriptPages::SP_HEAD)) return false;
	
	// load the inventory screen scripts
	if (!Load(ResourceManager::getInstance()->getLocalizedPath("ArmsItem.tsc"), ScriptPages::SP_ARMSITEM)) return false;
	
	// load stage select/teleporter scripts
	if (!Load(ResourceManager::getInstance()->getLocalizedPath("StageSelect.tsc"), ScriptPages::SP_STAGESELECT)) return false;
	
	return true;
}

void TSC::Close(void)
{
	// free all loaded scripts
	for(int i=0;i<NUM_SCRIPT_PAGES;i++)
		_script_pages[i].Clear();
}

// load a tsc file and return the highest script # in the file
bool TSC::Load(const std::string& fname, ScriptPages pageno)
{
ScriptPage *page = &_script_pages[(int)pageno];
int fsize;
std::string buf;
bool result;

	stat("tsc_load: loading '%s' to page %d", fname.c_str(), pageno);
	if (_curscript.running && _curscript.pageno == (int)pageno)
		StopScript(&_curscript);
	
	page->Clear();
	
	// load the raw script text
	buf = Decrypt(fname, &fsize);
	if (buf.empty())
	{
		staterr("tsc_load: failed to load file: '%s'", fname.c_str());
		return false;
	}

	// now "compile" all the scripts in the TSC
	//int top_script = CompileScripts(buf, fsize, base);
	result = Compile(buf.c_str(), fsize, pageno);
	
	return result;
}


std::string TSC::Decrypt(const std::string& fname, int *fsize_out)
{
    int fsize, i;
    std::ifstream ifs;

    ifs.open (widen(fname), std::ifstream::binary);

    if (!ifs)
    {
        staterr("tsc_decrypt: no such file: '%s'!", fname.c_str());
        return "";
    }

    ifs.seekg (0, ifs.end);
    fsize = ifs.tellg();
    ifs.seekg (0, ifs.beg);

    // load file
    uint8_t *buf = new uint8_t[fsize+1];
    ifs.read((char*)buf, fsize);
    buf[fsize] = 0;
    ifs.close();

    // get decryption key, which is actually part of the text
    int keypos = (fsize / 2);
    uint8_t key = buf[keypos];

    // everything EXCEPT the key is encrypted
    for(i=0;i<keypos;i++) { buf[i] = (buf[i] - key); }
    for(i++;i<fsize;i++)  { buf[i] = (buf[i] - key); }

    if (fsize_out) *fsize_out = fsize;

    std::string ret((char*)buf);
    delete[] buf;
    return ret;
}

/*
void c------------------------------() {}
*/

static char nextchar(const char **buf, const char *buf_end)
{
	if (*buf <= buf_end)
		return *(*buf)++;
	
	return 0;
}

static int ReadNumber(const char **buf, const char *buf_end)
{
static char num[5] = { 0 };
int i = 0;
	
	while(i < 4)
	{
		num[i] = nextchar(buf, buf_end);
		if (!isdigit(num[i]))
		{
			(*buf)--;
			break;
		}
		
		i++;
	}
	
	return atoi(num);
}

static void ReadText(std::vector<uint8_t> *script, const char **buf, const char *buf_end)
{
	while(*buf <= buf_end)
	{
		char ch = nextchar(buf, buf_end);
		if (ch == '<' || ch == '#')
		{
			(*buf)--;
			break;
		}
		
		if (ch != 10)
			script->push_back(ch);
	}
	
	script->push_back('\0');
}

// compile a tsc file--a set of scripts in raw text format--into 'bytecode',
// and place the finished scripts into the given page.
bool TSC::Compile(const char *buf, int bufsize, ScriptPages pageno)
{
ScriptPage *page = &_script_pages[(int)pageno];
const char *buf_end = (buf + (bufsize - 1));
std::vector<uint8_t> *script = NULL;
char cmdbuf[4] = { 0 };

	stat("<> tsc_compile bufsize = %d pageno = %d", bufsize, pageno);
	
	while(buf <= buf_end)
	{
		char ch = *(buf++);
		
		if (ch == '#')
		{	// start of a scriptzz
			if (script)
			{
				script->push_back(OP_END);
				script = NULL;
			}
			
			int scriptno = ReadNumber(&buf, buf_end);
			if (scriptno >= 10000 || scriptno < 0)
			{
				staterr("tsc_compile: invalid script number: %d", scriptno);
				return false;
			}
			
			// skip the CR after the script #
			while(buf < buf_end)
			{
				if (*buf != '\r' && *buf != '\n') break;
				buf++;
			}
			
			//stat("Parsing script #%04d", scriptno);
			if (page->scripts.find(scriptno) != page->scripts.end() )
			{
				staterr("tsc_compile WARNING: duplicate script #%04d; ignoring", scriptno);
				// because script is left null, we'll ignore everything until we see another #
			}
			else
			{
				page->scripts.insert(std::pair<uint16_t, std::vector<uint8_t> >(scriptno,{}));
				script = &(page->scripts[scriptno]);
			}
		}
		else if (ch == '<' && script)
		{
			// read the command type
			cmdbuf[0] = nextchar(&buf, buf_end);
			cmdbuf[1] = nextchar(&buf, buf_end);
			cmdbuf[2] = nextchar(&buf, buf_end);
			
			int cmd = MnemonicToOpcode(cmdbuf);
			if (cmd == -1) return false;
			
			//stat("Command '%s', parameters %d", cmdbuf, cmd_table[cmd].nparams);
			script->push_back(cmd);
			
			// read all parameters expected by that command
			int nparams = cmd_table[cmd].nparams;
			for(int i=0;i<nparams;i++)
			{
				int val = ReadNumber(&buf, buf_end);
				
				script->push_back(val >> 8);
				script->push_back(val & 0xff);
				
				// colon between params
				if (i < (nparams - 1))
					buf++;
			}
		}
		else if (script)
		{	// text for message boxes
			buf--;
			script->push_back(OP_TEXT);
			ReadText(script, &buf, buf_end);
		}
		
	}
	
	if (script)
		script->push_back(OP_END);
	
	return true;
}


/*
void c------------------------------() {}
*/

void TSC::RunScripts(void)
{
	if (_curscript.running)
		ExecScript(&_curscript);
}

void TSC::StopScripts(void)
{
	if (_curscript.running)
		StopScript(&_curscript);
}

int TSC::GetCurrentScript(void)
{
	if (_curscript.running)
		return _curscript.scriptno;
	
	return -1;
}

ScriptInstance *TSC::GetCurrentScriptInstance()
{
	if (_curscript.running)
		return &_curscript;
	
	return NULL;
}

/*
void c------------------------------() {}
*/

// returns a pointer to the executable data/bytecode of the given script.
// handles looking on head, etc.
const uint8_t *TSC::FindScriptData(int scriptno, ScriptPages pageno, ScriptPages *page_out)
{
    ScriptPage *page = &_script_pages[(int)pageno];

    stat("Looking for script #%04d in %d (%d)", scriptno, (int)pageno, page->scripts.size());

    if (page->scripts.find(scriptno) == page->scripts.end())
	{
		if (pageno != ScriptPages::SP_HEAD)
		{	// try to find the script in head.tsc
			stat("Looking for script #%04d in head", scriptno);
			return FindScriptData(scriptno, ScriptPages::SP_HEAD, page_out);
		}
		else
		{
			return NULL;
		}
	}

	
	if (page_out) *page_out = pageno;
	return &(page->scripts.find(scriptno)->second.front());
}


bool TSC::StartScript(int scriptno, ScriptPages pageno)
{
const uint8_t *program;
ScriptPages found_pageno;

	program = FindScriptData(scriptno, pageno, &found_pageno);
	if (!program)
	{
		staterr("StartScript: no script at position #%04d page %d!", scriptno, pageno);
		return false;
	}
	
	// don't start regular map scripts (e.g. hvtrigger) if player is dead
	if (player->dead && found_pageno != ScriptPages::SP_HEAD)
	{
		stat("Not starting script %d; player is dead", scriptno);
		return false;
	}
	
	// set the script
	memset(&_curscript, 0, sizeof(ScriptInstance));
	
	_curscript.program = program;
	_curscript.scriptno = scriptno;
	_curscript.pageno = (int)found_pageno;
	
	_curscript.ynj_jump = -1;
	_curscript.running = true;
	
	textbox.ResetState();
	stat("  - Started script %04d", scriptno);
	
	RunScripts();
	return true;
}

void TSC::StopScript(ScriptInstance *s)
{
	if (!s->running)
		return;
	
	s->running = false;
	stat("  - Stopped script %04d", s->scriptno);
	
	// TRA is really supposed to be a jump, not a script restart--
	// in that in maintains KEY/PRI across the stage transition.
	// Emulate this by leaving the script state alone until the
	// on-entry script starts.
	player->inputs_locked = false;
	game.frozen = false;
	player->lookaway = false;
	
	textbox.ResetState();
}

/*
void c------------------------------() {}
*/

bool TSC::JumpScript(int newscriptno, ScriptPages pageno)
{
ScriptInstance *s = &_curscript;

	if (pageno == ScriptPages::SP_NULL)
		pageno = (ScriptPages)s->pageno;
	
	stat("JumpScript: moving to script #%04d page %d", newscriptno, pageno);
	
	s->program = FindScriptData(newscriptno, pageno, &pageno);
	s->pageno = (int)pageno;
	s->scriptno = newscriptno;
	s->ip = 0;
	
	if (!s->program)
	{
		staterr("JumpScript: missing script #%04d! Script terminated.", newscriptno);
		StopScript(s);
		return 1;
	}
	
	s->delaytimer = 0;
	s->waitforkey = false;
	s->wait_standing = false;
	
	// <EVE doesn't clear textbox mode or the face etc
	if (textbox.IsVisible())
	{
		textbox.ClearText();
		
		// see entrance to Sacred Grounds when you have the Nikumaru Counter
		// to witness that EVE clears TUR.
		textbox.SetFlags(TB_LINE_AT_ONCE, false);
		textbox.SetFlags(TB_VARIABLE_WIDTH_CHARS, false);
		textbox.SetFlags(TB_CURSOR_NEVER_SHOWN, false);
	}
	
	return 0;
}

void TSC::ExecScript(ScriptInstance *s)
{
char debugbuffer[512];
int cmd;
int val;
int parm[6] = {0,0,0,0,0,0};
int i;
Object *o;
const char *mnemonic;
char *str;
int cmdip;

	#define JUMP_IF(cond) \
	{	\
		if (cond)	\
		{	\
			if (JumpScript(parm[1])) return;	\
		}	\
	}
	
	// pause script while FAI/FAO still working
	if (fade.getstate() == FS_FADING) return;
	if (game.mode == GM_ISLAND) return;
	
	// waiting for an answer from a Yes/No prompt?
	if (s->ynj_jump != -1)
	{
		if (textbox.YesNoPrompt.ResultReady())
		{
			if (textbox.YesNoPrompt.GetResult() == NO)
				JumpScript(s->ynj_jump);
			
			textbox.YesNoPrompt.SetVisible(false);
			s->ynj_jump = -1;
		}
		else
		{	// pause script until answer is receieved
			return;
		}
	}
	
	// pause script while text is still displaying
	if (textbox.IsBusy()) return;
	
	// pause while NOD is in effect
	if (s->waitforkey)
	{
		if (s->nod_delay)	// used to pause during <QUA without freezing textboxes in Hell
		{
			s->nod_delay--;
		}
		else
		{
			// if key was just pressed release nod.
			// check them separately to allow holding X while
			// tapping Z to keep text scrolling fast.
			if ((inputs[JUMPKEY] && !s->lastjump) || \
				(inputs[FIREKEY] && !s->lastfire))
			{
				// hide the fact that the key was just pushed
				// so player doesn't jump/fire stupidly when dismissing textboxes
				lastinputs[JUMPKEY] |= inputs[JUMPKEY];
				lastinputs[FIREKEY] |= inputs[FIREKEY];
				lastpinputs[JUMPKEY] |= inputs[JUMPKEY];
				lastpinputs[FIREKEY] |= inputs[FIREKEY];
				
				s->waitforkey = false;
				textbox.ShowCursor(false);
			}
			
			s->lastjump = inputs[JUMPKEY];
			s->lastfire = inputs[FIREKEY];
		}
		
		// if still on return
		if (s->waitforkey) return;
	}
	
	// pause scripts while WAI is in effect.
	// <WAI9999, used in inventory/stage-select screen, means forever.
	if (s->delaytimer)
	{
		if (s->delaytimer == 9999)
		{
			UnlockInventoryInput();
		}
		else
		{
			s->delaytimer--;
		}
		
		return;
	}
	
	// pause while WAS (wait until standing) is in effect.
	if (s->wait_standing)
	{
		if (!player->blockd) return;
		s->wait_standing = false;
	}
	
	//stat("<> Entering script execution loop at ip = %d", s->ip);
	
	// main execution loop
	for(;;)
	{
		char debugbuffer2[256];
		cmdip = s->ip++;
		cmd = s->program[cmdip];
		if (cmd<OP_COUNT)
			mnemonic = cmd_table[cmd].mnemonic;
		else
			mnemonic = "???";
		
		if (cmd != OP_TEXT)
		{
			snprintf(debugbuffer2, sizeof(debugbuffer2), "%04x <%s ", cmd, mnemonic);
			for(i=0;i<cmd_table[cmd].nparams;i++)
			{
				val = ((int)s->program[s->ip++]) << 8;
				val |= s->program[s->ip++];
				parm[i] = val;
				snprintf(debugbuffer, sizeof(debugbuffer), "%s %04d", debugbuffer2, val);
			}
		}
		#ifdef TRACE_SCRIPT
		else
		{
			crtoslashn((char *)&s->program[s->ip], debugbuffer2);
			snprintf(debugbuffer, sizeof(debugbuffer), "TEXT  '%s'", debugbuffer2);
		}
		
		if (cmd == OP_TEXT && !textbox.IsVisible() && !strcmp(debugbuffer, "TEXT  '\n'")) { }
		else
		{
			stat("%04d:%d  %s", s->scriptno, cmdip, debugbuffer);
		}
		#endif
		
		switch(cmd)
		{
			case OP_END: StopScript(s); return;
			
			case OP_FAI: fade.Start(FADE_IN, parm[0], SPR_FADE_DIAMOND); return;
			case OP_FAO: fade.Start(FADE_OUT, parm[0], SPR_FADE_DIAMOND); return;
			case OP_FLA: flashscreen.Start(); break;
			
			case OP_SOU: sound(parm[0]); break;
			case OP_CMU: music(parm[0]); break;
			case OP_RMU: music(music_lastsong(), true); break;
			case OP_FMU: music_fade(); break;
			
			case OP_SSS: StartStreamSound(parm[0]); break;
			case OP_SPS: StartPropSound(); break;
			
			case OP_CSS:	// these seem identical-- either one will
			case OP_CPS:	// in fact stop the other.
			{
				StopLoopSounds();
			}
			break;
			
			// free menu selector in Inventory. It also undoes <PRI,
			// as can be seen at the entrance to Sacred Ground.
			case OP_FRE:
			{
				game.frozen = false;
				player->inputs_locked = false;
				UnlockInventoryInput();
			}
			break;
			
			case OP_PRI:	// freeze entire game (players + NPCs)
			{
				game.frozen = true;
				player->inputs_locked = false;
				statusbar.xpflashcount = 0;			// looks odd if this happens after a long <PRI, even though it's technically correct
			}
			break;
			
			case OP_KEY:	// lock players input but NPC/objects still run
			{
				game.frozen = false;
				player->inputs_locked = true;
			}
			break;
			
			case OP_MOV:
				player->x = (parm[0] * TILE_W) * CSFI;
				player->y = (parm[1] * TILE_H) * CSFI;
				player->xinertia = player->yinertia = 0;
				player->lookaway = false;
			break;
			
			case OP_UNI:
				player->movementmode = parm[0];
				map_scroll_lock(parm[0]);		// locks on anything other than 0
			break;
			
			case OP_MNA:		// show map name (as used on entry)
				map_show_map_name();
			break;
			
			case OP_MLP:		// bring up Map System
				game.setmode(GM_MAP_SYSTEM, game.mode);
			break;
			
			case OP_TRA:
			{
				bool waslocked = (player->inputs_locked || game.frozen);
				
				stat("******* Executing <TRA to stage %d", parm[0]);
				game.switchstage.mapno = parm[0];
				game.switchstage.eventonentry = parm[1];
				game.switchstage.playerx = parm[2];
				game.switchstage.playery = parm[3];
				StopScript(s);
				
				if (game.switchstage.mapno != 0)
				{
					// KEY is maintained across TRA as if the TRA
					// were a jump instead of a restart; but if the
					// game is in PRI then it is downgraded to a KEY.
					// See entrance to Yamashita Farm.
					if (waslocked)
					{
						player->inputs_locked = true;
						game.frozen = false;
					}
				}
				
				return;
			}
			break;
			
			case OP_AMPLUS: GetWeapon(parm[0], parm[1]); _lastammoinc = parm[1]; break;
			case OP_AMMINUS: LoseWeapon(parm[0]); break;
			case OP_TAM: TradeWeapon(parm[0], parm[1], parm[2]); break;
			case OP_AMJ: JUMP_IF(player->weapons[parm[0]].hasWeapon); break;
			
			case OP_ZAM:	// drop all weapons to level 1
			{
				for(int i=0;i<WPN_COUNT;i++)
				{
					player->weapons[i].xp = 0;
					player->weapons[i].level = 0;
				}
			}
			break;
			
			case OP_EVE: JumpScript(parm[0]); break;		// unconditional jump to event
			
			case OP_FLPLUS: game.flags[parm[0]] = 1; break;
			case OP_FLMINUS: game.flags[parm[0]] = 0; break;
			case OP_FLJ: JUMP_IF(game.flags[parm[0]]); break;
			
			case OP_ITPLUS: AddInventory(parm[0]); break;
			case OP_ITMINUS: DelInventory(parm[0]); break;
			case OP_ITJ: JUMP_IF((FindInventory(parm[0]) != -1)); break;
			
			// the PSelectSprite is a hack so when the Mimiga Mask is taken
			// it disappears immediately even though the game is in <PRI.
			case OP_EQPLUS:	 player->equipmask |= parm[0]; PSelectSprite(); break;
			case OP_EQMINUS: player->equipmask &= ~parm[0]; PSelectSprite(); break;
			
			case OP_SKPLUS: game.skipflags[parm[0]] = 1; break;
			case OP_SKMINUS: game.skipflags[parm[0]] = 0; break;
			case OP_SKJ: JUMP_IF(game.skipflags[parm[0]]); break;
			
			case OP_PSPLUS: textbox.StageSelect.SetSlot(parm[0], parm[1]); break;
			
			case OP_NCJ:
				JUMP_IF(CountObjectsOfType(parm[0]) > 0);
			break;
			
			case OP_ECJ:	// unused but valid
				JUMP_IF(FindObjectByID2(parm[0]));
			break;
			
			// life capsule--add to max life
			case OP_MLPLUS:
				player->maxHealth += parm[0];
				player->hp = player->maxHealth;
			break;
			
			case OP_FON:	// focus on NPC
			{
				if ((o = FindObjectByID2(parm[0])))
				{
					map_focus(o, parm[1]);
				}
			}
			break;
			case OP_FOB:	// focus on boss
			{
				if (game.stageboss.object)
					map_focus(game.stageboss.object, parm[1]);
				else
					staterr("tsc: <FOB without stage boss");
			}
			break;
			case OP_FOM:	// focus back to player (mychar)
			{
				map_focus(NULL, parm[0]);
			}
			break;
			
			case OP_DNA:	// delete all objects of type parm1
			{
				Object *o = firstobject;
				while(o)
				{
					if (o->type == parm[0]) o->Delete();
					o = o->next;
				}
			}
			break;
			
			case OP_ANP: _NPCDo(parm[0], parm[1], parm[2], _DoANP); break;
			case OP_CNP: _NPCDo(parm[0], parm[1], parm[2], _DoCNP); break;
			case OP_DNP: _NPCDo(parm[0], parm[1], parm[2], _DoDNP); break;
			
			case OP_MNP:	// move object X to (Y,Z) with direction W
				if ((o = FindObjectByID2(parm[0])))
				{
					_SetCSDir(o, parm[3]);
					o->x = (parm[1] * TILE_W) * CSFI;
					o->y = (parm[2] * TILE_H) * CSFI;
				}
			break;
			
			case OP_BOA:	// set boss state
			{
				game.stageboss.SetState(parm[0]);
			}
			break;
			case OP_BSL:	// bring up boss bar
			{
				Object *target;
				if (parm[0] == 0)
				{	// <BSL0000 means the stage boss
					target = game.stageboss.object;
					if (!game.stageboss.object)
						staterr("<BSL0000 but no stage boss present");
				}
				else
				{
					target = FindObjectByID2(parm[0]);
				}
				
				if (target)
				{
					game.bossbar.object = target;
					game.bossbar.defeated = false;
					game.bossbar.starting_hp = target->hp;
					game.bossbar.bar.displayed_value = target->hp;
				}
				else
				{
					staterr("Target of <BSL not found");
				}
			}
			break;
			
			case OP_MM0: player->xinertia = 0; break;
			case OP_MYD: _SetPDir(parm[0]); break;
			case OP_MYB:
			{
				player->lookaway = 0;
				player->yinertia = -0x200;
				// nudge a little more in plantation
				if (game.curmap == 56 && GetCurrentScript()==480)
					player->yinertia = -0x400;
				int dir = parm[0];
				
				if (dir >= 10)			// bump away from the object in parm
				{
					o = FindObjectByID2(dir);
					if (o)
					{
						if (player->CenterX() > o->CenterX())
							dir = 0;
						else
							dir = 2;
					}
				}
				
				if (dir == 0)
				{
					player->dir = LEFT;
					player->xinertia = 0x200;
				}
				else if (dir == 2)
				{
					player->dir = RIGHT;
					player->xinertia = -0x200;
				}
			}
			break;
			
			case OP_WAI: s->delaytimer = parm[0]; return;
			case OP_WAS: s->wait_standing = true; return;	// wait until player has blockd
			
			case OP_SMP: map.tiles[parm[0]][parm[1]]--; break;
			
			case OP_CMP:	// change map tile at x:y to z and create smoke
			{
				int x = parm[0];
				int y = parm[1];
				map.tiles[x][y] = parm[2];
				
				// get smoke coords
				x = ((x * TILE_W) + (TILE_W / 2)) * CSFI;
				y = ((y * TILE_H) + (TILE_H / 2)) * CSFI;
				// when tiles are CMP'd during a PRI the smoke is not visible
				// until the game is released, so I came up with this scheme
				// to make that happen. See the "you see a button" destroyable
				// box on the 2nd level of Maze M.
				if (game.frozen)
				{
					o = CreateObject(x, y, OBJ_SMOKE_DROPPER);
					o->timer2 = 4;	// amount of smoke
				}
				else
				{
					SmokeXY(x, y, 4, TILE_W/2, TILE_H/2);
				}
			}
			break;
			
			case OP_QUA:
			{
			    s->nod_delay = parm[0];
			    quake(parm[0],0);
			}
			break;
			
			case OP_LIPLUS: AddHealth(parm[0]); break;
			case OP_AEPLUS: RefillAllAmmo(); break;		// refills missiles
			
			case OP_INI: game.switchstage.mapno = NEW_GAME; break;		// restart game from beginning
			case OP_STC: niku_save(game.counter); break;
			
			case OP_SVP:
			{
				if (!settings->multisave)
				{
					game_save(settings->last_save_slot);
				}
				else
				{
					textbox.SaveSelect.SetVisible(true, SS_SAVING);
					s->delaytimer = 9999;
					return;
				}
			}
			break;
			case OP_LDP:
				game.switchstage.mapno = LOAD_GAME;
			break;
			
			case OP_HMC: player->hide = true; break;
			case OP_SMC: player->hide = false; break;
			
			// ---------------------------------------
			
			case OP_MSG:		// bring up text box
			{
				// required for post-Ballos cutscene
				textbox.SetFlags(TUR_PARAMS, false);
				textbox.SetVisible(true, TB_DEFAULTS);
				textbox.SetCanSpeedUp(true);
			}
			break;
			
			case OP_MS2:		// bring up text box, at top, with no border
			{
				textbox.SetFace(0);		// required for Undead Core intro
				textbox.SetFlags(TUR_PARAMS, false);
				textbox.SetVisible(true, TB_DRAW_AT_TOP | TB_NO_BORDER);
				textbox.SetCanSpeedUp(true);
			}
			break;
			
			case OP_MS3:		// bring up text box, at top
			{
				textbox.SetFlags(TUR_PARAMS, false);
				textbox.SetVisible(true, TB_DRAW_AT_TOP);
				textbox.SetCanSpeedUp(true);
			}
			break;
			
			case OP_CLO:		// dismiss text box.
				textbox.SetVisible(false);
				textbox.ClearText();
				// ...don't ResetState(), or it'll clear <FAC during Momorin dialog (Hideout)
			break;
			
			case OP_TEXT:		// text to be displayed
			{
				str = (char *)&s->program[s->ip];
				s->ip += (strlen(str) + 1);
				
				textbox.AddText(str);
				
				// must yield execution, because the message is busy now.
				// however, if the message contains only CR's, then we don't yield,
				// because CR's take no time to display.
				if (contains_non_cr(str))
				{
					//stat("<> Pausing script execution to display message.");
					return;
				}
				/*else
				{
					stat("<> Message is only CR's, continuing script...");
				}*/
			}
			break;
			
			case OP_CLR:		// erase all text in box
				textbox.ClearText();
			break;
			
			case OP_FAC:		// set and slide in given character face
				textbox.SetFace(parm[0]);
			break;
			
			case OP_NOD:		// pause till user presses key
			{
				if (textbox.IsVisible())
				{
					s->waitforkey = true;	// pause exec till key pressed
					// don't release immediately if keys already down
					s->lastjump = true;
					s->lastfire = true;
					
					textbox.ShowCursor(true);
				}
			}
			return;
			
			case OP_YNJ:		// prompt Yes or No and jump to given script if No
			{
				textbox.YesNoPrompt.SetVisible(true);
				s->ynj_jump = parm[0];
				
				return;
			}
			break;
			
			case OP_SAT:		// disables typing animation
			case OP_CAT:		// unused synonym
			{
				textbox.SetFlags(TB_LINE_AT_ONCE | TB_CURSOR_NEVER_SHOWN, true);
			}
			break;
			
			case OP_TUR:		// set text mode to that used for signs
			{
				textbox.SetFlags(TUR_PARAMS, true);
				textbox.SetCanSpeedUp(false);
			}
			break;
			
			case OP_GIT:		// show item graphic
			{
				if (parm[0] != 0)
				{
					int sprite, frame;
					
					if (parm[0] >= 1000)
					{	// an item
						sprite = SPR_ITEMIMAGE;
						frame = (parm[0] - 1000);
					}
					else
					{	// a weapon
						sprite = SPR_ARMSICONS;
						frame = parm[0];
					}
					
					textbox.ItemImage.SetSprite(sprite, frame);
					textbox.ItemImage.SetVisible(true);
				}
				else
				{
					textbox.ItemImage.SetVisible(false);
				}
			}
			break;
			
			case OP_NUM:
			{	// seems to show the last value that was used with "AM+"
				char buf[16];
				sprintf(buf, "%d", _lastammoinc);
				
				textbox.AddText(buf);
			}
			break;
			
			case OP_SLP:	// bring up teleporter menu
			{
				textbox.StageSelect.SetVisible(true);
				return;
			}
			break;
			
			case OP_ESC:
			{
				StopScript(s);
				game.reset();
			}
			break;
			
			// ---------------------------------------
			
			// trigger island-falling cinematic
			// if the parameter is 0, the island crashes (good ending);
			// if the parameter is 1, the island survives (best ending)
			case OP_XX1:
			{
				game.setmode(GM_ISLAND, parm[0]);
				return;
			}
			break;
			
			case OP_CRE:
			{
				game.setmode(GM_CREDITS);
				return;
			}
			break;
			
			case OP_SIL:
				credit_set_image(parm[0]);
			break;
			
			case OP_CIL:
				credit_clear_image();
			break;
			
			case OP_ACH:
			break;
			
			default:
			{
				if (cmd < OP_COUNT)
					console.Print("- unimplemented opcode %s; script %04d halted.", cmd_table[cmd].mnemonic, s->scriptno);
				else
					console.Print("- unimplemented opcode %02x; script %04d halted.", cmd, s->scriptno);
				
				StopScript(s);
				return;
			}
		}
	}
}

std::string _DescribeCSDir(int csdir)
{
	switch(csdir)
	{
		case 0: return "LEFT";
		case 1: return "UP";
		case 2: return "RIGHT";
		case 3: return "DOWN";
		case 4: return "FACE_PLAYER";
		case 5: return "NO_CHANGE";
		default: return "Invalid CS Dir"+std::to_string(csdir);
	}
}

// converts from a CS direction (0123 = left,up,right,down)
// into a NXEngine direction (0123 = right,left,up,down),
// and applies the converted direction to the object.
void _SetCSDir(Object *o, int csdir)
{
	if (csdir < 4)
	{
		o->dir = CVTDir(csdir);
	}
	else if (csdir == 4)
	{	// face towards player
		o->dir = (o->x >= player->x) ? LEFT : RIGHT;
	}
	else if (csdir == 5)
	{	// no-change, used with e.g. ANP
	}
	else
	{
		staterr("SetCSDir: warning: invalid direction %04d passed as dirparam only", csdir);
	}
	
	// a few late-game objects, such as statues in the statue room,
	// use ANP/CNP's direction parameter as an extra generic parameter
	// to the object. I didn't feel it was safe to set a dir of say 200
	// in our engine as it may cause crashes somewhere if the sprite was
	// ever tried to be drawn using that dir. There's also the complication
	// that we're about to munge the requested values since our direction
	// constants don't have the same numerical values as CS's engine.
	// So is dirparam holds the raw value of the last dir that a script
	// tried to set.
	o->dirparam = csdir;
}

void _SetPDir(int d)
{
	if (d == 3)
	{	// look away
		player->lookaway = 1;
	}
	else
	{
		player->lookaway = 0;
		
		if (d < 10)
		{	// set direction - left/right/up/down
			_SetCSDir(player, d);
		}
		else
		{	// face the object in parm
			Object *o;
			
			if ((o = FindObjectByID2(d)))
			{
				player->dir = (player->x > o->x) ? LEFT:RIGHT;
			}
		}
	}
	
	player->xinertia = 0;
	PSelectFrame();
}

/*
void c------------------------------() {}
*/

// call action_function on all NPCs with id2 matching "id2".
void TSC::_NPCDo(int id2, int p1, int p2, void (*action_function)(Object *o, int p1, int p2))
{
	// make a list first, as during <CNP, changing the
	// object type may call BringToFront and break stuff
	// if there are multiple hits.
	Object *hits[MAX_OBJECTS], *o;
	int numhits = 0;
	
	FOREACH_OBJECT(o)
	{
		if (o->id2 == id2 && o != player)
		{
			if (numhits < MAX_OBJECTS)
				hits[numhits++] = o;
		}
	}
	
	for(int i=0;i<numhits;i++)
		(*action_function)(hits[i], p1, p2);
	
}

void TSC::_DoANP(Object *o, int p1, int p2)		// ANIMATE (set) object's state to p1 and set dir to p2
{
	#ifdef TRACE_SCRIPT
		stat("ANP: Obj %08x (%s): setting state: %d and dir: %s", \
			o, DescribeObjectType(o->type), p1, _DescribeCSDir(p2).c_str());
	#endif
	
	o->state = p1;
	_SetCSDir(o, p2);
}

void TSC::_DoCNP(Object *o, int p1, int p2)		// CHANGE object to p1 and set dir to p2
{
	#ifdef TRACE_SCRIPT
		stat("CNP: Obj %08x changing from %s to %s, new dir = %s",
			o, DescribeObjectType(o->type), DescribeObjectType(p1), _DescribeCSDir(p2).c_str());
	#endif
	
	// Must set direction BEFORE changing type, so that the Carried Puppy object
	// gets priority over the direction to use while the game is <PRI'd.
	_SetCSDir(o, p2);
	o->ChangeType(p1);
}

void TSC::_DoDNP(Object *o, int p1, int p2)		// DELETE object
{
	#ifdef TRACE_SCRIPT
		stat("DNP: %08x (%s) deleted", o, DescribeObjectType(o->type));
	#endif
	
	o->Delete();
}




