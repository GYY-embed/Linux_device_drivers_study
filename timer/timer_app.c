#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

#define CLOSE_CMD       (_IO(0xEF,0x1))
#define OPEN_CMD        (_IO(0xEF,0x2))
#define SETPERIOD_CMD   (_IO(0xEF,0x3))



int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    unsigned char databuf[1];
    unsigned long cmd;
    unsigned long arg;
    unsigned char str[100];

    if(argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }
    printf("Open file %s OK!\r\n", filename);
    
    while(1)
    {
        printf("Input CMD: ");
        retvalue = scanf("%d", &cmd);
        printf("cmd:%d\r\n",cmd);
        if(cmd == 1)
            cmd = CLOSE_CMD;
        else if(cmd == 2)
            cmd = OPEN_CMD;
        else if(cmd == 3)
        {
            cmd = SETPERIOD_CMD;
            printf("Input timer period: ");
            retvalue = scanf("%d",&arg);
            if(retvalue != 1)
            {
                gets(str);
            }
        }
        ioctl(fd, cmd, arg);
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