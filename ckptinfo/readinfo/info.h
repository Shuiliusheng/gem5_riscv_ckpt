#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

typedef struct {
  uint8_t  e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} Elf64_Phdr;

typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct{
    uint64_t addr;
    uint64_t size;
}MemRangeInfo;

typedef struct{
    uint64_t addr;
    uint64_t size;
    uint64_t data;
}LoadInfo;

typedef struct{
    uint64_t pc;
    uint64_t num;
    uint64_t p0, p1, p2;
    uint64_t hasret, ret;
    uint64_t bufaddr, data_offset, data_size;
}SyscallInfo;

typedef struct{
    uint64_t syscall_info_addr;
    uint64_t nowcallnum;
    uint64_t totalcallnum;
    uint64_t intregs[32];
    uint64_t fpregs[32];
    uint64_t oldregs[32];
}RunningInfo;

#define PF_X 1
#define PF_W 2
#define PF_R 4
#define RunningInfoAddr 0x150000
#define StoreIntRegAddr "0x150018"
#define OldIntRegAddr   "0x150218"

MemRangeInfo data_seg, text_seg;

#define Save_int_regs(BaseAddr) asm volatile( \
    "sub x0, a0, x7 #save a0 to rtemp7 \n\t"   \
    "fence \n\t"   \
    "li a0, " BaseAddr " \n\t"   \
    "sd x1,8*1(a0)  \n\t"   \
    "sd x2,8*2(a0)  \n\t"   \
    "sd x3,8*3(a0)  \n\t"   \
    "sd x4,8*4(a0)  \n\t"   \
    "sd x5,8*5(a0)  \n\t"   \
    "sd x6,8*6(a0)  \n\t"   \
    "sd x7,8*7(a0)  \n\t"   \
    "sd x8,8*8(a0)  \n\t"   \
    "sd x9,8*9(a0)  \n\t"   \
    "sd x10,8*10(a0)  \n\t"   \
    "sd x11,8*11(a0)  \n\t"   \
    "sd x12,8*12(a0)  \n\t"   \
    "sd x13,8*13(a0)  \n\t"   \
    "sd x14,8*14(a0)  \n\t"   \
    "sd x15,8*15(a0)  \n\t"   \
    "sd x16,8*16(a0)  \n\t"   \
    "sd x17,8*17(a0)  \n\t"   \
    "sd x18,8*18(a0)  \n\t"   \
    "sd x19,8*19(a0)  \n\t"   \
    "sd x20,8*20(a0)  \n\t"   \
    "sd x21,8*21(a0)  \n\t"   \
    "sd x22,8*22(a0)  \n\t"   \
    "sd x23,8*23(a0)  \n\t"   \
    "sd x24,8*24(a0)  \n\t"   \
    "sd x25,8*25(a0)  \n\t"   \
    "sd x26,8*26(a0)  \n\t"   \
    "sd x27,8*27(a0)  \n\t"   \
    "sd x28,8*28(a0)  \n\t"   \
    "sd x29,8*29(a0)  \n\t"   \
    "sd x30,8*30(a0)  \n\t"   \
    "sd x31,8*31(a0)  \n\t"   \
    "add x0, a1, x7  #read rtemp7 to a1  \n\t"   \
    "fence  \n\t"   \
    "sd a1,8*10(a0)  #read rtemp7 to a1  \n\t"   \
);  

#define Load_regs(BaseAddr) asm volatile( \
    "li a0, " BaseAddr " \n\t"   \
    "ld x1,8*1(a0)  \n\t"   \
    "ld x2,8*2(a0)  \n\t"   \
    "ld x3,8*3(a0)  \n\t"   \
    "ld x4,8*4(a0)  \n\t"   \
    "ld x5,8*5(a0)  \n\t"   \
    "ld x6,8*6(a0)  \n\t"   \
    "ld x7,8*7(a0)  \n\t"   \
    "ld x8,8*8(a0)  \n\t"   \
    "ld x9,8*9(a0)  \n\t"   \
    "ld x11,8*11(a0)  \n\t"   \
    "ld x12,8*12(a0)  \n\t"   \
    "ld x13,8*13(a0)  \n\t"   \
    "ld x14,8*14(a0)  \n\t"   \
    "ld x15,8*15(a0)  \n\t"   \
    "ld x16,8*16(a0)  \n\t"   \
    "ld x17,8*17(a0)  \n\t"   \
    "ld x18,8*18(a0)  \n\t"   \
    "ld x19,8*19(a0)  \n\t"   \
    "ld x20,8*20(a0)  \n\t"   \
    "ld x21,8*21(a0)  \n\t"   \
    "ld x22,8*22(a0)  \n\t"   \
    "ld x23,8*23(a0)  \n\t"   \
    "ld x24,8*24(a0)  \n\t"   \
    "ld x25,8*25(a0)  \n\t"   \
    "ld x26,8*26(a0)  \n\t"   \
    "ld x27,8*27(a0)  \n\t"   \
    "ld x28,8*28(a0)  \n\t"   \
    "ld x29,8*29(a0)  \n\t"   \
    "ld x30,8*30(a0)  \n\t"   \
    "ld x31,8*31(a0)  \n\t"   \
    "ld a0,8*10(a0)  \n\t"   \
); 


#define Load_necessary(BaseAddr) asm volatile( \
    "li a0, " BaseAddr " \n\t"   \
    "ld sp,8*2(a0)  \n\t"   \
    "ld gp,8*3(a0)  \n\t"   \
    "ld tp,8*4(a0)  \n\t"   \
    "ld fp,8*8(a0)  \n\t"   \
); 

#define JmpTemp(num) asm volatile( \
    "or x0, x0, x" num " \n\t"   \
); 

#define WriteTemp(num, val) asm volatile( \
    "mv t0, %[data] \n\t"   \
    "sub x0, t0, x" num " \n\t"   \
    "fence \n\t"   \
    :  \
    :[data]"r"(val)  \
); 

//showlog = false
#define TakeOverAddrFalse 0x10664
//showlog = true
#define TakeOverAddrTrue 0x10672
#define ShowLog false

void takeoverSyscall()
{
    Save_int_regs(StoreIntRegAddr);
    Load_necessary(OldIntRegAddr);

    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    uint64_t saddr = runinfo->syscall_info_addr;
    if (runinfo->nowcallnum >= runinfo->totalcallnum){
        printf("syscall is overflow, exit!\n");
        exit(0);
    }

    SyscallInfo infos;
    infos = *((SyscallInfo *)(saddr+8+runinfo->nowcallnum*sizeof(infos)));
    if(runinfo->intregs[17] != infos.num){
        printf("syscall num is wrong! callnum: 0x%lx, record num: 0x%lx, 0x%lx\n", runinfo->intregs[17], infos.pc, infos.num);
        exit(0);
    }

    if(ShowLog) 
        printf("sysnum: 0x%lx\n", infos.num);

    if (infos.data_offset != 0xffffffff){
        if (infos.num == 0x3f) {    //read
            unsigned char *data = (unsigned char *)(saddr + infos.data_offset);
            unsigned char *dst = (unsigned char *)runinfo->intregs[11];
            for(int c=0;c<infos.data_size;c++){
                dst[c] = data[c];
            }

            if(ShowLog){
                printf("read, p0-p2: 0x%lx, 0x%lx, 0x%lx; record p0-p2: 0x%lx, 0x%lx, 0x%lx\n", runinfo->intregs[10], runinfo->intregs[11], runinfo->intregs[12], infos.p0, infos.p1, infos.p2);
            }
        }
        else if(infos.num == 0x40) { //write
            if(ShowLog){
                printf("write, p0-p2: 0x%lx, 0x%lx, 0x%lx; record p0-p2: 0x%lx, 0x%lx, 0x%lx\n", runinfo->intregs[10], runinfo->intregs[11], runinfo->intregs[12], infos.p0, infos.p1, infos.p2);
                char *data = (char *)(saddr + infos.data_offset);
                printf("record data: ");
                if(runinfo->intregs[10] == 1){
                    char t = data[runinfo->intregs[12]-1];
                    data[runinfo->intregs[12]-1]='\0';
                    printf("%s", (char *)data);
                    data[runinfo->intregs[12]-1] = t;
                }
            }
            char *dst = (char *)runinfo->intregs[11];
            if(runinfo->intregs[10] == 1){
                char t = dst[runinfo->intregs[12]-1];
                dst[runinfo->intregs[12]-1]='\0';
                printf("%s", (char *)dst);
                dst[runinfo->intregs[12]-1] = t;
            }
            printf("\n");
        }
        else{
            unsigned char *dst = (unsigned char *)infos.bufaddr;
            unsigned char *data = (unsigned char *)(saddr + infos.data_offset);
            for(int c=0;c<infos.data_size;c++){
                dst[c] = data[c];
            }
            if(ShowLog){
                printf("p0-p2: 0x%lx, 0x%lx, 0x%lx; record p0-p2: 0x%lx, 0x%lx, 0x%lx, 0x%lx(bufaddr)\n", runinfo->intregs[10], runinfo->intregs[11], runinfo->intregs[12], infos.p0, infos.p1, infos.p2, infos.bufaddr);
            }
        }
    }

    runinfo->nowcallnum ++;
    if(infos.hasret)
        runinfo->intregs[10] = infos.ret;
    
    infos.pc = infos.pc + 4;
    WriteTemp("5", infos.pc);
    Load_regs(StoreIntRegAddr);
    JmpTemp("5");
}

static inline int get_prot(uint32_t p_flags)
{
  int prot_x = (p_flags & PF_X) ? PROT_EXEC  : PROT_NONE;
  int prot_w = (p_flags & PF_W) ? PROT_WRITE : PROT_NONE;
  int prot_r = (p_flags & PF_R) ? PROT_READ  : PROT_NONE;
  return (prot_x | prot_w | prot_r);
}

//replace ecall with jmptemp 6
void replaceEcall(uint16_t *text, uint64_t length)
{
    uint16_t data1 = 0x0000, data0 = 0x0073;
    //JmpTemp("6");//00606033
    uint16_t pdata1 = 0x0060, pdata0 = 0x6033;
    for(int i=0;i<length;i++) {
        if(text[i] == data0 && text[i+1] == data1){
            text[i] = pdata0;
            text[i+1] = pdata1;
        }
    }
}


uint64_t loadelf(char * progname)
{
	Elf64_Ehdr ehdr;
	Elf64_Phdr phdr;
    uint64_t startvaddr;

	FILE *fp = fopen(progname, "r");
	if (fp == 0) {
		printf("cannot open %s\n", progname);
        exit(1);
	}

	if (fread(&ehdr, sizeof(ehdr), 1, fp) != 1) {
        printf("cannot read ehdr\n");
		exit(1);
	}
	
    startvaddr = ehdr.e_entry;
	for(int i=0; i<ehdr.e_phnum; i++) 
    {
		fseek(fp, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
		if (fread(&phdr, sizeof(phdr), 1, fp) != 1 ) {
			printf("cannot read phdr\n");
		    exit(1);
		}
        //PT_LOAD 1
        if(phdr.p_type == 1 && phdr.p_memsz && phdr.p_filesz > 0){
            uint64_t prepad = phdr.p_vaddr % 4096; 
            uint64_t vaddr = phdr.p_vaddr - prepad;
            uint64_t postpad = 4096 - (phdr.p_vaddr + phdr.p_memsz) % 4096;
            uint64_t pmemsz = (phdr.p_vaddr + phdr.p_memsz + postpad) - vaddr;
            int prot = get_prot(phdr.p_flags);
            if(prot & PROT_WRITE) {
                data_seg.addr = vaddr;
                data_seg.size = pmemsz;
                printf("data segment info, addr: 0x%lx, size: 0x%lx, endaddr: 0x%lx\n", phdr.p_vaddr, phdr.p_memsz, phdr.p_vaddr + phdr.p_memsz);
                printf("padded data segment info, addr: 0x%lx, size: 0x%lx, endaddr: 0x%lx\n", data_seg.addr, data_seg.size, vaddr + pmemsz);
                continue;
            }
            if(prot & PROT_EXEC) {
                text_seg.addr = vaddr;
                text_seg.size = pmemsz;
                printf("text segment info, addr: 0x%lx, size: 0x%lx, endaddr: 0x%lx\n", phdr.p_vaddr, phdr.p_memsz, phdr.p_vaddr + phdr.p_memsz);
                printf("padded text segment info, addr: 0x%lx, size: 0x%lx, endaddr: 0x%lx\n", text_seg.addr, text_seg.size, vaddr + pmemsz);
            }	

            uint64_t alloc_vaddr = (uint64_t)mmap((void*) vaddr, pmemsz, prot | PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, -1, 0);
            printf("vaddr: 0x%lx, alloc_vaddr: 0x%lx\n", vaddr, alloc_vaddr);	
            assert(alloc_vaddr == vaddr);  
                
            fseek(fp, phdr.p_offset, SEEK_SET);
            if (fread((void *)phdr.p_vaddr, phdr.p_filesz, 1, fp) != 1) {
                printf("cannot read phdr from file: %d\n", i);
                exit(1);
            }

            Elf64_Shdr string_shdr, shdr;
            fseek(fp, ehdr.e_shoff+ehdr.e_shstrndx*ehdr.e_shentsize, SEEK_SET);
            fread(&string_shdr, sizeof(string_shdr), 1, fp);
	        fseek(fp, string_shdr.sh_offset + string_shdr.sh_size, SEEK_SET);
            for (int c = 0; c < ehdr.e_shnum; c++) {
                fseek(fp, ehdr.e_shoff + c * ehdr.e_shentsize, SEEK_SET);
                fread(&shdr, sizeof(shdr), 1, fp );
                if(shdr.sh_size!=0 && shdr.sh_flags & 0x4){//SHF_EXECINSTR = 0x4
                    printf("code section: 0x%lx, 0x%lx\n", shdr.sh_addr, shdr.sh_size);
                    replaceEcall((uint16_t *)shdr.sh_addr, shdr.sh_size/2);
                }
            }
            if (!(prot & PROT_WRITE)){
                if (mprotect((void *)phdr.p_vaddr, phdr.p_memsz, prot) == -1) {
                    printf("mprotect error: %d\n", i);
                    exit(1);
                }
            }
        }
	}
    fclose(fp);
    return startvaddr;
}


void read_ckptsyscall(char filename[]);
void read_ckptinfo(char ckptinfo[], char ckpt_sysinfo[])
{
    uint64_t npc, intregs[32], fpregs[32], mrange_num=0, loadnum = 0;
    MemRangeInfo memrange, extra;
    LoadInfo loadinfo;
    FILE *p=fopen(ckptinfo,"rb");

    fread(&npc, 8, 1, p);
    RunningInfo *runinfo = (RunningInfo *)RunningInfoAddr;
    fread(&runinfo->intregs[0], 8, 32, p);
    fread(&fpregs[0], 8, 32, p);

    // for(int i=0;i<32;i++){
    //     printf("ireg %d: 0x%lx\n", i, runinfo->intregs[i]);
    // }

    //init memrange
    fread(&mrange_num, 8, 1, p);
    for(int i=0;i<mrange_num;i++){
        fread(&memrange, sizeof(memrange), 1, p);
        // printf("raw memrange: 0x%llx, 0x%llx\n", memrange.addr, memrange.size);

        extra.size = 0;
        extra.addr = 0;
        uint64_t msaddr = memrange.addr, meaddr = memrange.addr + memrange.size;
        uint64_t tsaddr = text_seg.addr, teaddr = text_seg.addr + text_seg.size;
        //delete the range that covered by text segment
        if(msaddr < tsaddr && meaddr > tsaddr) {
            memrange.size = tsaddr - msaddr;
            if(meaddr > teaddr) {
                extra.addr = teaddr;
                extra.size = meaddr - teaddr;
            }
            // printf("new memrange: 0x%llx 0x%llx, extra: 0x%llx 0x%llx\n", memrange.addr, memrange.size, extra.addr, extra.size);
        }
        else if(msaddr >= tsaddr && msaddr <= teaddr) {
            if(meaddr <= teaddr) {
                memrange.size = 0;
            }
            else{
                memrange.addr = teaddr;
                memrange.size = meaddr - teaddr;
            }
            // printf("new memrange: 0x%llx 0x%llx, extra: 0x%llx 0x%llx\n", memrange.addr, memrange.size, extra.addr, extra.size);
        }

        if(memrange.size !=0){
            if(memrange.addr < 0xfffffffff){ //
                int* arr = static_cast<int*>(mmap((void *)memrange.addr, memrange.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0));
                printf("map addr raw: 0x%lx, 0x%lx\n", arr, memrange.addr);
                assert(memrange.addr == (uint64_t)arr);  
            }
        }

        if(extra.size !=0){
            int* arr1 = static_cast<int*>(mmap((void *)extra.addr, extra.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0));
            printf("map addr extra: 0x%lx, 0x%lx\n", arr1, extra.addr);
            assert(extra.addr == (uint64_t)arr1); 
        }
    }
    
    //加载syscall的执行信息到内存中
    read_ckptsyscall(ckpt_sysinfo);

    //init first load to memory
    fread(&loadnum, 8, 1, p);
    for(int i=0;i<loadnum;i++){
        fread(&loadinfo, sizeof(loadinfo), 1, p);
        //栈不能乱写，会出现问题，需要在最后时刻写栈  
        if(loadinfo.addr < 0xfffffffff){
            switch(loadinfo.size) {
                case 1: *((uint8_t *)loadinfo.addr) = (uint8_t)loadinfo.data; break;
                case 2: *((uint16_t *)loadinfo.addr) = (uint16_t)loadinfo.data; break;
                case 4: *((uint32_t *)loadinfo.addr) = (uint32_t)loadinfo.data; break;
                case 8: *((uint64_t *)loadinfo.addr) = loadinfo.data; break;
            }
        }
    }
    fclose(p);
    printf("\n");

    WriteTemp("5", npc);
    uint64_t takeover_addr;
    takeover_addr = ShowLog ? TakeOverAddrTrue : TakeOverAddrFalse;
    WriteTemp("6", takeover_addr);

    Save_int_regs(OldIntRegAddr);
    Load_regs(StoreIntRegAddr);
    JmpTemp("5");
}