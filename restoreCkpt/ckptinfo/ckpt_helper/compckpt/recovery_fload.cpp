#include "ckptinfo.h"
#include "fastlz.h"

void recovery_nocomp(FILE *p);
void recovery_datamap(FILE *p);
void recovery_fastlz(FILE *p);


void recovery_FirstLoad(FILE *p)
{
    uint64_t compressTag = 0;
    fread(&compressTag, 8, 1, p);

    if(compressTag-0x123456 == 1){
        printf("the first load information is saved with data map\n");
        recovery_datamap(p);
    }
    else if(compressTag-0x123456 == 2 || compressTag-0x123456 == 3){
        printf("the first load information is saved with data map and compressed by fastlz\n");
        recovery_fastlz(p);
    }
    else{
        printf("the first load information is saved without compress\n");
        uint64_t place = ftell(p) - 8;
        fseek(p, place, SEEK_SET);
        recovery_nocomp(p);
    }
}

void recovery_nocomp(FILE *p)
{
    uint64_t loadnum = 0;
    fread(&loadnum, 8, 1, p);
    LoadInfo *linfos = (LoadInfo *)malloc(sizeof(LoadInfo)*loadnum);
    fread(&linfos[0], sizeof(LoadInfo), loadnum, p);
    for(int j=0;j<loadnum;j++){
        *((uint64_t *)linfos[j].addr) = linfos[j].data;
    }
    free(linfos);
}

void recovery_datamap(FILE *p)
{
    uint64_t addr=0, size = 0;
    uint8_t cmap[1024];
    uint64_t mapsize = 0, taddr=0;
    while(1) {
        memset(cmap, 0, 1024);
        addr = 0;

        fread(&addr, 8, 1, p);
        if(addr==0) break;

        fread(&size, 8, 1, p);
        mapsize = size/64;
        fread(&cmap, 1, mapsize, p);
        for(int i=0;i<mapsize;i++) {
            taddr = addr + (i*8*8);
            for(int m=0; m<8; m++) {
                if(cmap[i]%2==1) {
                    fread((uint64_t *)(taddr + m*8), 8, 1, p);
                }
                cmap[i] = cmap[i] >> 1;
            }
        }
    }
}

#define MaxCompressSize 1024*1024*30
#define ExtraDataSize   1024*6
//read ckpt and restore first loads
void recovery_fastlz(FILE *p)
{
    uint8_t *rawdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint8_t *compdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint64_t compsize, rawsize, infonum = 0, baseaddr = 0;

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
        baseaddr = (uint64_t)rawdata;
        for(int n=0; n<infonum; n++) {
            addr = *((uint64_t *)baseaddr);
            if(addr==0) break;
            baseaddr += 8;
            mapsize = *((uint64_t *)baseaddr) / 64;
            baseaddr+=8;
            cmap = (uint8_t *)baseaddr;
            baseaddr += mapsize;
            for(int i=0;i<mapsize;i++) {
                taddr = addr + (i*8*8);
                for(int m=0; m<8; m++) {
                    if(cmap[i]%2==1) {
                        *((uint64_t *)(taddr + m*8)) = *((uint64_t *)baseaddr);
                        baseaddr+=8;
                    }
                    cmap[i] = cmap[i] >> 1;
                }
            }
        }
    }
    free(rawdata);
    free(compdata);
}
