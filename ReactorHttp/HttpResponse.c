#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define resHeaderSize 16

struct HttpResponse* httpResponseInit()    // 初始化
{
    struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
    response->headerNum = 0;
    int size = sizeof(struct ResponseHeader) * resHeaderSize;
    response->headers = (struct ResponseHeader*)malloc(size);
    response->statusCode = Unknown;
    //初始化数组
    bzero(response->headers,size);
    bzero(response->statusMsg,sizeof(response->statusMsg));
    bzero(response->fileName,sizeof(response->fileName));

    //函数指针
    response->sendDataFunc = NULL;

    return response;
}

void httpResponseDestroy(struct HttpResponse* response)    // 销毁
{
    if(response != NULL)
    {
        free(response->headers);
        free(response);
    }
}

void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value)// 添加响应头
{
    if (response == NULL || key == NULL || value == NULL)
    {
        return;
    }
    strcpy(response->headers[response->headerNum].key, key);
    strcpy(response->headers[response->headerNum].value, value);
    response->headerNum++;
}
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket)// 组织http响应数据
{
     //状态行
    char tmp[1024] = {0};
    sprintf(tmp, "HTTP/1.1 %d %s\r\n",response->statusCode,response->statusMsg);
    bufferAppendString(sendBuf,tmp);
    //响应头
    for(int i=0; i<response->headerNum; i++)
    {
        sprintf(tmp,"%s: %s\r\n",response->headers[i].key,response->headers[i].value);
        bufferAppendString(sendBuf,tmp);
    }  
    //空行
    bufferAppendString(sendBuf,"\r\n");

    //发送数据
    #ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, socket);   
    #endif // !MSG_SEND_AUTO
    

    //回复数据
    response->sendDataFunc(response->fileName,sendBuf,socket);
}