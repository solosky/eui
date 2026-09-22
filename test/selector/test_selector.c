/*
 * test_selector.c — eui_selector（SmoothSelector 语义的动效选择器）。
 * 用例移植自 VAMeter app-eui 的同名测试。
 */
#include <assert.h>
#include <stdio.h>
#include "eui/eui_selector.h"
#include "mc_easing.h"
#include "common/eui_test.h"

static int click_count = 0, open_end_count = 0;
static void on_click(eui_selector_t *s) { (void)s; click_count++; }
static void on_open_end(eui_selector_t *s) { (void)s; open_end_count++; }

static void test_move_and_wrap(void)
{
    TEST("selector: is_changed start, goNext transition, loop wrap");
    eui_selector_t s;
    eui_selector_init(&s);
    s.on_click = on_click;
    s.on_open_end = on_open_end;
    eui_selector_set_duration(&s, 100);
    eui_selector_set_path(&s, mc_ease_back_out);

    eui_selector_option_t opts[3] = {{.x = 1, .y = 0, .w = 10, .h = 10},
                                     {.x = 50, .y = 0, .w = 10, .h = 10},
                                     {.x = 99, .y = 0, .w = 10, .h = 10}};
    for (int i = 0; i < 3; i++) assert(eui_selector_add_option(&s, &opts[i]));

    /* is_changed 初始为 true：第一次 update 把 selector 移向 option 0 */
    eui_selector_update(&s, 1000);
    eui_selector_update(&s, 1000 + 200); /* 100ms duration 已过 */
    int x, y, w, h;
    eui_selector_current_frame(&s, &x, &y, &w, &h);
    assert(x == 1 && w == 10);

    /* goNext → 移向 option 1（第一次 update 启动，第二次完成） */
    eui_selector_go_next(&s);
    assert(eui_selector_get_selected(&s) == &opts[1]);
    eui_selector_update(&s, 1400);
    eui_selector_update(&s, 1600);
    eui_selector_current_frame(&s, &x, &y, &w, &h);
    assert(x == 50);

    /* moveInLoop=true 时 goNext 回绕 */
    eui_selector_go_next(&s);
    eui_selector_go_next(&s);
    assert(eui_selector_get_selected(&s) == &opts[0]);
    PASS();
}

static void test_press_release_click_and_open(void)
{
    TEST("selector: press/release → on_click once; open → on_open_end once");
    eui_selector_t s;
    eui_selector_init(&s);
    s.on_click = on_click;
    s.on_open_end = on_open_end;
    eui_selector_set_duration(&s, 100);
    eui_selector_set_path(&s, mc_ease_back_out);

    eui_selector_option_t opts[1] = {{.x = 1, .y = 0, .w = 10, .h = 10}};
    assert(eui_selector_add_option(&s, &opts[0]));

    eui_selector_option_t sq = {.x = 2, .y = 2, .w = 5, .h = 5};
    eui_selector_press(&s, &sq);
    assert(eui_selector_is_pressing(&s));
    eui_selector_release(&s);
    assert(eui_selector_is_pressing(&s)); /* is_pressing 保持到 transition 完成 */
    eui_selector_update(&s, 1800);  /* is_changed → 回弹向 option 0 */
    eui_selector_update(&s, 2000);  /* transition 完成 → onClick */
    assert(click_count == 1);
    assert(!eui_selector_is_pressing(&s));

    /* open → 完成 → onOpenEnd */
    eui_selector_option_t full = {.x = 0, .y = 0, .w = 240, .h = 240};
    eui_selector_open(&s, &full);
    assert(eui_selector_is_opening(&s));
    eui_selector_update(&s, 2200);
    eui_selector_update(&s, 2400);
    assert(open_end_count == 1);
    PASS();
}

int main(void)
{
    eui_test_init();
    test_move_and_wrap();
    test_press_release_click_and_open();
    return eui_test_summary();
}
