#include "info.h"

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


#define PF_X 1
#define PF_W 2
#define PF_R 4

MemRangeInfo data_seg, text_seg;

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
			printf("read phdr wrong!\n");
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
                continue;
            }
            if(prot & PROT_EXEC) {
                text_seg.addr = vaddr;
                text_seg.size = pmemsz;
                if(ShowLog){
                    printf("text segment info, addr: 0x%lx, size: 0x%lx, endaddr: 0x%lx\n", phdr.p_vaddr, phdr.p_memsz, phdr.p_vaddr + phdr.p_memsz);
                    printf("padded text segment info, addr: 0x%lx, size: 0x%lx, endaddr: 0x%lx\n", text_seg.addr, text_seg.size, vaddr + pmemsz);
                }
            }	

            uint64_t alloc_vaddr = (uint64_t)mmap((void*)vaddr, pmemsz, prot | PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, -1, 0);
            printf("--- load elf: alloc text memory: (0x%lx, 0x%lx) ---\n", alloc_vaddr, alloc_vaddr + pmemsz);	
            if(alloc_vaddr != vaddr){
                printf("required vaddr: 0x%lx, alloc vaddr: 0x%lx\n", vaddr, alloc_vaddr);	
                exit(1);
            }
                
            fseek(fp, phdr.p_offset, SEEK_SET);
            if (fread((void *)phdr.p_vaddr, phdr.p_filesz, 1, fp) != 1) {
                printf("cannot read phdr from file: %d\n", i);
                exit(1);
            }

            //find text segment and replace ecall with jmp rtemp
            Elf64_Shdr string_shdr, shdr;
            fseek(fp, ehdr.e_shoff + ehdr.e_shstrndx * ehdr.e_shentsize, SEEK_SET);
            fread(&string_shdr, sizeof(string_shdr), 1, fp);
	        fseek(fp, string_shdr.sh_offset + string_shdr.sh_size, SEEK_SET);
            printf("--- ecall replace in text segment --- \n");
            for (int c = 0; c < ehdr.e_shnum; c++) {
                fseek(fp, ehdr.e_shoff + c * ehdr.e_shentsize, SEEK_SET);
                fread(&shdr, sizeof(shdr), 1, fp );
                if(shdr.sh_size!=0 && shdr.sh_flags & 0x4){//SHF_EXECINSTR = 0x4
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