#include "ckptinfo.h"
#include<vector>
using namespace std;

void compress_map(unsigned char *map1, unsigned char *map2, int size)
{
    int len = size>>3;
    for(int i=0; i<len; i++){
        unsigned data = 0;
        for(int j=7;j>=0;j--){
            data = data << 1;
            data += (map1[i*8+j]);
        }
        map2[i] = data;
    }
}

uint64_t getnum(uint8_t *cmap, int size)
{
    int num = 0;
    for(int i=0;i<size;i++) {
        uint8_t dmap = cmap[i];
        for(int m=0; m<8; m++) {
            if(dmap%2==1) {
                num++;
            }
            dmap = dmap >> 1;
        }
    }
    return num;
}

/* detailed design
    addr, size, map
    data*n
    addr, size, map
    data*n
    addr, size, map
    data*n
*/
//compress first information with second methods
void processFirstLoad1(LoadInfo *linfos, uint64_t loadnum, uint64_t maxsize, FILE *q)
{
    uint8_t map1[4096];
    uint8_t map2[4096];
    uint8_t pagedata[4096];
    uint64_t baseaddr, endaddr = 0;
    uint64_t d1 = 0, d2 = 0, a1 = 0, a2 = 0, offset;
    int j=0;
    uint64_t loadnum1 = 0, num = 0;

    maxsize = 4096;
    for(uint64_t i=0;i<loadnum;i++){
        baseaddr = linfos[i].addr;
        endaddr = baseaddr + maxsize;

        for(j=i+1; j<loadnum; j++){
            if(linfos[j].addr+8 > endaddr)
                break;
        }

        memset(map1, 0, maxsize/8);
        memset(map2, 0, maxsize/64);
        memset(pagedata, 0, 4096);
        
        for(int c=i; c<j; c++){
            offset = linfos[c].addr - linfos[i].addr;
            *(uint64_t *)(&pagedata[offset]) = linfos[c].data;
            a1 = offset % 8;
            offset = (offset - a1) >> 3;
            d1 = *((uint64_t *)(&pagedata[offset*8]));
            map1[offset] = (d1 == 0) ? 0: 1;

            if(a1 != 0){
                d1 = *((uint64_t *)(&pagedata[offset*8+8]));
                map1[offset+1] = (d1 == 0) ? 0: 1;
            }
        }

        compress_map(map1, map2, maxsize/8);
        fwrite(&linfos[i].addr, 8, 1, q);
        fwrite(&maxsize, 8, 1, q);
        fwrite(map2, 1, maxsize/64, q);
        uint64_t tnum = getnum(map2, maxsize/64);
        for(int c=0; c<maxsize/8; c++){
            if(map1[c]==1){
                d1 = *((uint64_t *)(&pagedata[c*8]));
                fwrite(&d1, 8, 1, q);
                loadnum1++;
            }
        }
        i=j-1;
    }
    //over flag
    a1 = 0;
    fwrite(&a1, 8, 1, q);
}


//read ckpt and restore first loads
void recoveryFirstLoad1(FILE *p)
{
    uint64_t addr=0, size = 0;
    uint8_t cmap[1024];
    uint64_t mapsize = 0;
    uint8_t dmap, taddr=0;
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


