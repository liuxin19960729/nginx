
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

/**
 * pool 内存池对象
 * n 每个链表元素数组可以容纳多少个个数
 * size 每个元素的大小
 */
 
ngx_list_t *
ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    ngx_list_t  *list;

    list = ngx_palloc(pool, sizeof(ngx_list_t));
    if (list == NULL) {// 内存分配失败
        return NULL;
    }

    if (ngx_list_init(list, pool, n, size) != NGX_OK) {// 链表初始化失败
        return NULL;
    }

    return list;
}

/**
 * null 失败
 * 返回到是节点里面元素数组开始的存储位置
 */
void *
ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;
    // list最有一个节点
    last = l->last;

    if (last->nelts == l->nalloc) {  // nelts==nalloc 节点里面的数组元素存储满了

        /* the last part is full, allocate a new list part */

        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) {// 内存分配失败 返回NULL
            return NULL;
        }
        // 分配节点里面元素数组空间
        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {// 分配内存错误
            return NULL;
        }
        // 当前节点里面数组 有0个元素
        last->nelts = 0;
        last->next = NULL;
        // 将新申请的节点插入到最后
        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;
    // 返回节点元素存储的位置 并将nelts ++ 
    return elt;
}
