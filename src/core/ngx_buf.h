
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;



typedef struct ngx_buf_s  ngx_buf_t;

/**
 * ngx_buf_s
 * Nginx 用于处理大数据关键数据结构 
 * 
 * 内存 pos->last 
 * 文件 file_pos->file_last
 * 
 * 核心设计
 * 零拷贝是想 
 *      不会申请一个大buffer 将 内存 和 文件里面的内容复制到大buf 
 */

struct ngx_buf_s {

    //1.[pos,last) 内存里面的数据 (note: 不包含last指向的数据)
    u_char          *pos;
    u_char          *last;

    // 2 [file_pos,file_last] 文件 开始到结束的偏移量
    off_t            file_pos;
    off_t            file_last;

    // 3.内存缓冲去物理边界(用于内存复用) 
    u_char          *start;         /* start of buffer */
    u_char          *end;           /* end of buffer */

    // 4.标识 note:通常是模块的loc_conf 执指针 用于区分缓冲去是由那个模块分配的
    ngx_buf_tag_t    tag;
    
    // 5.文件对象(如果数据在文件中)
    ngx_file_t      *file;
    // 6.影子缓冲区 通常用于多个buf 指向同一块内存 防止重复释放
    ngx_buf_t       *shadow;

    // 7.标志位
    /* the buf's content could be changed  数据在临时内存区域(处理完可以修改复用)*/
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed 数据在只读区域不能修改
     */
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed  数据通过mmap 映射到内存*/
    unsigned         mmap:1;
    // 缓冲区可回收（处理完后不释放，而是放入 free_bufs 链表复用）
    unsigned         recycled:1;
    //数据在文件中（需配合 file_pos/file_last 使用）
    unsigned         in_file:1;
    // 刷新标志。告诉过滤器链，即使数据不足也要立刻输出 例如 在长连接或流式响应中，强制把当前积压的数据发给客户端
    unsigned         flush:1;
    // 同步标志。无实际数据，仅作为控制信号（类似 flush，但更轻量）
    unsigned         sync:1;
    // 这是最后一块缓冲区（整个请求/响应结束）  例如 响应完全结束。HTTP Filter 看到它会计算 Content-Length 或发送 Chunked 结束块 (0\r\n\r\n) sync	同步占位	不含数据，仅为了刷新管道或传递控制信号
    unsigned         last_buf:1;
    //  这是当前 chain 的最后一块（但不是整个请求结束）
    unsigned         last_in_chain:1;
    //当前 buf 是 shadow 链的最后一个
    unsigned         last_shadow:1;
    /// 1: 数据在临时文件中
    unsigned         temp_file:1;

    /* STUB */ int   num;
};


struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;//如果是最后一个 ngx_chain_t 则 next 必须是NULL
};

/**
 * note:(Nginx框架要求)
 * 最后一个 ngx_chain_s next 不为NULL 则这个请求一直不会结束
 */


 
typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
    unsigned                     unaligned:1;
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
    unsigned                     aio:1;

#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
    ngx_output_chain_aio_pt      aio_handler;
#endif

#if (NGX_THREADS || NGX_COMPAT)
    ngx_int_t                  (*thread_handler)(ngx_thread_task_t *task,
                                                 ngx_file_t *file);
    ngx_thread_task_t           *thread_task;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)       ((b)->temporary || (b)->memory || (b)->mmap)
#define ngx_buf_in_memory_only(b)  (ngx_buf_in_memory(b) && !(b)->in_file)

#define ngx_buf_special(b)                                                   \
    (((b)->flush || (b)->last_buf || (b)->sync)                              \
     && !ngx_buf_in_memory(b) && !(b)->in_file)

#define ngx_buf_sync_only(b)                                                 \
    ((b)->sync && !ngx_buf_in_memory(b)                                      \
     && !(b)->in_file && !(b)->flush && !(b)->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) ((b)->last - (b)->pos):                  \
                            ((b)->file_last - (b)->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    (cl)->next = (pool)->chain;                                              \
    (pool)->chain = (cl)



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);

off_t ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit);

ngx_chain_t *ngx_chain_update_sent(ngx_chain_t *in, off_t sent);

#endif /* _NGX_BUF_H_INCLUDED_ */
