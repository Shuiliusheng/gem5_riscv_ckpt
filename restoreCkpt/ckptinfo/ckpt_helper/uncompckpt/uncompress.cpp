#include "ckptinfo.h"
#include "fastlz.h"
#include<vector>
using namespace std;

#define MaxCompressSize 1024*1024*30
#define ExtraDataSize   1024*6
void unzipFirstLoads(FILE *p, FILE *q)
{
    uint64_t loadnum = 0;
    uint8_t *rawdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint8_t *compdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint64_t compsize, rawsize, infonum = 0;

    uint8_t *cmap;
    uint64_t mapsize = 0, addr=0, taddr=0;
    while(1) {
        compsize = 0, infonum = 0;
        fread(&compsize, 8, 1, p);
        if(compsize==0) break;

        fread(&infonum, 8, 1, p);
        fread(compdata, 1, compsize, p);
        rawsize = fastlz_decompress(compdata, compsize, rawdata, MaxCompressSize+ExtraDataSize);
        printf("rawsize: %d, compsize: %d, infonum: %d\n", rawsize, compsize, infonum);
        fwrite(&rawsize, 8, 1, q);
        fwrite(&infonum, 8, 1, q);
        fwrite(rawdata, 1, rawsize, q);
    }
    fwrite(&compsize, 8, 1, q);
    free(rawdata);
    free(compdata);
}

/*
    data size, infonum, data
    data size, infonum, data
*/