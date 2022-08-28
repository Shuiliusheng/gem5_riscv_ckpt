#include "ckptinfo.h"
#include "fastlz.h"
#include<vector>
using namespace std;

#define MaxCompressSize 1024*1024*30
#define ExtraDataSize   1024*6
uint8_t tempdata[MaxCompressSize+ExtraDataSize];
uint8_t compdata[(MaxCompressSize)*2];
uint64_t tdsize = 0, infonum=0;
void compress_write(vector<uint64_t> &data, FILE *q, int complevel, bool end)
{
    for(int i=0;i<data.size();i++) {
        *((uint64_t *)(&tempdata[tdsize])) = data[i];
        tdsize += 8;
    }
    infonum++;

    if(tdsize < MaxCompressSize && !end) 
        return ;

    uint64_t compsize = fastlz_compress_level(complevel, tempdata, tdsize, compdata);
    fwrite(&compsize, 8, 1, q);
    fwrite(&infonum, 8, 1, q);
    fwrite(&compdata[0], 1, compsize, q);
    tdsize = 0;
    infonum = 0;
}

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

/* compress with lz design
    addr, size, map
    data*n
    addr, size, map
    data*n
    addr, size, map
    data*n
*/
//compress first information with third methods
void compress_FirstLoad(LoadInfo *linfos, uint64_t loadnum, FILE *q)
{
    uint8_t map1[4096];
    uint8_t map2[4096];
    uint8_t pagedata[4096];
    uint64_t baseaddr, endaddr = 0;
    uint64_t d1 = 0, a1 = 0, offset=0,  maxsize = 4096;
    uint64_t loadnum1 = 0, num = 0, j=0;
    vector<uint64_t> sdata;
   
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

        sdata.clear();
        sdata.push_back(linfos[i].addr);
        sdata.push_back(maxsize);
        for(int c=0; c<maxsize/64; c+=8){
            sdata.push_back(*((uint64_t *)(&map2[c])));
        }

        for(int c=0; c<maxsize/8; c++){
            if(map1[c]==1){
                d1 = *((uint64_t *)(&pagedata[c*8]));
                sdata.push_back(d1);
                loadnum1++;
            }
        }
        i=j-1;
        compress_write(sdata, q, 1, i==(loadnum-1));
    }
    //over flag
    a1 = 0;
    fwrite(&a1, 8, 1, q);
}


//read ckpt and restore first loads
void recovery_FirstLoad(FILE *p)
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

        baseaddr = 0;
        for(int n=0; n<infonum; n++) {
            memset(cmap, 0, 1024);
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
