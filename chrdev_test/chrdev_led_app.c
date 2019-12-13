#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"


static char userdata[] = {"user data!"};

int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    char readbuf[100], writebuf[100];

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

    if(atoi(argv[2]) == 1)
    {
        /*read from device driver*/
        retvalue = read(fd, readbuf,50);
        if(retvalue < 0)
        {
            printf("read file %s failed!\r\n", filename);
        }
        else
        {
            printf("read data : %s\r\n", readbuf);
        }
    }

    if(atoi(argv[2]) == 2)
    {
        /*write to device driver*/
        memcpy(writebuf, userdata, sizeof(userdata));
        retvalue = write(fd, writebuf, 50);
        if(retvalue < 0)
        {
            printf("write file %s failed!\r\n",filename);
            return -1;
        }
    }

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