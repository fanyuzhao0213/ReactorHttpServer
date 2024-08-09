#include "server.h"

struct FdInfo
{
    int fd;
    int epfd;
    pthread_t tid;
};


// 初始化监听的套接字
int InitListenFd(unsigned short port)
{
    int ret = 0;
    int lfd = 0;


    // 1. 创建监听的fd
    lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd == -1)
    {
        perror("socket");
        return -1;
    }

    // 2. 设置端口复用
    int opt = 1;
    ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        return -1;
    }

    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;          //ipv4协议
    addr.sin_port = htons(port);        //端口转为网络大端字节序
    addr.sin_addr.s_addr = INADDR_ANY;  //绑定本机任意IP，因为是0，所以大小端不用转
    ret = bind(lfd,(struct sockaddr*)&addr,sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        return -1;
    }

    //4.设置监听
    ret = listen(lfd,128);
    if (ret == -1)
    {
        perror("listen");
        return -1;
    }

    //返回监听的描述符
    return lfd;  
}


int epollRun(int lfd)
{
    int ret = 0;

    //创建1个epoll模型
    int epfd = epoll_create(1); //参数1无意义，只要比0大就行
    if(epfd == -1)
    {
        perror("epoll create");
        return -1;
    }

    //2.lfd上树
    //创建epoll_event实例
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;        //读事件
    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    if(ret == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    //定义一个传出参数，epoll_event结构体数组
    struct epoll_event evs[1024];
    int evsize = sizeof(evs)/sizeof(evs[0]);
    while(1)
    {
        int num = epoll_wait(epfd,evs,evsize,-1);  // -1代表一直查询,阻塞
        for(int i = 0; i < num; i++)
        {
            struct FdInfo *info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
            int fd = evs[i].data.fd;
            info->fd = fd;
            info->epfd = epfd;
            if(fd == lfd)
            {
                //监听的描述符
                //建立新连接
                pthread_create(&info->tid,NULL,acceptClient,info);
                // acceptClient(lfd,epfd);
            }
            else
            {
                //通信的描述符
                pthread_create(&info->tid,NULL,recvHttpRequest,info);
                // recvHttpRequest(fd,epfd);    
            }
        }
    }
    
}

//建立新连接
// void acceptClient(int lfd, int epfd)
void* acceptClient(void *arg)
{
    struct FdInfo* info = (struct FdInfo*)arg;

    int ret;
    //1.建立连接
    int cfd = accept(info->fd,NULL,NULL);
     if(ret == -1)
    {
        perror("accept");
        exit(-1);
    }

    //2.设置为非阻塞模式
    int flag = fcntl(cfd,F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd,F_SETFL,flag);

    //3.cfd添加到epoll中
    struct epoll_event ev;
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET;  //EPOLLET 设置为边沿模式，数据只触发一次，配合非zu塞
    ret = epoll_ctl(info->epfd,EPOLL_CTL_ADD,cfd,&ev);
    if(ret == -1)
    {
        perror("epoll_ctl");
        exit(-1);
    }
    printf("acceptcliet threadId: %ld\n", info->tid);
    free(info);
    return NULL;
}


// int recvHttpRequest(int cfd, int epfd)
void* recvHttpRequest(void *arg)
{
    struct FdInfo* info = (struct FdInfo*)arg;

    int len = 0;                    //每次接收数据的长度
    int totle = 0;                  //总接收数据长度
    char tmp[1024] = {0};           //接收数据的临时buf
    char revbuf[1024] = {0};       //接收数据的revbuf
    while ((len = recv(info->fd,tmp,sizeof(tmp),0)) > 0)
    {
        if(totle + len < sizeof(revbuf))
        {
            memcpy(revbuf+totle,tmp,len);
        }
        totle += len;
    }

    //判断数据是否接收完毕
    //因为用的是epoll的边沿触发模式，并且设置了recv为非阻塞模式
    if(len == -1 && errno == EAGAIN)
    {
        //如果非阻塞模式下读数据，没有数据的清空下会返回errno变量为EAGAIN
        //解析请求行
        //get /xxx/xxx.xxx http/1.1 \r\n
        printf("----revbuf:%s\n",revbuf);
        char *pt = strstr(revbuf,"\r\n");//从接收到的数据中截取首部的请求行，找到\r\n
        int reqlinelen = pt-revbuf;     //指针相减得到此时去掉\r\n数据的长度
        revbuf[reqlinelen] = '\0';      //追加字符串结尾标志
        //解析
        printf("----revbuf:%s\n",revbuf);
        parseRequestLine(revbuf,info->fd);
    }
    else if (len == 0)
    {
        //客户端断开了连接
        epoll_ctl(info->epfd,EPOLL_CTL_DEL,info->fd,NULL);
        close(info->fd);
    }
    else
    {
        perror("recv client");
    }
    printf("recvMsg threadId: %ld\n", info->tid);
    free(info);
    return NULL;
}

// 将字符转换为整形数
int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);
            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            *to = *from;
        }
    }
    *to = '\0';
}


/*解析请求行*/
int parseRequestLine(const char* line,int cfd)
{
    //get /xxx/xxx.xxx http/1.1 \r\n
    char method[12] = {0};      //get /post
    char path[1024] = {0};      //原始路径
    char dirpath[1024] = {0};   //将utf-8转换的最终路径
    printf("line:%s\n",line);
    sscanf(line,"%[^ ] %[^ ]",method,path);
    printf("parseRequestLine mathod:%s,path:%s\n",method,path);
    if(strcasecmp(method, "get") != 0)  //解析请求头不是get、GET
    {
        return -1;
    }

    decodeMsg(dirpath,path);

    printf("dirpath:%s\n",dirpath);
    //处理客户端请求的静态资源
    char *clientFile = NULL;
    if(strcmp(dirpath,"/") == 0)    //判断是否只是'/'
    {
        clientFile = "./";          //如果是 / 则更改为文件为./当前文件
    }
    else
    {
        // 去除路径中的尾部斜杠（如果不是根路径）  //20240802add 因为在windows测试每个文件后面都多了1个 '/'
        int len = strlen(dirpath);
        if (len > 1 && dirpath[len - 1] == '/')
        {
            printf("******\n");
            dirpath[len - 1] = '\0';
        }
        clientFile = dirpath+1;     //不是当前文件，则去掉前面的‘/’
        printf("clientFile:%s\n",clientFile);
    }

    //获取文件的属性
    struct stat st;
    int ret = stat(clientFile,&st);
    if(ret == -1)
    {
        //文件不存在
        //回复404
        sendHeaderMsg(cfd,404,"Not Found",getFileType(".html"),-1);  //-1不知道长度
        sendFile("404.html",cfd);
        return 0;
    }

    //判断文件类型 是目录还是普通文件
    if(S_ISDIR(st.st_mode))
    {
        //目录
        sendHeaderMsg(cfd,200,"OK",getFileType(".html"),-1);
        sendDir(clientFile,cfd);
    }
    else
    {
        //文件
        sendHeaderMsg(cfd,200,"OK",getFileType(clientFile),-1);
        sendFile(clientFile,cfd);
    }
    return 0;
}

//根据文件后缀，返回对应的Content-Type
const char* getFileType(const char* name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";	// 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

//发送http响应头
int sendHeaderMsg(int cfd, int status, const char* descr,const char* type, int length)
{
    //状态行
    char buf[4096] = {0};
    sprintf(buf,"http/1.1 %d %s\r\n",status,descr);

    //响应头
    sprintf(buf+strlen(buf),"content-type: %s\r\n",type);       //类型
    sprintf(buf+strlen(buf),"content-length: %d\r\n\r\n",length);   //长度 + 最后一个空行
    // sprintf(buf+strlen(buf),"\r\n");            //空行

    send(cfd,buf,strlen(buf),0);

    return 0;
}

//发送文件
int sendFile(const char* filename,int cfd)
{
    //打开文件
    int fd = open(filename,O_RDONLY);
    assert(fd > 0);     //assert是一个断言，如果条件为假，则会报错，退出任务

    //sendfile方式发送文件
    off_t offset = 0;  //发送文件的偏移量
    int size = lseek(fd,0,SEEK_END);    //返回的是文件的大小
    lseek(fd,0,SEEK_SET);  //将文件偏移量移动到头部

    while(offset < size)
    {
        int ret = sendfile(cfd,fd,&offset,size-offset);
        // printf("sendifle result:%d\n",ret);
        if(ret == -1)    //非阻塞方式发送数据
        {
            if(errno == EAGAIN)    //非阻塞方式发送数据
            {
                // printf("没有数据...\n");
                continue;
            }
            else
            {
                printf("sendfile");
                printf("errno:%d\n",errno);
                break;
            }

        }
    }

    close(fd);
    return 0;
}


/*
<html>
    <head>
        <title>test</title>
    </head>
    <body>
        <table>
            <tr>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td></td>
                <td></td>
            </tr>
        </table>
    </body>
</html>
*/
//发送目录，以html的格式发送
int sendDir(const char* dirName, int cfd)
{
    char buf[4096] = {0};
    sprintf(buf,"<html><head><title>%s</title></head><body><table>",dirName);

    //遍历目录
    struct dirent **namelist;
    int num = scandir(dirName,&namelist,NULL,alphasort);
    for(int i =0; i< num; i++)
    {
        //取出文件名
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath,"%s/%s",dirName,name);  //打包目录中的文件
        stat(subPath,&st); //获取打包文件的属性
        if(S_ISDIR(st.st_mode))
        {
            //html 标签  <a href = "xx"> name </a>
            sprintf(buf+strlen(buf),"<tr><td><a href = \"%s/\"'>%s</a></td><td>%ld</td></tr>",
            name,name,st.st_size);
        }
        else
        {
            //html 标签  <a href = "xx"> name </a>
            sprintf(buf+strlen(buf),"<tr><td><a href = \"%s/\"'>%s</a></td><td>%ld</td></tr>",
            name,name,st.st_size);
        }

        send(cfd,buf,strlen(buf),0);
        memset(buf,0,sizeof(buf));
        free(namelist[i]);
    }

    sprintf(buf,"</table></body></html>");
    send(cfd,buf,strlen(buf),0);

    free(namelist);
    return 0;
}