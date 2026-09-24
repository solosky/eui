# eui on ESP32-S3 + ST7789 240x240

最小可跑工程，也是 brick 契约的可执行说明书（板级参数全部集中在 `main.c`）。

## 构建

    . $IDF_PATH/export.sh
    idf.py set-target esp32s3
    idf.py build
    idf.py -p /dev/ttyACM0 flash monitor

## 这份工程演示了什么

- 板级参数集中在 `main.c` 顶部：引脚、面板时序、字节序、编码器/按键、fps、内存池
- `eui_port_esp_idf_board_init()` 内部固定七步顺序：
  池 → transport → driver → `eui_init` → tick 回调 → `display->init()` → 可用
- `eui_port_esp_idf_delay_frame()` 按 `fps_target` 与上一帧实际耗时做补偿节拍
- 16bpp FULL 画布需要 240*240*2 = 115200 字节；池必须 DMA 可达的内部 RAM
  （SPI DMA 读不了 PSRAM）
- bringup 会打印池的实际 used/peak，池容量按它校准

## 配置

Kconfig（`idf.py menuconfig` → Component config → EUI settings）暴露色深与
容量；本工程的 `sdkconfig.defaults` 已把 ST7789@16bpp 需要的值填好
（COLOR_DEPTH=16、MEM_POOL_SIZE=196608 = 画布 + ~80KB 余量）。

## 已知边界

- 帧率上限由 SPI 时钟决定：一帧 115200 字节，80MHz 理论 11.5ms（≈87fps
  上限），40MHz 约 23ms。本工程取 80MHz + 30fps 留余量。
- 面板字节序用 `little_endian = true`（写 RAMCTL 的 bit3）。若真机颜色反了，
  改成 false 重试——这是唯一需要上机确认的参数。
- 背光（PIN_BL）只是记录在描述符里，PWM 由 brick 自己配（eui 不管 LEDC）。
- 编码器计数倍率：PCNT half-quad 为 2 计数/格；若实测一格两次增量，在
  brick 侧把差值减半。
