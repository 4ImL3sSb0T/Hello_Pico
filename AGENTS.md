# Hello_Pico

正点原子 ATK-DNRP2350AM（RP2350A 小系统板）固件。CMake + Pico SDK，当前 demo：简易电子负载（GPIO4 150 kHz PWM + GPIO26 ADC0，KEY0 调占空比，上限 20%）。

## Hardware

- MCU：RP2350A，CMake 板型 `pico2`，平台 `rp2350-arm-s`
- SDK：Pico SDK **2.3.0**（`~/.pico-sdk/sdk/2.3.0`），工具链 `15_2_Rel1`，CMake `v4.3.4`，Ninja `v1.13.2`，picotool `2.3.0`
- 屏：ST7789 1.14"，逻辑分辨率横屏 **240×135**（`lcd_display_dir(1)`），颜色 RGB565
- stdio：USB CDC（`pico_enable_stdio_usb=1`，UART 关）
- 调试：SWD / CMSIS-DAP（OpenOCD）

### 已占用引脚

| GPIO | 功能 | 极性 / 备注 |
|------|------|-------------|
| 2 | KEY0 | 按下为低，内部上拉。原理图/丝印也叫 KEY1 |
| 3 | LED | `LED(0)` 亮，`LED(1)` 灭 |
| 4 | PWM 电子负载 | slice2A，约 150 kHz，占空比硬件钳位 ≤20%，上电 0% |
| 8 | LCD_DC | |
| 9 | LCD_CS | |
| 10 | SPI1 SCK | 丝印/分配表标 SDIO_SCK，现给 LCD |
| 11 | SPI1 MOSI | 丝印标 SDIO_CMD |
| 12 | SPI1 MISO | 丝印标 SDIO_D0 |
| 25 | LCD_BL | **低电平亮**（Q2 S8550 开 LEDA） |
| 26 | ADC0 负载采样 | 片内 12-bit SAR，Vref=3.3 V。超过 3.3 V 必须外部分压 |
| 29 | ADC3 | 板上已接 5V 分压监测，不要当负载采样脚 |

GPIO0/1 接板载 CH343 的 UART0。GPIO10–15 在分配表上是 SD 卡；LCD 已占用 10–12，接 SD 前先确认复用。未引出：片上 QSPI Flash、12 MHz 晶振、USB PHY。

引脚权威来源：`docs/RaspberryPi-RP2350A小系统板IO引脚分配表.xlsx`，板卡说明：`docs/ATK-DNRP2350AM_V1.0.PDF`。改引脚先对这两份，再改宏。

SDK 2.3.0 未修问题与规避：`docs/pico-sdk-2.3.0-bugs.md`。动 sleep / USB / DMA / PIO / GPIO 下拉 / TinyUSB 前先读，不要把已有规避改回去。

## Layout

```
Hello_Pico.c          # 入口 + async 主循环
blink.pio             # VS Code Pico 模板残留，已生成头文件，应用未用
src/bsp/led/          # GPIO3
src/bsp/lcd/          # ST7789 + SPI1 + DMA + 帧缓冲，无图形原语
src/bsp/input/        # KEY0 + MultiButton
src/bsp/pwm/          # GPIO4 高频 PWM，占空比钳位 20%
src/bsp/adc/          # GPIO26 / ADC0
src/bsp/flash/        # W25Q32 空壳，CMake 已排除
src/app/eload/        # 电子负载 demo（占空比、采样、绘制）
src/service/hagl_hal/ # HAGL 接到 lcd_fb / lcd_flush
third_party/hagl/     # 图形库（圆、矩形、字、blit）
docs/                 # 硬件手册 + SDK 2.3.0 已知问题（pico-sdk-2.3.0-bugs.md）
build/                # 本地构建，已 gitignore
```

新外设放 `src/bsp/<name>/`。可复用、与具体 demo 无关的逻辑放 `src/service/`。把 `Hello_Pico.c` 里的 demo 往外抽时放 `src/app/`，不要再往 BSP 里堆应用状态。

`src/**/*.c` 由 CMake `GLOB_RECURSE` 自动编入。排除目录用 `list(FILTER ... EXCLUDE REGEX)`，与 `flash` 相同。

## Runtime

主循环是 **`pico_async_context_poll`**，不是 RTOS，也不是 `sleep_ms` 空转：

1. `stdio_init_all` → `led_init` → `spi1_init` → `lcd_init` → `hagl_init` → `key_init` → `pwm_out_init` → `adc_in_init` → `eload_init`
2. `key_bind(&loop.core)`，再 `eload_bind_keys`
3. `display_worker` 按 60Hz 相位重挂（超时不追帧）；帧内 `eload_sample` + `eload_draw`
4. `stats_worker` 每秒算 FPS 并 `LED_TOGGLE`
5. `while (true) { poll; 若 next 未到则 busy_wait_until(next); }`
   （不用 `wait_for_work_until`，原因见 `docs/pico-sdk-2.3.0-bugs.md`）

Worker 里禁止长时间阻塞。定时用 `async_at_time_worker`，相位对齐抄 `display_work` / `key_tick_work`。

### LCD

分层：应用只调 `hagl_*` → `src/service/hagl_hal` 写 `lcd_fb()` → `src/bsp/lcd` 负责 SPI/DMA/flush。

- `lcd/` 只提供设备与帧缓冲：`lcd_init` / `lcd_fb` / `lcd_fb_lock` / `lcd_flush` / `lcd_flush_rect`
- 不要在 `lcd/` 里加画圆、画线、字库或其它图形原语
- 除 `hagl_hal` 和 `lcd_*` 自己外，不要写 `lcd_fb()`
- 单缓冲：`flush` 启动 DMA 后立刻返回；HAL 每帧第一次下笔会 `lcd_fb_lock()`
- 不要在 IRQ 或 DMA 进行中改帧缓冲
- 总线细节在 `lcd_priv.h` / `lcd_bus.c`，应用绘制不要包含它们
- SPI1 62.5 MHz，8bit 命令 / 16bit 像素 DMA；RX DMA 只为抽空 FIFO
- 240×135 可见区嵌在 ST7789 240×320 GRAM，偏移在 `lcd.c` 的 `lcd_orient[]`，不要随便改 `colstart`/`rowstart`/`MADCTL`

### KEY

- MultiButton 必须 **持续 5ms 打拍**（`TICKS_INTERVAL`），空闲也不能停，消抖发生在 IDLE
- 对外 API：`key_init` → `key_bind` → `key_attach`；读原始脚用 `key_raw_level`
- 不要绕过 `key.*` 去直接 `button_*`，除非在改 MultiButton 本身

## Build

Windows 环境（与 `.vscode/settings.json` / workspace 一致）：

```
PICO_SDK_PATH=%USERPROFILE%/.pico-sdk/sdk/2.3.0
PICO_TOOLCHAIN_PATH=%USERPROFILE%/.pico-sdk/toolchain/15_2_Rel1
PATH += toolchain/15_2_Rel1/bin
        picotool/2.3.0/picotool
        cmake/v4.3.4/bin
        ninja/v1.13.2
```

```powershell
cmake -S . -B build -G Ninja -DPICO_BOARD=pico2
ninja -C build
```

产物：`build/Hello_Pico.uf2` / `.elf` / `.hex`。

下载：

```powershell
picotool load build/Hello_Pico.uf2 -fx
```

或 VS Code 任务 `Run Project` / `Flash`（后者走 OpenOCD + CMSIS-DAP）。

改完 C / CMake 用 `ninja -C build` 确认能链过。不要提交 `build/`。

`CMakeLists.txt` 里 Pico VS Code 扩展那一段（`DO NOT EDIT`）不要动。需要的库加在 `target_link_libraries`：现已链 `pico_stdlib`、`pico_async_context_poll`、`hardware_{spi,i2c,dma,pio,interp,timer,watchdog,clocks,pwm,adc}`。

## Conventions

- 语言：C11（`CMAKE_C_STANDARD 11`），新代码不要上 C++，除非用户明确要求
- 缩进 4 空格；函数体 `{` 另起一行；`if`/`for`/`while` 的 `{` 跟在同一行
- 新文件头用短注释（`@file` + 一两句约束），不要复制正点原子那种整页 banner
- 注释写非显而易见的约束（极性、时序、DMA、引脚复用），中文即可
- include：应用/其他模块用 `"bsp/<mod>/<file>.h"`；LCD 内部才用 `lcd_priv.h`
- 类型 `snake_case` + `_t`；宏全大写；GPIO 宏集中在对应 BSP 头文件
- 颜色用应用本地 RGB565 或 `hagl_color()`，不要把调色板放进 `lcd.h`
- 第三方 `multi_button.*` 尽量少改；板级差异放 `key.c`

## Don't

- 不要把 `src/bsp/flash/` 编进工程，除非已经按本板引脚把 W25Q32 真正移植完
- 不要在 `display_work` 里再 `lcd_init` / 重配 SPI
- 不要把 demo 状态（占空比、ADC 读数、FPS）塞进 LCD/KEY 驱动；PWM BSP 只允许做 20% 钳位
- 不要把 PWM 占空比上限改到超过 20%，除非已经按负载功率和 MOSFET 散热重新算过
- 不要用 GPIO29 当负载 ADC（板上已接 5V 分压监测）
- 不要绕过 HAGL 去调 `lcd_fb()` 画图
- 不要假设 GPIO10–15 空闲
- 不要改 `pico_sdk_import.cmake`（SDK 原样拷贝）
- 不要违反 `docs/pico-sdk-2.3.0-bugs.md` 里的规避
- 没有单元测试；验证手段是编译 + 板子上的 USB 串口 `printf` 和屏显

## Git

远程：`https://github.com/4ImL3sSb0T/Hello_Pico.git`，默认分支 `master`。提交说明沿用现有风格：中文、写清改了什么行为（例如按键异步、DMA flush），不要空的 “update”。
