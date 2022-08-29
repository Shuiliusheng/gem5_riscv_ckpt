#include "ckptinfo.h"
#include "fastlz.h"
#include <vector>
using namespace std;

void read_nocomp(FILE *p, vector<LoadInfo> &tempinfos);
void read_datamap(FILE *p, vector<LoadInfo> &tempinfos);
void read_fastlz(FILE *p, vector<LoadInfo> &tempinfos);



void read_FirstLoad(FILE *p, vector<LoadInfo> &tempinfos)
{
    uint64_t compressTag = 0;
    fread(&compressTag, 8, 1, p);

    if(compressTag-0x123456 == 1){
        printf("the first load information is saved with data map\n");
        read_datamap(p, tempinfos);
    }
    else if(compressTag-0x123456 == 2 || compressTag-0x123456 == 3){
        printf("the first load information is saved with data map and compressed by fastlz\n");
        read_fastlz(p, tempinfos);
    }
    else{
        printf("the first load information is saved without compress\n");
        uint64_t place = ftell(p) - 8;
        fseek(p, place, SEEK_SET);
        read_nocomp(p, tempinfos);
    }
    printf("loadnum: %d\n", tempinfos.size());
}

void read_nocomp(FILE *p, vector<LoadInfo> &tempinfos)
{
    uint64_t loadnum = 0;
    fread(&loadnum, 8, 1, p);
    LoadInfo linfo;;
    for(int i=0;i<loadnum;i++){
        fread(&linfo, sizeof(LoadInfo), 1, p);
        tempinfos.push_back(linfo);
    }
}

void read_datamap(FILE *p, vector<LoadInfo> &tempinfos)
{
    uint64_t addr=0, size = 0;
    uint8_t cmap[1024];
    uint64_t mapsize = 0, taddr=0;
    LoadInfo info;
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
                    fread((uint64_t *)(info.data), 8, 1, p);
                    info.addr = taddr + m*8;
                    tempinfos.push_back(info);
                }
                cmap[i] = cmap[i] >> 1;
            }
        }
    }
}

#define MaxCompressSize 1024*1024*30
#define ExtraDataSize   1024*6
//read ckpt and restore first loads
void read_fastlz(FILE *p, vector<LoadInfo> &tempinfos)
{
    uint8_t *rawdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint8_t *compdata = (uint8_t *)malloc(MaxCompressSize+ExtraDataSize);
    uint64_t compsize, rawsize, infonum = 0, baseaddr = 0;

    uint8_t *cmap;
    uint64_t mapsize = 0, addr=0, taddr=0;
    LoadInfo info;
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
                        info.addr = taddr + m*8;
                        info.data = *((uint64_t *)baseaddr);
                        tempinfos.push_back(info);
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
