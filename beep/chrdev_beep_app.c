#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define LEDON   1
#define LEDOFF  0


int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    unsigned char databuf[1];

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
        printf("BEEP Control Failed!\r\n");
        close(fd);
        return -1;
    }
     printf("BEEP Control Successed!\r\n");

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