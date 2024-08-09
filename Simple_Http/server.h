#pragma once                                                    //类似于ifndef
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <pthread.h>

extern int InitListenFd(unsigned short port);                   // 初始化监听的套接字
extern int epollRun(int lfd);                                   // 启动epoll

//建立新连接
// void acceptClient(int lfd, int epfd);
void* acceptClient(void *arg);
// 接收http请求
void* recvHttpRequest(void *arg);
// int recvHttpRequest(int cfd, int epfd);
/*解析请求行*/
int parseRequestLine(const char* line,int cfd);
//发送文件
int sendFile(const char* filename,int cfd);
//发送文件
int sendFile(const char* filename,int cfd);
//发送目录，以html的格式发送
int sendDir(const char* dirName, int cfd);
//发送http响应头
int sendHeaderMsg(int cfd, int status, const char* descr,const char* type, int length);


// 将字符转换为整形数
int hexToDec(char c);
// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from);
//根据文件后缀，返回对应的Content-Type
const char* getFileType(const char* name);