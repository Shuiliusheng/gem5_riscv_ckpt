#include "ckptinfo.h"
#include<vector>
using namespace std;

typedef struct{
    uint64_t size;
    uint64_t addr;
    vector<uint64_t> data;
}SingleInfo;

typedef struct{
    int con;
    vector<SingleInfo> infos;
}ConInfos;


/* design
    load num, data size
    addr, data
    addr, data
    load num, data size
    addr, data
    addr, data
    load num, data size
    addr, data
    addr, data
*/
//compress first information with first methods
void processFirstLoad(LoadInfo *linfos, uint64_t loadnum, uint64_t maxcon, FILE *q)
{
    uint64_t j=0;
    uint64_t pagenum = 0;
    ConInfos continues[4096+1];
    uint64_t totalsize = 0;
    
    for(uint64_t i=0;i<loadnum;i++){
        SingleInfo sinfo;
        sinfo.data.clear();
        sinfo.addr = linfos[i].addr;
        sinfo.data.push_back(linfos[i].data);
        for(j=i+1; j<loadnum; j++){
            if(linfos[j].addr != linfos[j-1].addr+8)
                break;
            sinfo.data.push_back(linfos[j].data);
        }

        int con = j-i-1;
        sinfo.size = j-i;
        if(con<maxcon){
            continues[con].con = con;
            continues[con].infos.push_back(sinfo);
        }
        else{
            continues[maxcon].infos.push_back(sinfo);
        }
        i=j-1;
    }

    int num=0;
    for(int i=0;i<maxcon+1;i++){
        if(continues[i].infos.size()==0)
            continue;
        if(i<maxcon) {
            uint64_t firstinfo=continues[i].infos.size(), temp = 0;
            firstinfo = (firstinfo << 32) + (i+1);
            fwrite(&firstinfo, 8, 1, q);
            num++;
            for(int j=0;j<continues[i].infos.size();j++){
                fwrite(&(continues[i].infos[j].addr), 8, 1, q);
                for(int c=0;c<continues[i].infos[j].data.size();c++){
                    temp = continues[i].infos[j].data[c];
                    fwrite(&temp, 8, 1, q);
                }
            }
        }
        else {
            for(int j=0;j<continues[i].infos.size();j++){
                uint64_t firstinfo=1, temp = 0;
                firstinfo = (firstinfo << 32) + (continues[i].infos[j].data.size());
                fwrite(&firstinfo, 8, 1, q);
                fwrite(&(continues[i].infos[j].addr), 8, 1, q);
                for(int c=0;c<continues[i].infos[j].data.size();c++){
                    temp = continues[i].infos[j].data[c];
                    fwrite(&temp, 8, 1, q);
                }
            }
        }
    } 

    //over flag
    uint64_t firstinfo=0;
    fwrite(&firstinfo, 8, 1, q);  
}

//read ckpt and restore first loads
void recoveryFirstLoad(FILE *p)
{
    uint64_t loadnum = 0, firstinfo, datasize, infonum, addr=0;
    while(1) {
        firstinfo=0;
        fread(&firstinfo, 8, 1, p);
        if(firstinfo==0) break;

        datasize = (firstinfo<<32)>>32; 
        infonum = firstinfo>>32;
        addr = 0;
        loadnum += datasize*infonum;
        for(int i=0;i<infonum;i++){
            fread(&addr, 8, 1, p);
            fread((char*)addr, 8, datasize, p);
        }
    }
}



