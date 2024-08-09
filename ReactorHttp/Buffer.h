#pragma once

struct Buffer
{
    char* data;                 //指向内存的指针
    int capacity;               //容量
    int readPos;                //读指针
    int writePos;               //写指针
};

struct Buffer* bufferInit(int size);                                        //Buffer初始化
void bufferDestroy(struct Buffer* buf);                                     //Buffer销毁
void bufferExtendRoom(struct Buffer* buffer, int size);                     //Buffer扩容
int bufferWriteableSize(struct Buffer* buffer);                             // 得到剩余的可读的内存容量
int bufferReadableSize(struct Buffer* buffer);                              // 得到剩余的可读的内存容量
int bufferAppendData(struct Buffer* buffer, const char* data, int size);    //写数据
int bufferAppendString(struct Buffer* buffer, const char* data);            //写字符串
int bufferSocketRead(struct Buffer* buffer, int fd);                        //接收套接字数据
char* bufferFindCRLF(struct Buffer* buffer);                                //根据\r\n取出一行, 找到其在数据块中的位置, 返回该位置
int bufferSendData(struct Buffer* buffer, int socket);                      //发送数据 