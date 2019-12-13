#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    unsigned char databuf[1];
    int cnt=0;

    if(argc != 3)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    
    /*open device*/
    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }
    printf("Open file %s OK!\r\n", filename);
    
    databuf[0]=atoi(argv[2]);
    /*write data*/
    retvalue = write(fd, databuf, sizeof(databuf));
    if(retvalue < 0)
    {
        printf("LED Control Failed!\r\n");
        close(fd);
        return -1;
    }
     printf("LED Control Successed!\r\n");
    while(1)
    {
        sleep(5);
        cnt++;
        printf("APP running times:%d\r\n",cnt);
        if(cnt>=5)
            break;
    }
    printf("APP running finished!\r\n");

    /*close device*/
    retvalue = close(fd);
    if(retvalue < 0)
    {
        printf("Can't close file %s\r\n",filename);
        return -1;
    }
    printf("Close file %s OK!\r\n", filename);

    return 0;
}