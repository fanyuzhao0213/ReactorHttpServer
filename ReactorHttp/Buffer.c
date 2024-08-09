#define _GNU_SOURCE
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>

struct Buffer* bufferInit(int size)                                        //Buffer初始化
{
    struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
    if(buffer != NULL)
    {
        buffer->data = (char*)malloc(size);
        buffer->capacity = size;
        buffer->readPos = buffer->writePos = 0;
        memset(buffer->data,0,size);
    }

    return buffer;
}

void bufferDestroy(struct Buffer* buf)                                     //Buffer销毁
{
    if(buf != NULL)
    {
        if(buf->data != NULL)
        {
            free(buf->data);
        }
    }
    free(buf);
}

void bufferExtendRoom(struct Buffer* buffer, int size)                     //Buffer扩容
{
    //内存够用 不用扩容
    if(bufferWriteableSize(buffer) >= size)
    {
        return;
    }
    //内存需要合并才能用
    else if(bufferWriteableSize(buffer) + buffer->readPos >= size)
    {
        //得到未读的内存大小
        int readable = bufferReadableSize(buffer);
        //移动内存
        memcpy(buffer->data,buffer->data+buffer->readPos,readable);
        //更新位置
        buffer->readPos = 0;
        buffer->writePos = readable;
    }
    //内存不够用
    else
    {
        void* tmp = realloc(buffer->data,buffer->capacity+size);
        if(tmp == NULL)
        {
            perror("realloc");
            return;
        }
        memset(tmp+buffer->capacity,0,size);        //  申请出来的多余内存全部清除为0
        buffer->data = tmp;
        buffer->capacity += size;
    }


}

int bufferWriteableSize(struct Buffer* buffer)                             // 得到剩余的可读的内存容量
{
    return buffer->capacity - buffer->writePos;
}

int bufferReadableSize(struct Buffer* buffer)                              // 得到剩余的可读的内存容量
{
    return buffer->writePos - buffer->readPos;
}

int bufferAppendData(struct Buffer* buffer, const char* data, int size)    //写数据
{
    if(buffer == NULL || data == NULL || size <=0)
    {
        return -1;
    }

    //扩容,内存够就不需要扩容
    bufferExtendRoom(buffer,size);
    //数据拷贝
    memcpy(buffer->data+buffer->writePos,data,size);
    buffer->writePos += size;
    return 0;
}

int bufferAppendString(struct Buffer* buffer, const char* data)            //写字符串
{
    int size = strlen(data);
    int ret = bufferAppendData(buffer,data,size);
    return ret;
}

int bufferSocketRead(struct Buffer* buffer, int fd)                        //接收套接字数据
{
    //readv方式读取
    struct iovec vec[2];
    //初始化数组元素
    int writeable = bufferWriteableSize(buffer);
    vec[0].iov_base = buffer->data+buffer->writePos;
    vec[0].iov_len = writeable;
    char* tmpbuf = (char*)malloc(40960);        //申请4K用于vec[1]
    vec[1].iov_base = vec[0].iov_base = buffer->data+buffer->writePos;
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2);
    if(result == -1)       //读取错误
    {
        perror("readv");
        return -1;
    }
    else if(result <= writeable)   //大小全部读取到第一个缓冲区
    {
        buffer->writePos += result;
    }
    else
    {
        buffer->writePos += buffer->capacity;
        bufferAppendData(buffer,tmpbuf,result - writeable);
    }
    free(tmpbuf);
    return result;
}

char* bufferFindCRLF(struct Buffer* buffer)                                //根据\r\n取出一行, 找到其在数据块中的位置, 返回该位置
{
    // strstr --> 大字符串中匹配子字符串(遇到\0结束) char *strstr(const char *haystack, const char *needle);
    // memmem --> 大数据块中匹配子数据块(需要指定数据块大小)
    // void *memmem(const void *haystack, size_t haystacklen,
    //      const void* needle, size_t needlelen);
    char* ptr = memmem(buffer->data+buffer->readPos,bufferReadableSize(buffer),"\r\n",2);
    return ptr;
}

int bufferSendData(struct Buffer* buffer, int socket)                      //发送数据
{
    //判断有无数据
    int readable = bufferReadableSize(buffer);
    if(readable > 0)
    {
        int count = send(socket,buffer->data+buffer->readPos,readable,MSG_NOSIGNAL);
        if(count > 0)
        {
            buffer->readPos += count;
            usleep(1);
        }
        return count;
    }

    return 0;
}
