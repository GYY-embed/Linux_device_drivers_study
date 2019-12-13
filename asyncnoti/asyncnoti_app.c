#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "signal.h"
#include "linux/ioctl.h"


#define KEYVALUE     0x01   
#define INVAKEY      0xFF

static int fd = 0;

static void sigio_signal_func(int signum)
{
    int err = 0;
    unsigned int keyvalue = 0;

    err = read(fd, &keyvalue, sizeof(keyvalue));
    if(err < 0)
    {

    }
    else
    {
        printf("SIGIO signal! key value = %d\r\n",keyvalue);
    }
    
}
int main(int argc, char *argv[])
{
    int retvalue;
    int flags = 0;
    char *filename;
    unsigned char keyvalue;
    fd_set readfds;
    struct timeval timeout;

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
    
    signal(SIGIO, sigio_signal_func);

    fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFL, flags | FASYNC);

    while(1)
    {
        sleep(2);
    }
    retvalue = close(fd);
    if(retvalue < 0)
    {
        printf("file %s close failed!\r\n",filename);
        return -1;
    }
    return 0;
}