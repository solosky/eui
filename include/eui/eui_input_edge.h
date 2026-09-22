#ifndef EUI_INPUT_EDGE_H
#define EUI_INPUT_EDGE_H

#include <stdbool.h>
#include <stdint.h>
#include <eui/eui_input_drv.h>
#include <eui/eui_event.h>
#include <eui/eui_config.h>

/**
 * @file eui_input_edge.h
 * @brief 手势装配器：原始输入事件流 → 通用手势事件（push 管线）。
 *
 * eui core 持有唯一实例（内嵌于 view dispatcher）。eui_tick 把 input
 * manager 的去抖原始事件喂入，推进 hold 计时，从内部队列 pop 出手势
 * 事件路由给 dispatcher。
 *
 * key_id 为无语义 uint8 编号（分配归项目）；对每个 key_id 一视同仁
 * 装配。语义对齐 M5Stack Button_Class：hold 500ms；hold 触发吞掉本次
 * click（click = press→release 且未触发 hold）；press/release 原样透传
 * （选择器类交互需要 press/release 时序）。编码器 CW/CCW 组装为
 * ENC_STEP；方向键折算编码器归驱动层键位绑定。flush() 供视图切换时
 * 清除未派发事件与瞬时边沿（按住键的电平与 hold 计时保留）。
 */

#define EUI_INPUT_EDGE_DEFAULT_HOLD_MS 500u

typedef struct {
    bool     pressed;
    bool     armed;        /**< press 后未触发 hold：release 可成 click */
    bool     hold_fired;
    uint32_t press_ms;
    /* 一次性边沿锁存（拉取式查询；app 走事件，仅供 eui 单测/兼容） */
    bool press_edge, release_edge, click_edge, hold_edge;
} eui_input_edge_key_t;

typedef struct {
    eui_input_edge_key_t key[EUI_KEY_ID_MAX];
    eui_event_queue_t    queue;   /**< 组装好的手势事件 */
} eui_input_edge_t;

/**
 * @brief Initialize the gesture assembler (all zero).
 */
void eui_input_edge_init(eui_input_edge_t *in);

/** 喂一路原始事件（PRESS/RELEASE/ENC_CW/CCW/ENC_CLICK/TOUCH/REPEAT）。 */
void eui_input_edge_on_event(eui_input_edge_t *in, const eui_event_t *raw);

/** 推进 hold 计时，到期按键产出 KEY_HOLD（eui_tick 每帧调用）。 */
void eui_input_edge_tick(eui_input_edge_t *in, uint32_t now_ms);

/** pop 一条组装完成的手势事件；队列空返回 false。 */
bool eui_input_edge_pop_event(eui_input_edge_t *in, eui_event_t *out);

/** 清空未派发事件 + 瞬时边沿；按住键的电平/hold 计时保留。 */
void eui_input_edge_flush(eui_input_edge_t *in);

/* 拉取式一次性边沿（消费即清零；key_id 越界返回 false） */
bool eui_input_edge_was_pressed (eui_input_edge_t *in, uint8_t key_id);
bool eui_input_edge_was_released(eui_input_edge_t *in, uint8_t key_id);
bool eui_input_edge_was_clicked (eui_input_edge_t *in, uint8_t key_id);
bool eui_input_edge_was_hold    (eui_input_edge_t *in, uint8_t key_id);
/** 电平查询，不清锁存；key_id 越界视作已释放。 */
bool eui_input_edge_is_released (const eui_input_edge_t *in, uint8_t key_id);

#endif /* EUI_INPUT_EDGE_H */
