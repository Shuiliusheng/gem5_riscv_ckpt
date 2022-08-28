#include "sim/ckptinfo.hh"
#include "sim/fastlz.hh"
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

#define MaxCoverSize 4096
uint8_t map1[MaxCoverSize];
uint8_t map2[MaxCoverSize];
uint8_t pagedata[MaxCoverSize];
vector<uint64_t> sdata;

void saveFLoads_comp(vector<FirstLoadInfo> &linfos, FILE *q)
{
    uint64_t endaddr = 0, d1 = 0, a1 = 0, offset=0, j=0;
    uint64_t loadnum = linfos.size();
    for(uint64_t i=0;i<loadnum;i++){
        endaddr = linfos[i].addr + MaxCoverSize;

        for(j=i+1; j<loadnum; j++){
            if(linfos[j].addr+8 > endaddr)
                break;
        }

        memset(map1, 0, MaxCoverSize/8);
        memset(map2, 0, MaxCoverSize/64);
        memset(pagedata, 0, MaxCoverSize);
        sdata.clear();

        for(int c=i; c<j; c++){
            offset = linfos[c].addr - linfos[i].addr;
            *(uint64_t *)(&pagedata[offset]) = *(uint64_t *)(&linfos[c].data[0]);
            a1 = offset % 8;
            offset = (offset - a1) >> 3;
            d1 = *((uint64_t *)(&pagedata[offset*8]));
            map1[offset] = (d1 == 0) ? 0: 1;

            if(a1 != 0){
                d1 = *((uint64_t *)(&pagedata[offset*8+8]));
                map1[offset+1] = (d1 == 0) ? 0: 1;
            }
        }

        compress_map(map1, map2, MaxCoverSize/8);
        sdata.push_back(linfos[i].addr);
        sdata.push_back(MaxCoverSize);
        for(int c=0; c<MaxCoverSize/64; c+=8){
            sdata.push_back(*((uint64_t *)(&map2[c])));
        }

        for(int c=0; c<MaxCoverSize/8; c++){
            if(map1[c]==1){
                sdata.push_back(*((uint64_t *)(&pagedata[c*8])));
            }
        }
        i=j-1;
        compress_write(sdata, q, 1, i==(loadnum-1));
    }
    //over flag
    a1 = 0;
    fwrite(&a1, 8, 1, q);
}
/*
    compress data size
    load num 
    compress data
        - page addr
        - page size
        - map data
        - data
        - page addr
        - page size
        - map data
        - data
    compress data size
    load num 
    compress data

*/


void saveFLoads(vector<FirstLoadInfo> &firstloads, FILE *q)
{
    uint64_t temp = firstloads.size();
    fwrite(&temp, sizeof(uint64_t), 1, q);
    fwrite(&firstloads[0], sizeof(FirstLoadInfo), temp, q);
}