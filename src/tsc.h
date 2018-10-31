#ifndef _TSC_H
#define _TSC_H

#include "object.h"

#include <map>
#include <string>
#include <vector>

#define NUM_SCRIPT_PAGES 4
// TSC running script instance; there is only ever one running at once
// but I generalized it as if there might be more just for good style.
struct ScriptInstance
{
  const uint8_t *program; // compiled script code
  uint32_t ip;            // instruction pointer
  bool running;

  int scriptno; // script # of active script
  int pageno;   // ScriptPage/namespace # script is in

  int delaytimer; // time left on a <WAI delay

  // used with <NOD
  bool waitforkey;
  bool lastjump, lastfire;
  int nod_delay;

  int ynj_jump; // if != -1, a Yes/No choice is up, and this is the event # to jump to if they pick No.

  bool wait_standing; // if 1 pauses script until player touches ground
};

struct ScriptPage
{
  // a variable-length array of pointers to compiled script code
  // for each script in the page; their indexes in this array
  // correspond to their script numbers.
  std::map<uint16_t, std::vector<uint8_t>> scripts;

  void Clear()
  {
    scripts.clear();
  }
};

class TSC
{

public:
  // script "pages", like namespaces, for the different tsc files
  enum class ScriptPages : signed
  {
    SP_NULL        = -1, // head.tsc common scripts
    SP_HEAD        = 0,  // head.tsc common scripts
    SP_MAP         = 1,  // map scripts
    SP_ARMSITEM    = 2,  // inventory screen
    SP_STAGESELECT = 3   // scripts for Arthur's House teleporter
  };

  TSC();
  ~TSC();

  bool StartScript(int scriptno, ScriptPages pageno = ScriptPages::SP_MAP);
  void StopScript(ScriptInstance *s);
  bool JumpScript(int newscriptno, ScriptPages pageno = ScriptPages::SP_NULL);

  void Clear();
  bool Init(void);
  void Close(void);
  bool Load(const std::string &fname, ScriptPages pageno);
  std::string Decrypt(const std::string &fname, int *fsize_out);
  bool Compile(const char *buf, int bufsize, ScriptPages pageno);
  void RunScripts(void);
  void StopScripts(void);
  int GetCurrentScript(void);
  ScriptInstance *GetCurrentScriptInstance();
  const uint8_t *FindScriptData(int scriptno, ScriptPages pageno, ScriptPages *page_out);
  void ExecScript(ScriptInstance *s);

private:
  ScriptInstance _curscript;
  int _lastammoinc = 0;
  ScriptPage _script_pages[NUM_SCRIPT_PAGES];

  static void _NPCDo(int id2, int p1, int p2, void (*action_function)(Object *o, int p1, int p2));
  static void _DoANP(Object *o, int p1, int p2);
  static void _DoCNP(Object *o, int p1, int p2);
  static void _DoDNP(Object *o, int p1, int p2);
};

std::string _DescribeCSDir(int csdir);
void _SetCSDir(Object *o, int csdir);
void _SetPDir(int d);

// globally-accessible scripts in head.tsc
#define SCRIPT_NULL 0
#define SCRIPT_EMPTY 1             // displays a textbox that says "Empty."
#define SCRIPT_SAVE 16             // save-game script
#define SCRIPT_REFILL 17           // health refill script
#define SCRIPT_REST 19             // "Do you want to rest?"
#define SCRIPT_MISSILE_LAUNCHER 30 // spiel about missile launcher when you first get it
#define SCRIPT_DIED 40             // "You have died. Would you like to retry?"
#define SCRIPT_DROWNED 41          // "You have drowned. Would you like to retry?"
#define SCRIPT_NEVERSEENAGAIN 42   // "You were never seen again. Would you like to retry?"

#define OP_AEPLUS 0   // 0, 0,
#define OP_AMPLUS 1   // 2, 0,
#define OP_AMMINUS 2  // 1, 0,
#define OP_AMJ 3      // 2, 0,
#define OP_ANP 4      // 3, 0,
#define OP_BOA 5      // 1, 0,
#define OP_BSL 6      // 1, 0,
#define OP_CAT 7      // 0, 0,
#define OP_CIL 8      // 0, 0,
#define OP_CLO 9      // 0, 0,
#define OP_CLR 10     // 0, 0,
#define OP_CMP 11     // 3, 0,
#define OP_CMU 12     // 1, 0,
#define OP_CNP 13     // 3, 0,
#define OP_CPS 14     // 0, 0,
#define OP_CRE 15     // 0, 0,
#define OP_CSS 16     // 0, 0,
#define OP_DNA 17     // 1, 0,
#define OP_DNP 18     // 1, 0,
#define OP_ECJ 19     // 2, 0,
#define OP_END 20     // 0, 0,
#define OP_EQPLUS 21  // 1, 0,
#define OP_EQMINUS 22 // 1, 0,
#define OP_ESC 23     // 0, 0,
#define OP_EVE 24     // 1, 0,
#define OP_FAC 25     // 1, 0,
#define OP_FAI 26     // 1, 0,
#define OP_FAO 27     // 1, 0,
#define OP_FLPLUS 28  // 1, 0,
#define OP_FLMINUS 29 // 1, 0,
#define OP_FLA 30     // 0, 0,
#define OP_FLJ 31     // 2, 0,
#define OP_FMU 32     // 0, 0,
#define OP_FOB 33     // 2, 0,
#define OP_FOM 34     // 1, 0,
#define OP_FON 35     // 2, 0,
#define OP_FRE 36     // 0, 0,
#define OP_GIT 37     // 1, 0,
#define OP_HMC 38     // 0, 0,
#define OP_INI 39     // 0, 0,
#define OP_INP 40     // 3, 0,
#define OP_ITPLUS 41  // 1, 0,
#define OP_ITMINUS 42 // 1, 0,
#define OP_ITJ 43     // 2, 0,
#define OP_KEY 44     // 0, 0,
#define OP_LDP 45     // 0, 0,
#define OP_LIPLUS 46  // 1, 0,
#define OP_MLPLUS 47  // 1, 0,
#define OP_MLP 48     // 0, 0,
#define OP_MM0 49     // 0, 0,
#define OP_MNA 50     // 0, 0,
#define OP_MNP 51     // 4, 0,
#define OP_MOV 52     // 2, 0,
#define OP_MPPLUS 53  // 1, 0,
#define OP_MPJ 54     // 1, 0,
#define OP_MS2 55     // 0, 0,
#define OP_MS3 56     // 0, 0,
#define OP_MSG 57     // 0, 0,
#define OP_MYB 58     // 1, 0,
#define OP_MYD 59     // 1, 0,
#define OP_NCJ 60     // 2, 0,
#define OP_NOD 61     // 0, 0,
#define OP_NUM 62     // 1, 0,
#define OP_PRI 63     // 0, 0,
#define OP_PSPLUS 64  // 2, 0,
#define OP_QUA 65     // 1, 0,
#define OP_RMU 66     // 0, 0,
#define OP_SAT 67     // 0, 0,
#define OP_SIL 68     // 1, 0,
#define OP_SKPLUS 69  // 1, 0,
#define OP_SKMINUS 70 // 1, 0,
#define OP_SKJ 71     // 2, 0,
#define OP_SLP 72     // 0, 0,
#define OP_SMC 73     // 0, 0,
#define OP_SMP 74     // 2, 0,
#define OP_SNP 75     // 4, 0,
#define OP_SOU 76     // 1, 0,
#define OP_SPS 77     // 0, 0,
#define OP_SSS 78     // 1, 0,
#define OP_STC 79     // 0, 0,
#define OP_SVP 80     // 0, 0,
#define OP_TAM 81     // 3, 0,
#define OP_TRA 82     // 4, 0,
#define OP_TUR 83     // 0, 0,
#define OP_UNI 84     // 1, 0,
#define OP_UNJ 85     // 1, 0,
#define OP_WAI 86     // 1, 0,
#define OP_WAS 87     // 0, 0,
#define OP_XX1 88     // 1, 0,
#define OP_YNJ 89     // 1, 0,
#define OP_ZAM 90     // 0, 0
#define OP_ACH 91     // 0, 0

#define OP_COUNT 92

#define OP_TEXT 0xfa // mine, denotes start of text

#endif
