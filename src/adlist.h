/* adlist.h - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */
/* 链表节点 */
typedef struct listNode {
    struct listNode *prev; /* 前置节点 */
    struct listNode *next; /* 后置节点 */
    void *value; /* 节点值 注意这里是void指针，所以可以保存所有类型的值*/
} listNode;

/* 迭代 */
typedef struct listIter {
    listNode *next;
    int direction;
} listIter;

/* 用list结构来持有链表，方便操作 */
typedef struct list {
    listNode *head; /* 链表头 */
    listNode *tail; /* 链表位 */
    void *(*dup)(void *ptr); /* 节点值复制函数 */
    void (*free)(void *ptr); /* 节点值释放函数 */
    int (*match)(void *ptr, void *key); /* 节点值对比函数 */
    unsigned long len; /* 链表长度 */
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len) /* 获取链表的长度 */
#define listFirst(l) ((l)->head) /* 获取链表头 */
#define listLast(l) ((l)->tail) /* 获取链表尾 */
#define listPrevNode(n) ((n)->prev) /* 获取链表中指定节点的前置节点 */
#define listNextNode(n) ((n)->next) /* 获取链表中指定节点的前置节点 */
#define listNodeValue(n) ((n)->value) /* 获取链表中指定节点的值 */

#define listSetDupMethod(l,m) ((l)->dup = (m)) /* 设置链表的复制函数 */
#define listSetFreeMethod(l,m) ((l)->free = (m)) /* 设置链表的释放函数 */
#define listSetMatchMethod(l,m) ((l)->match = (m)) /* 设置链表的对比函数 */

#define listGetDupMethod(l) ((l)->dup) /* 获取链表的复制函数 */
#define listGetFree(l) ((l)->free) /* 获取链表的释放函数 */
#define listGetMatchMethod(l) ((l)->match) /* 获取链表的对比函数 */

/* Prototypes */
list *listCreate(void);
void listRelease(list *list);
void listEmpty(list *list);
list *listAddNodeHead(list *list, void *value);
list *listAddNodeTail(list *list, void *value);
list *listInsertNode(list *list, listNode *old_node, void *value, int after);
void listDelNode(list *list, listNode *node);
listIter *listGetIterator(list *list, int direction);
listNode *listNext(listIter *iter);
void listReleaseIterator(listIter *iter);
list *listDup(list *orig);
listNode *listSearchKey(list *list, void *key);
listNode *listIndex(list *list, long index);
void listRewind(list *list, listIter *li);
void listRewindTail(list *list, listIter *li);
void listRotate(list *list);
void listJoin(list *l, list *o);

/* Directions for iterators */
#define AL_START_HEAD 0 /* 从表头开始迭代 */
#define AL_START_TAIL 1 /* 从表尾开始迭代 */

#endif /* __ADLIST_H__ */
