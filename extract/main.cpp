#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "extractpxt.h"
#include "extractstages.h"
#include "extractfiles.h"

const char *data_dir = "data";
const char *stage_dir = "data/Stage";
const char *pic_dir = "endpic";
const char *nxdata_dir = ".";

static const char *filename = "Doukutsu.exe";

int main(int argc, char *argv[])
{
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp)
    {
        return 1;
    }

    if (extract_pxt(fp)) return 1;
    if (extract_files(fp)) return 1;
    if (extract_stages(fp)) return 1;
    fclose(fp);
    return 0;
}


