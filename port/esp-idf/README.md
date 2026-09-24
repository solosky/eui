# eui ESP-IDF port

## 这个目录提供什么 / 不提供什么

- 提供：SPI / I2C / GPIO 传输层（`eui_port_esp_idf.c`），PCNT 编码器计数
  （`driver/pulse_cnt.h` 新驱动），板级 bringup
  （`eui_port_esp_idf_board_init` + 板级描述符）
- 不提供：芯片驱动（在 `src/driver/`，平台无关）；**不提供 esp_lcd 或任何
  IDF 专有显示栈**——显示路径只经 `eui_hal_spi_t`，可移植性靠「一个芯片
  驱动 × 多种传输」保住，IDF 侧差异（DMA/CS/时钟/时序）全部关在本目录

## 接一个工程（四步）

1. `EXTRA_COMPONENT_DIRS` 加四项：eui 根、`port/esp-idf/eui_port`、
   `third_party/tlsf`、`third_party/motionc`（照抄
   `examples/esp-idf/st7789_240x240/CMakeLists.txt`）
2. Kconfig（`idf.py menuconfig` → Component config → EUI settings）定色深与
   容量。16bpp 240x240 的池需 ≥ 115200 字节（画布 w*h*2）
3. 填 `eui_port_esp_idf_board_t`（面板类型 + 引脚/时序/字节序、编码器/按键、
   fps、池指针）
4. `eui_port_esp_idf_board_init()` → 建 UI →
   `while (eui_is_running()) { eui_tick(); eui_port_esp_idf_delay_frame(); }`

可照抄的最小工程：`examples/esp-idf/st7789_240x240/`。

## 硬约束与坑

- **池必须在 DMA 可达的内部 RAM**（静态 `.bss` 或 `heap_caps_malloc(...,
  MALLOC_CAP_DMA)`）。SPI DMA 读不了 PSRAM。
- 画布一次性占 `width*height*2` 字节（16bpp FULL）。池不足时 `board_init`
  会打印「pool N < canvas M」并失败——按它加池，不要猜。
- `display->init()` 由 core 之外的地方调用（`board_init` 已代劳）；自己手搓
  装配时不要忘了它，否则面板停在初始化前。
- 字节序：`little_endian = true` 写 RAMCTL bit3（ESP32 原生 uint16 布局）。
  真机颜色反了改 false 重试——这是唯一需要上机确认的参数。
- CS：`hw_cs = true` 交给 SPI 外设按事务拉低（一次 RAMWR 连续写的要求）；
  `false` 则由 HAL 手工控制、`board_init` 把它配成输出。
- 帧率上限 = SPI 时钟 / (w*h*2*8)：240x240@16bpp 在 80MHz 约 87fps 上限，
  40MHz 约 43fps。`delay_frame` 按 `fps_target` 补偿节拍，不追帧。
- 多输入设备（编码器 + 按键）用 `eui_input_mux` 合成（`board_init` 已自动
  做）；编码器旋转走 `eui_hal_encoder_t`（PCNT 绝对计数，不丢步），编码器
  **按键**单独走 `eui_drv_buttons`。
- 传输句柄生命周期：芯片驱动按值持有 HAL 副本，transport 必须活得比驱动
  久；`board_deinit` 已按 driver → transport 反序释放。
- `port/esp-idf/include-standalone/freertos/FreeRTOSConfig.h` 是宿主构建用的
  假配置头，**绝不能出现在组件导出路径上**（会遮蔽 IDF 真配置；
  `examples/cross/esp_idf_build/main/eui_idf_config_asserts.c` 的静态断言
  守着这条）。
- I2C 仍用 legacy 驱动（`driver/i2c.h`）：IDF 5.1.5 没有 `i2c_master` 新 API
  （5.2 引入），工程升 IDF ≥ 5.2 时迁移（见 `eui_port_esp_idf.c` 内注释；
  legacy 驱动 6.x 移除，届时必须迁）。

## 真机已知待验项

- `little_endian` 取 true 还是 false（颜色对不对）
- PCNT half-quad 的计数倍率（一格是否正好 = 一次增量；当前按 2 计数/格）
- 实测帧率与 SPI 时钟/驱动匹配度（理论值见上）
