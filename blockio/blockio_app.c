#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define KEYVALUE     0x01   
#define INVAKEY      0xFF


int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    unsigned char keyvalue;

    if(argc != 2)
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
    
    while(1)
    {
        retvalue = read(fd, &keyvalue, sizeof(keyvalue));
        if(retvalue < 0)
        {

        }
        else
        {
            if(keyvalue == KEYVALUE)
                printf("KEY0 value = %#x \r\n",keyvalue);
        }
    }

    retvalue = close(fd);
    if(retvalue < 0)
    {
        printf("file %s close failed!\r\n",filename);
        return -1;
    }
    return 0;
}