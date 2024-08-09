#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "server.h"
#include <signal.h>

void handle_sigpipe(int signum) {
    // SIGPIPE信号处理程序
    printf("Received SIGPIPE, ignoring it...\n");
}


int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
    
    // 初始化用于监听的套接字
    int lfd = InitListenFd(port);

    // 忽略SIGPIPE信号
    //使用Edge浏览器测试MP3时，sendfile可能会遇到SIGPIPE，原因是浏览器发了2个连接，
    //第一个连接在sendfile之前被客户端关闭了，导致发送出现SIGPIPE异常，
    //解决：先用signal()函数把sigpipe忽略，然后在判断ret返回-1时，处理errno != EAGAIN的情况，break即可。
    signal(SIGPIPE, handle_sigpipe);

    // 启动服务器程序
    epollRun(lfd);

    return 0;
}