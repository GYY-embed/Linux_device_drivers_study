#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"


#define KEYVALUE     0x01   
#define INVAKEY      0xFF


int main(int argc, char *argv[])
{
    int fd,retvalue;
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
    
    while (1) 
    {	
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		/* 构造超时时间 */
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000; /* 500ms */
		retvalue = select(fd + 1, &readfds, NULL, NULL, &timeout);
		switch (retvalue) {
			case 0: 	/* 超时 */
				/* 用户自定义超时处理 */
				break;
			case -1:	/* 错误 */
				/* 用户自定义错误处理 */
				break;
			default:  /* 可以读取数据 */
				if(FD_ISSET(fd, &readfds)) {
					retvalue = read(fd, &keyvalue, sizeof(keyvalue));
					if (retvalue < 0) {
						/* 读取错误 */
					} else {
						if (keyvalue == KEYVALUE)
							printf("key value=%d\r\n", keyvalue);
					}
				}
				break;
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