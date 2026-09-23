#include "eui/eui_post.h"
#include "eui/eui_config.h"
#include "eui/eui.h"

/* 容量上限：head/tail/count 均为 uint8_t，EUI_POST_QUEUE_SIZE 超过 255 会让
 * 「队满」判断（count >= EUI_POST_QUEUE_SIZE）永远不成立，条目被静默覆盖且
 * 不计入 dropped。 */
#if EUI_POST_QUEUE_SIZE > 255
#error "EUI_POST_QUEUE_SIZE must be <= 255"
#endif

/* 预算下界：EUI_POST_DRAIN_MAX = 0 时 eui_post_drain() 一条都不执行，队列
 * 永不排空——投递方的自愈重投也会持续撞上「队列满」，导航/切换永久停摆。 */
#if EUI_POST_DRAIN_MAX < 1
#error "EUI_POST_DRAIN_MAX must be >= 1"
#endif

typedef struct {
    eui_post_fn_t fn;
    void         *user_data;
    const void   *owner;
} eui_post_entry_t;

static struct {
    eui_post_entry_t q[EUI_POST_QUEUE_SIZE];
    uint8_t  head;      /* 出队索引 */
    uint8_t  tail;      /* 入队索引 */
    uint8_t  count;
    uint32_t dropped;
} s_post;

bool eui_post(eui_post_fn_t fn, void *user_data, const void *owner)
{
    if (fn == NULL || !eui_is_running())
        return false;
    if (s_post.count >= EUI_POST_QUEUE_SIZE) {
        s_post.dropped++;
        return false;
    }
    s_post.q[s_post.tail].fn = fn;
    s_post.q[s_post.tail].user_data = user_data;
    s_post.q[s_post.tail].owner = owner;
    s_post.tail = (uint8_t)((s_post.tail + 1) % EUI_POST_QUEUE_SIZE);
    s_post.count++;
    return true;
}

void eui_post_drain(void)
{
    int budget = EUI_POST_DRAIN_MAX;
    /* 先出队再执行：回调内的 eui_post / eui_post_cancel 不会破坏本次迭代，
     * 因为每轮都重新读取 head / count（排空到空）。 */
    while (s_post.count > 0 && budget-- > 0) {
        eui_post_entry_t e = s_post.q[s_post.head];
        s_post.head = (uint8_t)((s_post.head + 1) % EUI_POST_QUEUE_SIZE);
        s_post.count--;
        e.fn(e.user_data);
    }
}

void eui_post_cancel(const void *owner)
{
    if (owner == NULL)
        return;                     /* 匿名条目不可批量取消 */

    /* 稳定压紧：读游标 r 从原 head 前进，写游标 w 只落后于 r（w <= r），
     * 因此只覆盖已读过的槽位，未读条目不会被破坏；相对顺序保持。 */
    uint8_t r = s_post.head;
    uint8_t w = s_post.head;
    uint8_t kept = 0;
    for (uint8_t i = 0; i < s_post.count; i++) {
        eui_post_entry_t e = s_post.q[r];
        r = (uint8_t)((r + 1) % EUI_POST_QUEUE_SIZE);
        if (e.owner == owner)
            continue;               /* 丢弃 */
        s_post.q[w] = e;
        w = (uint8_t)((w + 1) % EUI_POST_QUEUE_SIZE);
        kept++;
    }
    s_post.count = kept;
    s_post.tail = w;                /* head 不变：条目仍自原 head 起连续存放 */
}

uint8_t eui_post_count(void)
{
    return s_post.count;
}

uint32_t eui_post_dropped(void)
{
    return s_post.dropped;
}

void eui_post_reset(void)
{
    s_post.head = 0;
    s_post.tail = 0;
    s_post.count = 0;
}
