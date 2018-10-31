#include "extractfiles.h"
#include "extractpxt.h"
#include "extractstages.h"

#include <cstdio>
#include <cstdlib>

static const char *filename = "Doukutsu.exe";

int main(int argc, char *argv[])
{
  FILE *fp;

  fp = fopen(filename, "rb");
  if (!fp)
  {
    return 1;
  }

  if (extract_pxt(fp))
    return 1;
  if (extract_files(fp))
    return 1;
  if (extract_stages(fp))
    return 1;
  fclose(fp);
  printf("Sucessfully extracted.\n");
  return 0;
}
