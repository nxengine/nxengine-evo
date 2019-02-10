
#ifndef _YESNOPROMPT_H
#define _YESNOPROMPT_H

enum YNJResult
{
  NO,
  YES
};

class TB_YNJPrompt
{
public:
  void SetVisible(bool enable);
  void ResetState();

  void Draw();
  void Tick();

  bool IsVisible()
  {
    return fVisible;
  }
  bool ResultReady(); // returns true if the user has selected a result
  int GetResult();    // returns YES or NO

private:
  bool fVisible;
  struct
  {
    int y;
  } fCoords;

  int fState;
  int fTimer;

  int fAnswer;
};

#endif
