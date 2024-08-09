#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "TcpServer.h"

int main(int argc, char* argv[])
{
    printf("sysyterm start!\n");

#if 0
    if (argc < 3)
    { 
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
#else
    unsigned short port = 10000;  
    chdir("/home/fyyz/test/");  
#endif
    struct TcpServer* server = tcpServerInit(port, 4);      // 服务器初始化
    tcpServerRun(server);                                   // 启动服务器
    return 0;
}