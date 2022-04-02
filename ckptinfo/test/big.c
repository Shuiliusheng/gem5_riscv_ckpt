#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

int main()
{
	int a[1000];
	for(int i=0;i<1000;i+=50){
		for(int j=i;j<i+50;j++){
			a[j]=j;
			printf("%d ", j);
		}
		printf("\n");
	}

	struct timeval tval;
    int ret = gettimeofday(&tval, NULL);
    if (ret == -1){
        printf("Error: gettimeofday()\n");
        return ret;
    }
	printf("start sec: %ld, usec: %ld\n", tval.tv_sec, tval.tv_usec);

	FILE *p=fopen("test.txt","wb");
	fwrite(&a[0], sizeof(int), 1000, p);
	fclose(p);

	char path[255];
	printf("%s\n", getcwd(path, sizeof(path)));

	struct stat buf;
	int fd;
    fd = open("test.txt", O_RDONLY);
	fstat(fd, &buf);
   	printf("/etc/passwd file size +%d\n ", buf.st_size);
	printf("buf addr: 0x%lx, %d\n", (unsigned long long)&buf, sizeof(struct stat));
	printf("stat, dev: 0x%lx, ino: 0x%lx, mode: 0x%lx\n", buf.st_dev, buf.st_ino, buf.st_mode);
	printf("stat, nlink: 0x%lx, uid: 0x%lx, gid: 0x%lx, rdev: 0x%lx\n", buf.st_nlink, buf.st_uid, buf.st_gid, buf.st_rdev);
	printf("stat, size: 0x%lx, blksize: 0x%lx, blocks: 0x%x\n", buf.st_size, buf.st_blksize, buf.st_blocks);

	int *arr = (int *)malloc(sizeof(int)*2000);
	for(int i=0;i<2000;i++){
		arr[i]=a[i%100];
	}

	struct timeval tval1;
    int ret1 = gettimeofday(&tval1, NULL);
    if (ret1 == -1){
        printf("Error: gettimeofday()\n");
        return ret1;
    }
	printf("end sec: %ld, usec: %ld\n", tval1.tv_sec, tval1.tv_usec);
    
	FILE *p1=fopen("test.txt","rb");
	fread(&arr[0], sizeof(int), 1000, p1);
	fclose(p1);
	for(int i=0;i<1000;i+=50){
		for(int j=i;j<i+50;j++){
			printf("%d ", arr[j]);
		}
		printf("\n");
	}
	return 0;
}
