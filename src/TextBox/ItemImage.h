
#ifndef _ITEMIMAGE_H
#define _ITEMIMAGE_H

class TB_ItemImage
{
public:
  void ResetState();
  void Draw();

  void SetVisible(bool enable);
  void SetSprite(int sprite, int frame);

private:
  bool fVisible;
  int fSprite, fFrame;
  int fYOffset;
};

#endif
