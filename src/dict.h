/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
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

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0 /* 字典操作成功的返回值 */
#define DICT_ERR 1 /* 字典操作失败的返回值 */

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

/* 哈希表的节点，存储键值对的结构 */
typedef struct dictEntry {
    void *key; /* key用于存储键的指针 */
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v; /* v用于保存值，可以是具体值或者值的指针 */
    struct dictEntry *next; /* next指向链表下一个节点 */
} dictEntry;

/* 字典的类型结构 */
typedef struct dictType {
    uint64_t (*hashFunction)(const void *key); /* 哈希函数 */
    void *(*keyDup)(void *privdata, const void *key); /* 键拷贝 */
    void *(*valDup)(void *privdata, const void *obj); /* 值拷贝 */
    int (*keyCompare)(void *privdata, const void *key1, const void *key2); /* 键比较 */
    void (*keyDestructor)(void *privdata, void *key); /* 键析构 */
    void (*valDestructor)(void *privdata, void *obj); /* 值析构 */
} dictType;

/* 哈希表结构 */
/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct dictht {
    dictEntry **table; /* 保存键值对的数组 */
    unsigned long size; /* 记录哈希表大小，也就是table数组大小 */
    unsigned long sizemask; /* 大小为size-1, 与size配合决定键应该存在哪个表中 */
    unsigned long used; /* 记录当前已有的键值对的数量 */
} dictht;

/* 字典结构 type和privdata是针对不同类型的键值对，redis会根据不同的字典用途设置不通过的type
 * 函数，privadata保存了要传给这些函数的可选参数
 */
typedef struct dict {
    dictType *type; /* 各种处理函数 */
    void *privdata; /* 私有数据 */
    dictht ht[2]; /* 两个哈希表 */
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */ /* rehash索引 */
    unsigned long iterators; /* number of iterators currently running */ /* 迭代 */
} dict;

/* 字典的迭代器 */
/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
typedef struct dictIterator {
    dict *d;
    long index;
    int table, safe;
    dictEntry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    long long fingerprint;
} dictIterator;

typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* 哈希表的初始大小 */
/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* ------------------------------- Macros ------------------------------------*/
/* 使用指定的析构函数释放指定实体的值 */
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

/* 使用指定的值复制函数，给指定实体设置给定的值，否则用给定值直接赋值。 */
#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)

/* 给实体设置有符号整数 */
#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)

/* 给实体设置无符号整数 */
#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)

/* 给实体设置有浮点数 */
#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)

/* 使用指定的键析构函数释放实体的键 */
#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

/* 使用字典的键复制函数，给指定实体的键赋值，否则直接赋值 */
#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)

/* 使用字典的键比较函数，比较两个键值，否则直接进行对比 */
#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

#define dictHashKey(d, key) (d)->type->hashFunction(key) /* 使用哈希函数，获取指定键对应的哈希值 */
#define dictGetKey(he) ((he)->key) /* 获取一个哈希实体的键 */
#define dictGetVal(he) ((he)->v.val) /* 获取一个哈希实体的值 */
#define dictGetSignedIntegerVal(he) ((he)->v.s64) /* 获取哈希实体的有符号的整数值 */
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64) /* 获取哈希实体的无符号整数值 */
#define dictGetDoubleVal(he) ((he)->v.d) /* 获取哈希实体的浮点数值 */
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size) /* 获取整个字典的存储大小 */
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used) /* 获取整个字典已使用的存储大小 */
#define dictIsRehashing(d) ((d)->rehashidx != -1) /* 判断字典是不是在rehash过程中 */

/* API */
dict *dictCreate(dictType *type, void *privDataPtr);
int dictExpand(dict *d, unsigned long size);
int dictAdd(dict *d, void *key, void *val);
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);
dictEntry *dictAddOrFind(dict *d, void *key);
int dictReplace(dict *d, void *key, void *val);
int dictDelete(dict *d, const void *key);
dictEntry *dictUnlink(dict *ht, const void *key);
void dictFreeUnlinkedEntry(dict *d, dictEntry *he);
void dictRelease(dict *d);
dictEntry * dictFind(dict *d, const void *key);
void *dictFetchValue(dict *d, const void *key);
int dictResize(dict *d);
dictIterator *dictGetIterator(dict *d);
dictIterator *dictGetSafeIterator(dict *d);
dictEntry *dictNext(dictIterator *iter);
void dictReleaseIterator(dictIterator *iter);
dictEntry *dictGetRandomKey(dict *d);
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
void dictGetStats(char *buf, size_t bufsize, dict *d);
uint64_t dictGenHashFunction(const void *key, int len);
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);
void dictEmpty(dict *d, void(callback)(void*));
void dictEnableResize(void);
void dictDisableResize(void);
int dictRehash(dict *d, int n);
int dictRehashMilliseconds(dict *d, int ms);
void dictSetHashFunctionSeed(uint8_t *seed);
uint8_t *dictGetHashFunctionSeed(void);
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);
uint64_t dictGetHash(dict *d, const void *key);
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
