#ifndef EUI_POST_H
#define EUI_POST_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file eui_post.h
 * @brief 帧尾延迟执行（事件循环）：投递回调，在 eui_tick() 渲染完成后的帧尾执行。
 *
 * 用途：把「下一帧再处理」的逻辑（导航派发、app 切换、动画后置动作）交给
 * 帧循环统一调度，宿主不必在 eui_tick() 之后手工调用派发函数。
 *
 * 语义：
 *  - 投递序即执行序（FIFO）；执行点在 eui_tick() 的 Step 6（chrome 层之后）。
 *  - **排空到空**：回调内再投递的条目在同一帧内继续执行（受每帧预算限制）。
 *  - 每帧最多执行 EUI_POST_DRAIN_MAX 条，余量留到下一帧（不丢弃）。
 *  - 队列为静态容量 EUI_POST_QUEUE_SIZE；满时 eui_post() 返回 false 并计入
 *    eui_post_dropped()。
 *  - 回调内绘制只写 canvas、不 commit：本帧上屏内容仍由 Step 4/5 决定，
 *    与「宿主在 eui_tick() 之后派发」的行为一致。
 *
 * 生命周期契约：投递方保证执行时 fn 与 user_data 有效。跨帧存活的条目
 * 必须在对象销毁前 eui_post_cancel(owner) —— owner 是工具，不是自动内存管理。
 *
 * 线程模型：单线程。eui_post() 必须与 eui_tick() 同线程调用。
 */

/** 帧尾回调。user_data 为投递时传入的上下文。 */
typedef void (*eui_post_fn_t)(void *user_data);

/**
 * @brief 投递一条帧尾回调。
 *
 * @param fn        回调（NULL 时拒绝投递）
 * @param user_data 回调上下文
 * @param owner     归属标记（可为 NULL = 不可取消）；同一 owner 的条目可被
 *                  eui_post_cancel() 一次清掉
 * @return true 投递成功；false = fn 为 NULL / eui 未初始化或已 deinit / 队列满
 *         （仅「队列满」计入 eui_post_dropped()）
 */
bool eui_post(eui_post_fn_t fn, void *user_data, const void *owner);

/**
 * @brief 清掉所有未执行的、owner 匹配的条目（保持其余条目相对顺序）。
 *
 * owner 为 NULL 时是 no-op（防止误清全部匿名条目）。可在回调内安全调用。
 *
 * @param owner 归属标记
 */
void eui_post_cancel(const void *owner);

/**
 * @brief 队列中待执行条目数（诊断 / 测试用）。
 */
uint8_t eui_post_count(void);

/**
 * @brief 累计投递失败次数（仅统计队列满；跨 eui_init/eui_deinit 累计）。
 */
uint32_t eui_post_dropped(void);

/**
 * @brief 排空队列（内部钩子）：由 eui_tick() Step 6 调用，应用层不要直接调用。
 */
void eui_post_drain(void);

/**
 * @brief 清空队列（内部钩子）：由 eui_init() / eui_deinit() 调用，应用层不要直接调用。
 *
 * 被清掉的回调不会执行。
 */
void eui_post_reset(void);

#endif /* EUI_POST_H */
