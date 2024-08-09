#pragma once
#include <stdbool.h>

struct ChannelMap
{
    int size;                           //记录指针指向的数组的元素的总个数有
    struct Channel** list;              //二级指针,  可以写成struct channel* list[]
};


//channelmap 初始化
struct ChannelMap* ChannelMapInit(int size);
//清空map
void channelMapClear(struct ChannelMap* map);
//重新分配内存空间
bool remakeMapRoom(struct ChannelMap* map,int newsize,int unitsize);    //unitsize泛指指针占用的大小，可以不用这个参数，直接设置为4
