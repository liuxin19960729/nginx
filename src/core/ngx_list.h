
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

// ngx_list_part_t 链表的一个元素(节点)  （实际上是一个固定大小的数组块）
typedef struct ngx_list_part_s  ngx_list_part_t;

// 链表元素 这样设计非常灵活 可以是任何一种数据结构
struct ngx_list_part_s {
    void             *elts;// 指向该节点的数据区（数组首地址）
    ngx_uint_t        nelts;//该节点已使用的元素个数
    ngx_list_part_t  *next;// 下一个链表节点
};


typedef struct {
    ngx_list_part_t  *last;// 指向链表的最后一个元素
    ngx_list_part_t   part;// 链表的第一个节点（直接内嵌在头里，减少一次分配）
    size_t            size;// 单个元素的大小 ngx_list_part_s.elts 占用大小
    ngx_uint_t        nalloc;// 每个节点（数组块）能容纳的元素总数
    ngx_pool_t       *pool;// 内存池（用于分配新节点）
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    //链表第一个节点里面 elts 申请内存
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }
    //数组存储了0个元素
    list->part.nelts = 0;
    // 下一个节点指向位置
    list->part.next = NULL;
    // 当前既是第一个元素也是最后一个 设置最后一个位置
    list->last = &list->part;
    //设置每个节点上数组存储元素占用大小
    list->size = size;
    // 每个节点存储元素个数
    list->nalloc = n;
    // 内存池指针
    list->pool = pool;

    return NGX_OK;
}


/*
 *  遍历列表
 *  the iteration through the list:
 *
 *  part = &list.part;// 第一个节点地址
 *  data = part->elts;// 第一个节点数组存在几个元素
 *
 *  for (i = 0 ;; i++) {
 *      // 如果i 超过了节点里面存储元素个数 
 *      // 设置下一个节点状态
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;// 不存在下一个节点退出循环
 *          }
 *          
 *          part = part->next;// 下一个节点
 *          data = part->elts;//下一个节点元素数组的位置
 *          i = 0;// 重置节点数组的索引
 *      }
 *      
 *      // 获取遍历的值
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
