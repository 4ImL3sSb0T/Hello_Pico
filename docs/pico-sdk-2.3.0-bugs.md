# Pico SDK 2.3.0 已知问题（本仓库必读）

整理日期：2026-08-15。SDK 版本锁定 **2.3.0**（2026-07-03，`98a542c`）。2.3.1 尚未发布。

权威来源：

- 发行说明：<https://github.com/raspberrypi/pico-sdk/releases/tag/2.3.0>
- 仍开着的 issue：<https://github.com/raspberrypi/pico-sdk/issues>
- 2.3.1 milestone：<https://github.com/raspberrypi/pico-sdk/milestone/26>
- 硅 errata：RP2350 datasheet Appendix B

本文只记 **2.3.0 里还在、写代码会踩到的东西**。2.3.0 已经修掉的旧 bug（alarm 取消不清 pending、`pio_encode_mov(pio_exec)` 那一版、`rom_reboot(0)` 等）不重复罗列。本板是 RP2350A / `pico2` / USB CDC / SPI1 DMA / `async_context_poll`，下面按对本工程的影响排序。

---

## 开发时必须遵守

1. 主循环不要用 `async_context_wait_for_work_until()`，不要用 `sleep_until()` / `sleep_us()` / `sleep_ms()` 做周期等待。短延时用 `busy_wait_us()` / `busy_wait_ms()` / `busy_wait_until()`。
2. 不要手写 `tud_task(); __wfe();`，不要把 TinyUSB 从 IRQ 后台改成 poll 模式。
3. 不要用 `dma_channel_abort()` 裸调用。LCD 走 `lcd_dma_abort_one()`（先 `DREQ_FORCE`、清 EN、再 abort、超时退出）。
4. GPIO 输入用内部上拉可以；**不要开内部下拉**（RP2350-E9）。需要下拉就外接 ≤8.2 kΩ。
5. 不要用 `pio_encode_mov(pio_osr, …)` / `pio_encode_mov(pio_exec, …)` 现场拼指令，用 pioasm 或手写立即数。
6. USB 复位相关宏用 `PICO_USB_RESET_*`，不要再用 `PICO_STDIO_USB_RESET_*`。
7. 共享 IRQ 只在一个核上挂/拆；不要双核同时 enable 同一条 IRQ。
8. 不要设 `PICO_DEBUG_INFO_IN_RELEASE=0`，除非先核对 RAM。不要擅自换 TinyUSB 版本。
9. 不要开 `SCB_CCR_UNALIGN_TRP` 再对未对齐地址 `memset`/`memcpy`。
10. 升 2.3.1 之前不要把已经写好的规避改回去。

细节与出处见下文。

---

## 对本板高风险（会卡死 / 丢帧 / 指令错）

### 1. `sleep_until()` / `sleep_us()` 在 RP2350 上可能永远不醒

- Issue：[#3078](https://github.com/raspberrypi/pico-sdk/issues/3078)，milestone 2.3.1，仍开着。
- 现象：连续几次短 `sleep_us()` 后卡在 `sleep_until()` 的 `__wfe()`。硬件 alarm **未 armed**、中断未 pending、timer 仍在走。RP2040 同代码不复现。`busy_wait_us()` 可避开。
- 本仓库：`Hello_Pico.c` 主循环已经改成 `busy_wait_until()`，注释写了原因。
- 仍有风险：`lcd_init()` / `lcd_on()` / `lcd_off()` 里还有 `sleep_ms(10/120)`。上电阶段偶发卡死时，改成 `busy_wait_ms()`。
- 禁止：在 worker 或主循环里重新引入 `sleep_ms` / `sleep_until` / `async_context_wait_for_work_until`。`wait_for_work_until` 内部也走同一条 WFE + alarm 路径。

### 2. `best_effort_wfe_or_timeout()` 同一 deadline 会永远等

- Issue：[#3124](https://github.com/raspberrypi/pico-sdk/issues/3124)，milestone 2.3.1，仍开着。2.3.0 和当前 develop 都能复现。
- 现象：USB CDC 收到一个字符后，用**同一个** `absolute_time_t` 反复调用 `best_effort_wfe_or_timeout()` 会不再超时。USB 代码会取消并改写更早的 alarm；函数里的 `last_added` 以为自己的 timer 还在，直接 `__wfe()` 且不再重挂。再来一个字符又好，再来一个又卡，确定性翻转。
- 规避：每次换新的 timeout；或不用这条 API，改 `busy_wait_until`（本仓库主循环已如此）。
- 禁止：`while (!cond) best_effort_wfe_or_timeout(same_deadline);`

### 3. TinyUSB 仍是 0.18.x；poll 模式当串口基本不可用

- TinyUSB 升级到 0.20.0 仍开着：[#2756](https://github.com/raspberrypi/pico-sdk/issues/2756)，milestone 2.3.1。2.3.0 **没有**跟着升。
- 论坛（[SDK 2.3.0 is released](https://forums.raspberrypi.com/viewtopic.php?t=399421)）：从 IRQ 后台改成 poll 模式后，TinyUSB 当 CDC 串口几乎不可用。维护者说修会进 2.3.1。
- `tud_task()` 在 RP2350 上会因 spinlock 隐式 SEV，后面的 `__wfe()` 立刻醒，低功耗环变成忙等（[#2495](https://github.com/raspberrypi/pico-sdk/issues/2495)）。2.3.0 的 `PICO_SYNC_RP2350_SPINLOCK_WORKAROUND` 修的是 sleep/同步原语，**不**保证 `tud_task()+WFE` 这种写法。
- 本仓库：`pico_enable_stdio_usb=1`，保持 SDK 默认 IRQ 后台。不要改 `PICO_STDIO_USB` 的 poll 路径，不要手写 TinyUSB 主循环。
- FreeRTOS 且把 TinyUSB 钉到 core1 时，`printf` 可能没输出但 USB 还活着（[#2803](https://github.com/raspberrypi/pico-sdk/issues/2803)）。本仓库不用 FreeRTOS，但以后上双核 USB 要避开。

### 4. `pio_encode_mov` 在 2.3.0 仍然编错

- Issue：[#3068](https://github.com/raspberrypi/pico-sdk/issues/3068)。2.3.0 修了 `pio_pindirs`，但把 `pio_osr` / `pio_exec` 编坏了。
- RP2350 + 2.3.0 实测：

  | 调用 | 得到 | 应为 |
  |------|------|------|
  | `pio_encode_mov(pio_osr, pio_x)` | `0xA061`（实际是 mov pindirs） | `0xA0E1` |
  | `pio_encode_mov(pio_exec, pio_x)` | `0xA061` | `0xA081` |
  | `pio_encode_mov(pio_exec_mov, pio_x)` | `0xA061` | `0xA081` |

- 2.2.0 则是 `pio_pindirs` 错、`pio_osr` 对。不要假设「新 SDK 更正确」。
- 本仓库：`blink.pio` 未用。以后动态拼 PIO 用 pioasm，或对 `osr`/`exec` 写死立即数。不要用 `pio_encode_mov` 编这两档。develop 已合修复，等 2.3.1。

### 5. `dma_channel_abort()` 不等 `CHAN_ABORT` 清零

- Issue：[#923](https://github.com/raspberrypi/pico-sdk/issues/923)，milestone 2.4.0。手册要求写 abort 后必须等到 `CHAN_ABORT` 全 0 才能重启通道；SDK 只等 `CTRL.BUSY`。
- 硅：RP2350-E5，链式 DMA 必须先清 `EN` 再 abort，否则可能重触发。
- 本仓库：`lcd_dma_abort_one()` 已：改 `DREQ_FORCE`（DREQ 不来时 BUSY 永远不掉）→ 清 EN（E5）→ 写 abort → 超时等 BUSY。**仍未**轮询 `dma_hw->abort`。
- 禁止：别处直接 `dma_channel_abort()` 完立刻 `dma_channel_configure` / start。新 DMA 代码抄 LCD 这条，并补等 `CHAN_ABORT`。

---

## 对本板中风险（引脚 / 宏 / 内存 / 构建）

### 6. RP2350-E9：内部下拉或高阻输入可能锁在 ~2.2 V

硅缺陷，SDK 不会默默修。输入使能打开后，弱下拉（内部或偏大外接下拉）第一次被拉高，可能停在约 2.2 V，数字读永远是 1。PIO 不能用「读前开 IE、读后关 IE」。

- 本板 KEY0（GPIO2）是**按下为低 + 内部上拉**，不受 E9 下拉路径影响。
- 禁止：`gpio_pull_down()` 当按键/检测。需要下拉就外接 **≤8.2 kΩ**。模拟输入先关对应 GPIO IE。
- 新输入脚先确认极性：优先上拉+低有效。

### 7. USB 复位宏改名，旧名没有反向兼容

- Issue：[#3060](https://github.com/raspberrypi/pico-sdk/issues/3060)。`pico_usb_reset` 从 `pico_stdio_usb` 拆出。
- `PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE` 等已改成 `PICO_USB_RESET_*`。旧名→新名有兼容；**新默认不会再定义旧名**。应用里写旧宏，2.3.0 直接编不过。
- 用：`PICO_USB_RESET_MAGIC_BAUD_RATE`（默认 1200）、`PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE` 等。头文件 `pico/usb_reset.h`。

### 8. RISC-V / `PICO_DEBUG_INFO_IN_RELEASE=0` 会把整份 libc 放进 RAM

- Issue：[#3070](https://github.com/raspberrypi/pico-sdk/issues/3070)。2.3.0 链接脚本空格修对之后，`*libc.a:` 整库进 RAM，RISC-V 上约 **+40 KB**，512 KB SRAM 工程可能链不过。
- Arm + 默认 `libg.a` 通常只有 memcpy 等进 RAM，本仓库 `pico2` / `rp2350-arm-s` 默认路径一般没事。
- 禁止：设 `PICO_DEBUG_INFO_IN_RELEASE=0` 却不看 map。不要上 RISC-V 构建除非接受这份开销。2.3.1（PR #3075）会收回，只留 mem 函数或改回 Flash。

### 9. 共享 IRQ 链表按单核写的

- Issue：[#730](https://github.com/raspberrypi/pico-sdk/issues/730)。VTOR 默认双核共用。一核正在走 handler 链、另一核 `irq_remove_handler` 会竞态。极少见，但存在。
- 本仓库：LCD 在 core0 挂 `DMA_IRQ_0` 共享 handler。USB stdio 也占共享 IRQ（2.3.0 把默认 `PICO_MAX_SHARED_IRQ_HANDLERS` 从 4 提到 6）。
- 禁止：core1 再 enable 同一条 IRQ。不要在 IRQ 里拆自己这条链。共享槽用完会 assert，新共享 IRQ 先确认还剩槽。

### 10. `xosc_init()` 写死 1–15 MHz

- Issue：[#3074](https://github.com/raspberrypi/pico-sdk/issues/3074)。本板 12 MHz，无影响。以后换 >15 MHz 晶振不能只改 `XOSC_KHZ`，要改 SDK 或等 2.3.1 可配宏。

---

## 以后加外设时才会踩

### I2C

[#2311](https://github.com/raspberrypi/pico-sdk/issues/2311)、[#1471](https://github.com/raspberrypi/pico-sdk/issues/1471)：`i2c_read_timeout_us` / `i2c_write_timeout_us` 在 2.x 可能远超 timeout 占 CPU，甚至和 USB 串口一起锁死。从设备无应答、总线卡住时不要假设 timeout 会准时返回。需要可靠超时就自己看 FIFO + `absolute_time`，或抄 1.5.1 的内部实现。

### SPI 从机

[#1757](https://github.com/raspberrypi/pico-sdk/issues/1757)：`spi_set_baudrate` 对 slave 不适用。本仓库 SPI1 是 LCD 主机，无关。

### 双核

[#1977](https://github.com/raspberrypi/pico-sdk/issues/1977)：`stdio_init_all()` 后立刻 `multicore_launch_core1()`，core1 可能只打印一句启动就进不了循环。中间加数毫秒延时（用 `busy_wait_ms`，不要 `sleep_ms`）。2.3.0 的 `PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED=1` 修的是 lockout，不是这个启动时序。

### 低功耗

[#3115](https://github.com/raspberrypi/pico-sdk/issues/3115)：`low_power_dormant_for_ms()` 醒来后不拆 powman alarm，下一次 GPIO dormant 会立刻假醒。规避：GPIO dormant 前先 `powman_disable_alarm_wakeup()`。本仓库未链 `pico_low_power`。

### 看门狗 + USB BOOTSEL

[#2689](https://github.com/raspberrypi/pico-sdk/issues/2689)：看门狗开着时 `rom_reset_usb_boot` 可能不如预期。要进 BOOTSEL 先停 watchdog。`rom_reboot(..., delay_ms=0)` 是硅 bug RP2350-E30；SDK 封装已改成 1 ms，不要绕过 SDK 直接调 ROM。

### 蓝牙（本板无 CYW43，仅备忘）

2.3.0 把 BTstack 升到 1.8.2：默认 Secure Connections Only、最小密钥 16 字节、关掉 SSP 自动接受。[#3106](https://github.com/raspberrypi/pico-sdk/issues/3106) 报告 2.2.0 能连的 A2DP 在 2.3.0 重连失败。TLV/link key 字节序也有人踩过。不要假设 pico-examples 旧蓝牙代码直接能用。

### PIO RXFIFO PUTGET + DMA DREQ

[#2350](https://github.com/raspberrypi/pico-sdk/issues/2350)：`.fifo putget` 时 `mov OSR, RXFIFO[y]` 会误触发 TX DREQ，DMA 提前打完、数据被冲掉。用 PUTGET 当 scratch 就不要同时用 FIFO DREQ 喂同一 SM。

---

## 2.3.0 已带、不要关掉的规避

这些是 **SDK 已经替你做的**，乱关宏会把硅 bug 放回来。

| 宏 / 行为 | 对应缺陷 | 说明 |
|-----------|----------|------|
| `rom_reboot()` 把 0 ms 改成 1 ms | RP2350-E30 | 0 ms 根本不复位 |
| `PICO_BOOTROM_WORKAROUND_RP2350_A2_ACTIVITY_LED_BUG`（QFN60 Arm 默认 1） | RP2350-E3 | A2 上 Arm 进 USB boot 时活动灯不闪，SDK 改走 RISC-V USB boot |
| `PICO_SYNC_RP2350_SPINLOCK_WORKAROUND`（默认开） | 硬件 spinlock 在本核 SEV | 不开会让 `sleep_*` / 带超时的同步原语在 RP2350 上变成忙等或乱醒 |
| `PICO_CRT0_DEBUG_ENTRY_RESETS_VIA_BOOTROM` | debugger 加载 no_flash | 从调试器进 RAM 镜像时走 bootrom 复位 |
| 软件 spinlock 编号避开 RP2350-E2 | E2 | `PICO_USE_SW_SPIN_LOCKS=0` 时才改硬件锁号 |

RP2350-E9 **没有**对应的 SDK 自动规避。

---

## 2.3.0 行为变化（不是 crash，但会让旧代码编不过或变胖）

- 链接脚本拆成 `.incl`，可用 `pico_add_linker_script_override_path()` / `pico_set_linker_script_var()`。自备 ld 脚本要对新布局。
- 默认 TLS 是 `per_thread`（`pico_thread_local`）。不用 `__thread` 时开销应很小；明确不用可设 TLS=`none` 缩体积。
- `rp2040_rom_version()` 在 RP2350 构建里改名为 `rp2350_rom_version()`。
- `pico_set_printf_implementation(compiler)` + `PICO_STDIO_SHORT_CIRCUIT_CLIB_FUNCS=1` 时，每次 `printf` 后强制 flush，只是部分绕过 C 库缓冲。要正确输出设 `PICO_STDIO_SHORT_CIRCUIT_CLIB_FUNCS=0`。
- GCC 15.2 已支持；官方只按最新 GCC 扫 `-fanalyzer`。本仓库工具链就是 `15_2_Rel1`。
- BTstack 1.6.2 → 1.8.2，MbedTLS 3.6.2 → 3.6.6（含 CVE-2026-25833/25834/25835）。本仓库都没用。

---

## 本仓库现有规避（改主循环 / DMA 前先读）

| 位置 | 做法 | 对应问题 |
|------|------|----------|
| `Hello_Pico.c` 主循环 | `busy_wait_until(next)`，不用 `wait_for_work_until` | #3078、#3124 |
| `src/bsp/lcd/lcd_bus.c` `lcd_dma_abort_one` | `DREQ_FORCE` + 清 EN + abort + 超时 | #923、RP2350-E5 |
| KEY0 | 内部上拉、低有效 | 躲开 E9 下拉 |

`lcd.c` 初始化路径的 `sleep_ms` 尚未改。不要把主循环改回 SDK 示例那种 `wait_for_work_until(at_the_end_of_time)`。

---

## 复查清单

再开发或怀疑「卡住 / USB 没字 / DMA 重启花屏」时按这次序想：

1. 是不是又用了 `sleep_*` / `wait_for_work_until` / 同一 deadline 的 `best_effort_wfe_or_timeout`？
2. 是不是改了 TinyUSB 为 poll，或手写了 `tud_task`+`wfe`？
3. 新 GPIO 是不是内部下拉？
4. 新 DMA 是不是裸 `dma_channel_abort`？
5. 新 PIO 是不是 `pio_encode_mov(osr/exec)`？
6. 共享 IRQ 是不是双核都开了，或 handler 槽用完了？

2.3.1 发布后对照 milestone 26 把已修项划掉，再决定哪些规避可以撤。
