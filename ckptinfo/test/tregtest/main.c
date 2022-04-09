#include<stdio.h>
#include<stdlib.h>

#define SetSTemReg(src1, src2, dst1, dst2) asm volatile( \
    "mv t0, %[rtemp1]  \n\t"  \
    "sub x0, t0, x0  # write rtemp0 \n\t"  \
    "mv t0, %[rtemp2]  \n\t"  \
    "sub x0, t0, x1  # write rtemp 1 \n\t"  \
    "li t0, 0x10638  \n\t"  \
    "sub x0, t0, x2  # write rtemp 2 \n\t"  \
    "add x0, t1, x0  # read rtemp0 \n\t"  \
    "mv %[wtemp1], t1  \n\t"  \
    "add x0, t1, x1  # read rtemp 1 \n\t"  \
    "or x0, x0, x2  # read rtemp 1 \n\t"  \
    "addi t1, t1, 1  \n\t"  \
    "addi t1, t1, 1  \n\t"  \
    "addi t1, t1, 1  \n\t"  \
    "addi t1, t1, 1  #here \n\t"  \
    "addi t1, t1, 1  \n\t"  \
    "mv %[wtemp2], t1  \n\t"  \
    :[wtemp1]"=r"(dst1), [wtemp2]"=r"(dst2)  \
    :[rtemp1]"r"(src1), [rtemp2]"r"(src2)  \
); 

int main()
{
	unsigned long long s1=1234, s2=4567, d1=0, d2=0;
    SetSTemReg(s1, s2, d1, d2);
    printf("%ld, %ld, %ld, %ld\n", s1, s2, d1, d2);
	return 0;
}
