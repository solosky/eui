#ifndef EUI_INPUT_EDGE_H
#define EUI_INPUT_EDGE_H

#include <stdbool.h>
#include <stdint.h>
#include <eui/eui_input_drv.h>

/**
 * @file eui_input_edge.h
 * @brief Encoder + two-button edge-latch helper on top of the eui event stream.
 *
 * 面向"编码器 + OK 键 + 侧键"设备的输入状态机：把 eui 输入事件流整理成
 * 应用层一次消费的**边沿锁存**（press/release/hold/click）与虚拟编码器
 * 计数。语义对齐 M5Stack 系 Button_Class（hold 500ms、hold 触发后吞掉
 * 短按 click），并做两处扩展：OK 侧与侧键同构支持 hold；方向键
 * UP/DOWN/LEFT/RIGHT 折算为编码器计数。
 *
 * 用法：在输入回调/轮询里喂 eui_input_edge_on_event()，每帧先
 * eui_input_edge_tick(now_ms) 再查询 *_was_* 边沿；进入新视图时调用
 * eui_input_edge_reset_edges() 防幽灵边沿。
 */

#define EUI_INPUT_EDGE_DEFAULT_HOLD_MS 500u

typedef struct eui_input_edge {
    int encoder_count;                       /**< 虚拟编码器累计计数 */
    bool ok_pressed;
    bool ok_press_edge, ok_release_edge;
    uint32_t ok_press_ms;
    bool ok_hold_fired, ok_hold_edge;
    bool side_pressed;
    uint32_t side_press_ms;
    bool side_hold_fired;
    bool side_click_edge, side_hold_edge;
} eui_input_edge_t;

/**
 * @brief Initialize the edge-latch state machine (all zero).
 *
 * 零值全零结构体也是合法状态（hold 阈值是编译期常量），init 只是
 * 显式归零的便利函数。
 */
void eui_input_edge_init(eui_input_edge_t *in);

/**
 * @brief Feed one eui input event into the state machine.
 */
void eui_input_edge_on_event(eui_input_edge_t *in, const eui_event_t *evt);

/**
 * @brief Advance hold timers to @p now_ms; converts overdue presses to
 *        hold edges (and swallows the unconsumed OK press edge).
 */
void eui_input_edge_tick(eui_input_edge_t *in, uint32_t now_ms);

/**
 * @brief Clear all unconsumed edge latches (OK press/release/hold, side
 *        click/hold) and any pending "press awaiting click conversion"
 *        state. Called when re-entering a view so input that arrived
 *        while the view was inactive/opening cannot leak in as ghost
 *        edges. Keys currently held keep their pressed state and hold
 *        timer (raw state keeps being tracked, like Button_Class).
 */
void eui_input_edge_reset_edges(eui_input_edge_t *in);

int  eui_input_edge_encoder_count(const eui_input_edge_t *in);

/* 一次性边沿读取：返回当前锁存值并清零。 */
bool eui_input_edge_ok_was_pressed(eui_input_edge_t *in);
bool eui_input_edge_ok_was_released(eui_input_edge_t *in);
bool eui_input_edge_ok_is_released(const eui_input_edge_t *in);  /**< 电平查询，不清锁存 */
bool eui_input_edge_ok_was_hold(eui_input_edge_t *in);
bool eui_input_edge_side_was_clicked(eui_input_edge_t *in);
bool eui_input_edge_side_was_hold(eui_input_edge_t *in);

#endif /* EUI_INPUT_EDGE_H */
