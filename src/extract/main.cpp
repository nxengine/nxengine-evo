#include <stdio.h>
#include <stdlib.h>
#include "extractpxt.h"
#include "extractstages.h"
#include "extractfiles.h"

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
	printf("Extracted. Press any key to continue.\n");
	getchar();
    return 0;
}


