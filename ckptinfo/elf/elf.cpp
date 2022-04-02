#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

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

#define PF_X 1
#define PF_W 2
#define PF_R 4

static inline int get_prot(uint32_t p_flags)
{
  int prot_x = (p_flags & PF_X) ? PROT_EXEC  : PROT_NONE;
  int prot_w = (p_flags & PF_W) ? PROT_WRITE : PROT_NONE;
  int prot_r = (p_flags & PF_R) ? PROT_READ  : PROT_NONE;
  return (prot_x | prot_w | prot_r);
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
            printf("hdr %d, off: 0x%lx, fsize: %ld, vaddr: 0x%lx, memsize: %ld\n", i, phdr.p_offset, phdr.p_filesz, phdr.p_vaddr, phdr.p_memsz);

            uint64_t prepad = phdr.p_vaddr % 4096; 
            uint64_t vaddr = phdr.p_vaddr - prepad;
            int prot = get_prot(phdr.p_flags);
            uint64_t alloc_vaddr = (uint64_t)mmap((void*) vaddr, phdr.p_memsz + prepad, prot | PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, -1, 0);
            printf("vaddr: 0x%lx, alloc_vaddr: 0x%lx, prepad: %lx\n", vaddr, alloc_vaddr, prepad);	
            assert(alloc_vaddr == vaddr);  	
                
            fseek(fp, phdr.p_offset, SEEK_SET);
            if (fread((void *)phdr.p_vaddr, phdr.p_filesz, 1, fp) != 1) {
                printf("cannot read phdr from file: %d\n", i);
                exit(1);
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
    //((void*(*)())startvaddr)();
    return startvaddr;
}

int main(int argc, char **argv)
{
    if(argc < 2){
        printf("parameters are not enough!\n");
        printf("./read ckptinfo.log ckpt_syscallinfo.log \n");
        return 0;
    }

    uint64_t saddr = loadelf(argv[1]);

    asm volatile( 
        "mv ra, %[addr]  \n\t"  
        "ret  \n\t"  
        : 
        :[addr]"r"(saddr)  
    );

    return 0;
}
