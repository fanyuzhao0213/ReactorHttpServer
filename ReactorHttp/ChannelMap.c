#include "ChannelMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct ChannelMap* ChannelMapInit(int size)
{
    struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
    map->size = size;
    map->list = (struct Channel**)malloc(sizeof(struct Channel*) * size);
    return map;
}


void channelMapClear(struct ChannelMap* map)
{
    if(map!=NULL)
    {
        for(int i=0; i<map->size;i++)
        {
            if(map->list[i] != NULL)
            {
                free(map->list[i]);
            }
        }
        free(map->list);
        map->list = NULL;
    }

    map->size = 0; 
}

bool remakeMapRoom(struct ChannelMap* map, int newsize, int unitsize)
{
    if(map->size < newsize) //判断如果当前的尺寸小于新设置的size才允许新设
    {
        int cursize = map->size;   //取出当前的size
        while(cursize < newsize)
        {
            cursize *= 2;       //容量扩大为之前的2倍，这个可以跟据项目需求更改
        }

        //扩容
        struct Channel** temp = (struct Channel**)realloc(map->list, cursize*unitsize);
        if(temp == NULL)
        {
            return false;
        }

        map->list = temp;
        memset(&map->list[map->size],0,(cursize - map->size) * unitsize);
        map->size = cursize;
    }
    return true;
}