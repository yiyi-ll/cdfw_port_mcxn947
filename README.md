# MCXN947 Boot 平台实施设计

> 文档状态：V1.0，已完成设计评审（2026-10-03）
> 适用目标：NXP FRDM-MCXN947，MCXN947VDF CM33 Core0
> 上位契约：`cdfw_shared` 公共 Port、`cdfw_boot` BCB/Image/Startup 公共头文件
> 实施阶段：D7 平台适配和目标板集成

本文是 `cdfw_port_mcxn947` 的唯一 Boot 平台实施基线。它冻结文件职责、产品参数、
函数边界、状态机、链接布局、错误语义和验收条件。实现时不得从聊天记录补充另一套
流程；发现硬件事实或上位契约与本文冲突时，先更新本文和现有架构待更新清单，再改
代码。

本设计没有修改 CDFW 已冻结的 Storage、Crypto、Platform 或 Startup Handoff Port
结构。评审发现的 Boot 镜像向量偏移问题列为实施前门禁，并单独修改 Boot Image
Policy；该修改不改变 256 字节镜像头的持久化布局。

## 1. 设计结论

### 1.1 已冻结的平台基线

| 项目 | 决定 |
| --- | --- |
| MCU / 核 | MCXN947VDF，CM33 Core0；首版不启动 Core1 |
| 开发板 | FRDM-MCXN947；首版只依赖 MCU 内部资源，不依赖板载外部 Flash |
| SDK | MCUXpresso SDK 26.06.00 |
| 工具链 | Arm GNU Toolchain 15.2.1、CMake 3.30.3、Ninja 1.13.2 |
| 构建目标 | `frdmmcxn947/cm33_core0` |
| 安全域 | 首版采用单安全域构建，不执行 Secure/Non-secure 跨域跳转 |
| Boot 时钟 | FROHF 144 MHz；不把外部晶振或 PLL 锁定作为启动必要条件 |
| 内部 Flash | 基址 `0x00000000`，2 MiB，2 个 1 MiB block |
| Flash 几何 | sector 8192、page 128、phrase 16、ROM program unit 512、ROM read unit 16 |
| Flash 驱动 | MCXN947 专用 `driver.romapi_flashiap`；其擦除/编程调用是同步调用 |
| 密码后端 | PSA Crypto + ELS/PKC |
| 签名算法 | 算法 ID 1：ECDSA P-256 + SHA-256 |
| 看门狗 | WWDT，4 秒，无窗口；Boot 启用并在跳转后由 APP 继承 |
| 时间基 | SysTick 1 ms，`uint32_t` 自然回绕 |

SAMV71 与 MCXN947 地位相同，但实现、链接布局和目标板证据相互独立。本文只约束
MCXN947。

### 1.2 完成定义

“MCXN947 Boot 平台完成”表示以下项目全部成立：

1. 本文第 2 节的向量偏移前置变更已经通过 Core review 和 PC 测试。
2. Storage、Crypto、Platform 和 Handoff Port 通过主机契约测试与目标板测试。
3. Boot、APP A、APP B 使用本文冻结的同一内存布局，并通过 map 文件检查。
4. confirmed、pending、确认、超限回滚、fallback、损坏镜像和 BCB 掉电路径在板上闭环。
5. watchdog、复位原因、电源安全、mailbox、跳转和 Flash 故障路径有可重复证据。
6. 生产公钥、镜像签名和 Boot 写保护已经接入。
7. Startup 发布 recovery 决定后的恢复服务已有产品实现；在此之前只能标记为
   “Boot 平台 Port 完成”，不能标记为“生产 Boot 完成”。

## 2. 实施前门禁：向量表偏移

### 2.1 评审发现

当前 `cdfw_boot_image.c` 固定要求：

```text
header.vector_offset == CDFW_IMAGE_HEADER_SIZE == 0x100
```

MCXN947 Core0 有 172 个向量项。目标链接脚本为向量区保留 `0x400` 字节，平台跳转
也必须保守地要求 1 KiB 对齐。Slot A/B 的基址都按 8 KiB 对齐时，`slot + 0x100`
只有 256 字节对齐，不能作为本产品的向量地址。

### 2.2 冻结修正

不修改 `cdfw_image_header_t` 和 256 字节编码格式。镜像头已经包含并认证
`vector_offset`，因此只扩展 `cdfw_boot_image_policy_t`：

```c
uint32_t required_vector_offset;
uint32_t vector_alignment;
```

Boot Image 初始化必须验证两者非零、`required_vector_offset` 不小于头大小，并验证
与槽范围的加法不溢出。镜像头检查必须执行：

```text
header.vector_offset == policy.required_vector_offset
absolute_vector_address % policy.vector_alignment == 0
```

MCXN947 产品策略固定为：

```text
required_vector_offset = 0x400
vector_alignment       = 0x400
```

打包器在 256 字节镜像头后填充 `0x300` 字节，再放置从向量表开始的 APP 原始镜像。
`image_size`、payload SHA-256 和 BCB 中的镜像大小都只覆盖从 `vector_offset` 开始的
APP 字节，不包含镜像头和中间填充。签名 canonical header 范围保持不变，因为
`vector_offset` 已位于被认证的头字段中。

完成条件：公共头文件注释、Boot Image 实现、详细设计、打包器和相应 PC 测试同步
修改并通过。此门禁完成前不得冻结 MCXN947 APP 链接产物，也不得开始真实跳转测试。

## 3. 范围和职责

本模块负责：

- 把 CDFW 逻辑区域映射到 MCXN947 内部 Flash；
- 实现 Storage Port 的 geometry、read、erase、program、poll 和 sync；
- 使用 PSA/ELS-PKC 实现 SHA-256 流式哈希和可信公钥签名验证；
- 捕获并规范化复位原因，提供 1 ms 时间、电源安全和 WWDT 服务；
- 编码 Boot-to-APP mailbox；
- 清理平台状态、二次校验目标向量并完成 Cortex-M33 跳转；
- 提供 FRDM-MCXN947 的区域表、链接脚本、构建配置和验证程序。

本模块不负责选槽、修改 BCB 业务状态、判断镜像业务有效性、实现 recovery Transport、
实现升级协议，或确认当前 APP。上述职责仍属于现有 Core 或后续服务。

## 4. 冻结的 Flash 与 RAM 布局

### 4.1 内部 Flash 区域表

| 区域 | 基址 | 大小 | 末地址（含） | Boot 中权限 |
| --- | ---: | ---: | ---: | --- |
| `BOOT` | `0x00000000` | `0x00040000`（256 KiB） | `0x0003FFFF` | 只读 |
| `SLOT_A` | `0x00040000` | `0x000C0000`（768 KiB） | `0x000FFFFF` | 读；恢复/升级写入由更高层授权 |
| `BCB0` | `0x00100000` | `0x00002000`（8 KiB） | `0x00101FFF` | 读写 |
| `BCB1` | `0x00102000` | `0x00002000`（8 KiB） | `0x00103FFF` | 读写 |
| 保留 | `0x00104000` | `0x0003C000`（240 KiB） | `0x0013FFFF` | 不暴露 |
| `SLOT_B` | `0x00140000` | `0x000C0000`（768 KiB） | `0x001FFFFF` | 读；恢复/升级写入由更高层授权 |

所有起始地址和大小都按 8 KiB sector 对齐。Boot 与 Slot A 位于 block 0，BCB 和
Slot B 位于 block 1；Boot 执行期间提交 BCB 不会擦除当前代码所在 block。`CONFIG`
和 `LOG` 在首版内部 Flash 产品中返回 `CDFW_E_UNSUPPORTED`，不得临时占用保留区。

每个已配置区域报告同一几何：

```text
erase_unit       = 8192
program_unit     = 512
read_unit        = 16
erased_value     = 0xFF
read_while_write = false
```

区域表必须只在 `board/frdm_mcxn947/config/` 定义。Port 不允许用 `switch` 再维护一份
地址副本。编译期断言和板级测试同时检查区域边界、对齐、互不重叠及物理 Flash 上限。

### 4.2 APP 槽内布局

每个槽使用相同相对布局：

| 相对偏移 | 内容 |
| ---: | --- |
| `0x000`–`0x0FF` | CDFW 256 字节镜像头 |
| `0x100`–`0x3FF` | 打包填充，固定 `0xFF` |
| `0x400`–`0x7FF` | APP 向量区，链接脚本保留 1 KiB |
| `0x800` 起 | APP text/rodata/data load image |

APP 本身从 `slot_base + 0x400` 链接和生成原始二进制；打包工具负责增加头与填充。
Slot A 和 Slot B 使用相同源码、不同链接脚本，分别生成两个位置相关的产物。

### 4.3 RAM 与 mailbox

首版 APP 初始 MSP 只允许位于主 SRAM，不允许指向 SRAMX：

```text
APP RAM 允许区: [0x20000000, 0x2004DF00]
mailbox 保留区: [0x2004DF00, 0x2004E000)  共 256 字节
```

`cdfw_boot_image_policy_t` 使用：

```text
ram_base        = 0x20000000
ram_size        = 0x0004DF00
stack_alignment = 8
```

根据现有 Boot Image 契约，允许的 MSP 区间为 `(ram_base, ram_base + ram_size]`，因此
栈顶可以等于 `0x2004DF00`，但不会进入 mailbox。Boot 与 APP 链接脚本都必须为
mailbox 保留 256 字节，任何 `.bss`、heap 或 stack 都不得覆盖它。

## 5. 最终目录和文件职责

```text
cdfw_port_mcxn947/
├─ README.md
├─ inc/cdfw/mcxn947/
│  ├─ cdfw_mcxn947_config.h
│  ├─ cdfw_mcxn947_storage_port.h
│  ├─ cdfw_mcxn947_crypto_port.h
│  ├─ cdfw_mcxn947_platform_port.h
│  └─ cdfw_mcxn947_handoff_port.h
├─ src/
│  ├─ cdfw_mcxn947_internal_flash.h
│  ├─ cdfw_mcxn947_internal_flash.c
│  ├─ cdfw_mcxn947_storage_port.c
│  ├─ cdfw_mcxn947_crypto_port.c
│  ├─ cdfw_mcxn947_platform_port.c
│  ├─ cdfw_mcxn947_handoff_port.c
│  └─ cdfw_mcxn947_jump.S
├─ test/
│  ├─ cdfw_test_mcxn947_regions.c
│  ├─ cdfw_test_mcxn947_storage_port.c
│  ├─ cdfw_test_mcxn947_crypto_port.c
│  ├─ cdfw_test_mcxn947_platform_port.c
│  └─ cdfw_test_mcxn947_handoff_port.c
└─ board/frdm_mcxn947/
   ├─ config/
   │  ├─ cdfw_frdm_mcxn947_boot_config.h
   │  ├─ cdfw_frdm_mcxn947_boot_config.c
   │  └─ cdfw_frdm_mcxn947_trust_anchor.c
   ├─ linker/
   │  ├─ cdfw_mcxn947_boot.ld
   │  ├─ cdfw_mcxn947_app_slot_a.ld
   │  └─ cdfw_mcxn947_app_slot_b.ld
   ├─ test/                         目标板验证程序和证据
   └─ firmware/
      ├─ frdm_mcxn947_boot/         现有 Boot 工程，保留此名称
      └─ app_reference/             后续参考 APP 工程
```

公共头文件只声明 Port 初始化所需类型和函数。`internal_flash.h` 与 `jump.S` 是平台
私有边界，不允许 Core、板级 main 或 APP 直接包含/调用。验证代码只放在 `test/` 或
板级验证工程中，不进入生产 Boot 目标。

### 5.1 公共头文件契约冻结

本节冻结类型和签名；头文件编写时可以调整排版和注释，不能更改字段含义、所有权或
生命周期。

`cdfw_mcxn947_config.h` 只定义跨本平台文件共享的稳定常量，不放板级地址：

```c
#define CDFW_MCXN947_FLASH_SIZE                 UINT32_C(0x00200000)
#define CDFW_MCXN947_FLASH_ERASE_UNIT           UINT32_C(0x00002000)
#define CDFW_MCXN947_FLASH_PROGRAM_UNIT         UINT32_C(512)
#define CDFW_MCXN947_FLASH_READ_UNIT            UINT32_C(16)
#define CDFW_MCXN947_VECTOR_ALIGNMENT           UINT32_C(0x00000400)
#define CDFW_MCXN947_SIGNATURE_ECDSA_P256_RAW   UINT16_C(1)
```

Storage 公共类型和初始化签名：

```c
typedef struct
{
    uint32_t base;
    uint32_t size;
    bool configured;
    bool writable;
} cdfw_mcxn947_storage_region_config_t;

typedef struct
{
    cdfw_mcxn947_storage_region_config_t region[CDFW_REGION_COUNT];
} cdfw_mcxn947_storage_config_t;

typedef enum
{
    CDFW_MCXN947_STORAGE_OPERATION_NONE = 0,
    CDFW_MCXN947_STORAGE_OPERATION_ERASE,
    CDFW_MCXN947_STORAGE_OPERATION_PROGRAM
} cdfw_mcxn947_storage_operation_t;

typedef union
{
    uint32_t words[CDFW_MCXN947_FLASH_PROGRAM_UNIT / sizeof(uint32_t)];
    uint8_t bytes[CDFW_MCXN947_FLASH_PROGRAM_UNIT];
} cdfw_mcxn947_flash_program_buffer_t;

typedef struct
{
    bool initialized;
    cdfw_mcxn947_storage_operation_t operation;
    const cdfw_mcxn947_storage_config_t *config;
    const cdfw_platform_port_t *platform;
    flash_config_t flash;
    cdfw_region_id_t operation_region;
    uint64_t current_offset;
    uint64_t remaining;
    const uint8_t *program_data;
    uint32_t native_detail;
    cdfw_mcxn947_flash_program_buffer_t program_buffer;
} cdfw_mcxn947_storage_context_t;

cdfw_result_t cdfw_mcxn947_storage_initialize(
    cdfw_mcxn947_storage_context_t *context,
    const cdfw_mcxn947_storage_config_t *config,
    const cdfw_platform_port_t *platform,
    cdfw_storage_port_t *port);
```

该 MCU 专用头文件可以包含 MCXN947 SDK 的 `fsl_flash.h`；平台无关 Core 永远不包含
它。`config`、`platform` 和未完成 program 的 `data` 按上述 Port 契约由调用方保持
有效。初始化成功前不设置 `initialized`，失败时 `port` 完全不变。

Crypto 公共类型和初始化签名：

```c
typedef struct
{
    const uint8_t *public_key;
    uint32_t public_key_size;
    uint16_t signature_algorithm;
} cdfw_mcxn947_crypto_config_t;

typedef struct
{
    bool initialized;
    bool hash_active;
    psa_hash_operation_t hash_operation;
    psa_key_id_t public_key_id;
} cdfw_mcxn947_crypto_context_t;

cdfw_result_t cdfw_mcxn947_crypto_initialize(
    cdfw_mcxn947_crypto_context_t *context,
    const cdfw_mcxn947_crypto_config_t *config,
    cdfw_crypto_port_t *port);
```

该头文件可以包含 PSA 类型。初始化把公钥导入 PSA 后不保留 `public_key` 指针；
context 和 PSA key 生命周期覆盖整个 Boot，销毁只发生在进入 recovery 或测试清理时。

Platform config/context 在现有 reset snapshot 基础上冻结为：

```c
typedef struct
{
    uint32_t slot_a_base;
    uint32_t slot_b_base;
    uint32_t slot_size;
    uint32_t vector_offset;
    uint32_t vector_alignment;
    uint32_t ram_base;
    uint32_t ram_size;
    uint32_t stack_alignment;
    uint32_t entry_address_mask;
    uint32_t entry_required_mask;
    uint32_t entry_required_value;
    uint32_t watchdog_clock_hz;
    uint32_t watchdog_timeout_ticks;
} cdfw_mcxn947_platform_config_t;

typedef struct
{
    bool reset_captured;
    volatile bool initialized;
    bool jump_prepared;
    bool watchdog_active;
    volatile uint32_t tick_ms;
    uint32_t reset_reason;
    cdfw_mcxn947_reset_snapshot_t reset_snapshot;
    const cdfw_mcxn947_platform_config_t *config;
} cdfw_mcxn947_platform_context_t;
```

Platform 初始化签名使用第 9.1 节列出的三项；`config` 被保留，生命周期覆盖 Boot。

Handoff 公共类型和初始化签名：

```c
typedef struct
{
    uintptr_t mailbox_address;
    uint32_t mailbox_capacity;
    bool watchdog_active;
} cdfw_mcxn947_handoff_config_t;

typedef struct
{
    bool initialized;
    cdfw_mcxn947_handoff_config_t config;
} cdfw_mcxn947_handoff_context_t;

cdfw_result_t cdfw_mcxn947_handoff_initialize(
    cdfw_mcxn947_handoff_context_t *context,
    const cdfw_mcxn947_handoff_config_t *config,
    cdfw_boot_startup_handoff_port_t *port);
```

Handoff 初始化复制 config，不保留输入指针。所有结构体必须支持静态分配，不允许初始化
函数分配堆内存。

## 6. 初始化和运行顺序

生产 Boot 的固定顺序为：

1. Reset_Handler 完成最小运行时初始化。
2. 在 `BOARD_InitHardware()` 可能清除状态前调用
   `cdfw_mcxn947_platform_capture_reset()`。
3. 初始化 pin、FROHF 144 MHz、必要的时钟与内存。
4. 初始化 Platform Port：配置 1 ms SysTick、LVD/BOD 策略和 WWDT。
5. 初始化 Storage Port，读取 ROM Flash 属性并核对本文冻结几何。
6. 初始化 Crypto Port，初始化 PSA/ELS-PKC 并导入编译期可信公钥。
7. 初始化 Handoff Port，校验固定 mailbox 地址和大小。
8. 调用 `cdfw_boot_startup_get_work_buffer_size()`，分配/检查静态工作区。
9. 调用 `cdfw_boot_startup_initialize()`、`start_begin()`，在主循环中每次调用一次
   `start_poll()` 并服务 watchdog。
10. Jump 决定正常不会返回；Recovery 决定交给产品 recovery 服务。

任一初始化失败都不得发布半初始化 Port。输出 Port 在失败时保持调用前内容；只有
全部检查成功后才一次性写入完整描述符。

## 7. Storage Port 设计

### 7.1 公共边界

`cdfw_mcxn947_storage_port.h` 声明：

- 逻辑区域配置项：基址、大小、是否配置、是否允许破坏性操作；
- 单一未完成操作的枚举与上下文；
- `cdfw_mcxn947_storage_initialize()`；
- 固定 Flash 几何常量。

只有初始化函数是 MCXN947 模块的公共函数。`get_geometry`、`read`、`erase_begin`、
`program_begin`、`operation_poll` 和 `sync` 都是 `cdfw_mcxn947_storage_port.c` 中的
私有回调，通过 `cdfw_storage_port_t` 使用和测试。这样不会建立第二套 Storage API。

上下文保存：初始化标志、区域表引用、Platform Port 引用、SDK `flash_config_t`、当前
操作种类、逻辑区域、当前物理地址、剩余长度、调用方 program 指针、512 字节对齐
暂存区、最近原生状态。区域表和 Platform Port 的生命周期必须覆盖上下文。

### 7.2 初始化

`cdfw_mcxn947_storage_initialize()` 必须：

1. 检查参数、未初始化状态、完整 Platform Port 和区域表。
2. 验证所有已配置区域位于 2 MiB PFlash 内、边界不溢出、按 8 KiB 对齐且互不重叠。
3. 验证 `BOOT` 只读，`BCB0/BCB1/SLOT_A/SLOT_B` 与本文地址完全一致，CONFIG/LOG
   未配置。
4. 调用 `FLASH_Init()` 与 `FLASH_GetProperty()`，核对 block 数、block 大小和 sector
   大小；不匹配返回 `CDFW_E_UNSUPPORTED`。
5. 清空操作状态并一次性发布全部 Storage 回调。

初始化只查询控制器，不擦除、不编程，也不因初始化而读取 CDFW 数据。

### 7.3 查询和读取

`get_geometry` 按冻结 Port 契约检查 context、初始化状态、region 和配置状态；只有成功
才写出 geometry。它不访问 Flash 控制器。

`read` 先执行逻辑区域、范围、防溢出和 16 字节对齐检查。合法空读直接成功并允许
`buffer == NULL`。非空读在未完成写操作期间返回 `CDFW_BUSY`；其余情况调用 ROM
`FLASH_Read()`，完整成功后才返回 `CDFW_OK`。调用方不得使用失败后的部分 buffer。

### 7.4 逻辑异步擦写

MCXN947 专用 ROM Flash 驱动没有该器件可用的 non-blocking erase/poll API。Port 仍须
满足 CDFW 的 `begin/poll` 契约，采用“请求异步、每次 poll 完成一个硬件单元”的状态机：

- 非空 `erase_begin` 只验证并记录请求，返回 `CDFW_PENDING`；
- 每次 `operation_poll` 最多擦除并验证一个 8 KiB sector；
- 非空 `program_begin` 只验证并保留调用方指针，返回 `CDFW_PENDING`；
- 每次 `operation_poll` 复制一个 512 字节单元到对齐暂存区，编程并精确读回验证；
- 单元完成但仍有剩余时返回 `CDFW_PENDING`；最后一个单元完成返回 `CDFW_OK` 并释放
  操作槽；任何错误都终止并释放操作槽。

每个破坏性单元开始前重新调用 `power_is_safe()`；不安全返回 `CDFW_E_POWER`，绝不
调用 ROM。调用 ROM 前保存并禁止中断、停止 LPCAC 预取/缓存，调用后执行必要的
cache invalidate、恢复 cache/中断并执行 DSB/ISB。WWDT 在每个单元前后服务，单次
ROM 调用必须小于 watchdog 周期。

`sync` 在有活动操作时返回 `CDFW_BUSY`；否则执行数据/指令屏障并返回 `CDFW_OK`。
内部 Flash 无额外缓存，`sync` 不启动擦写。`read` 和 `sync` 永不返回
`CDFW_PENDING`。

### 7.5 私有 Flash 包装层

`src/cdfw_mcxn947_internal_flash.h` 只供 Storage 实现包含，冻结以下职责：

- 初始化和读取控制器属性；
- 读取一段满足 ROM 对齐的 Flash；
- 擦除并验证一个 sector；
- 编程并验证一个 512 字节单元；
- 保存/恢复中断和 LPCAC 状态；
- 把 SDK `status_t` 保存为 `native_detail` 并映射为 `cdfw_result_t`。

该私有层不认识 `cdfw_region_id_t`，不做区域授权，不管理多单元操作，也不保存调用方
program 指针。区域和异步生命周期只属于 Storage Port。

## 8. Crypto Port 设计

新增 `cdfw_mcxn947_crypto_port.h/.c`。公共头文件声明 context、可信公钥配置和
`cdfw_mcxn947_crypto_initialize()`；SHA 与签名回调保持为 `.c` 内私有函数，只通过
`cdfw_crypto_port_t` 调用。

冻结规则：

- SHA 固定为 SHA-256，使用 `psa_hash_setup/update/finish`；
- 算法 ID 1 固定为 ECDSA P-256 + SHA-256；
- 签名编码固定为 64 字节大端 `r || s`，不得接受 DER；
- 可信公钥固定为 65 字节 SEC1 uncompressed `0x04 || X || Y`；
- 公钥由 `cdfw_frdm_mcxn947_trust_anchor.c` 编译进 Boot，并在初始化时导入 PSA；
- 镜像、BCB 和外部 Transport 都无权替换可信公钥；
- 生产构建若仍使用测试公钥或空公钥必须编译失败；测试公钥只允许测试配置启用；
- 不使用无界动态内存。若 PSA 后端需要内存池，必须使用构建期固定大小的静态池；
- `hash_begin()` 会废弃旧的未完成哈希；`CDFW_BUSY` 时不消耗数据、不改变可重试状态；
- PSA invalid signature 映射 `CDFW_E_AUTH`，未知算法映射 `CDFW_E_UNSUPPORTED`，参数
  和签名长度错误按公共契约映射，其他后端失败映射 `CDFW_E_IO`。

需要启用的 SDK 组件固定为 `component.els_pkc` 和
`component.psa_crypto_driver.els_pkc`，与 SDK 26.06.00 的 FRDM-MCXN947
`mbedtls3x_examples/psa_crypto_examples` 配置一致；其余源文件由 MCUX 组件依赖图带入。
不得在 Port 内实现自制 ECDSA，也不得同时接入旧 mbedTLS PSA 配置形成第二套后端。

## 9. Platform Port 设计

### 9.1 公共函数

`cdfw_mcxn947_platform_port.h` 只公开三个生命周期函数：

```c
cdfw_result_t cdfw_mcxn947_platform_capture_reset(
    cdfw_mcxn947_platform_context_t *context);

cdfw_result_t cdfw_mcxn947_platform_initialize(
    cdfw_mcxn947_platform_context_t *context,
    const cdfw_mcxn947_platform_config_t *config,
    cdfw_platform_port_t *port);

void cdfw_mcxn947_platform_systick_isr(
    cdfw_mcxn947_platform_context_t *context);
```

所有 CDFW Platform 回调是 `.c` 内私有函数。当前已有源码只算草稿，必须按本节
review：保留正确的 reset capture/time 方向，补全 config、电源、watchdog、跳转和
初始化原子发布；失败关闭的死循环不能作为生产 `jump_to_image()`。

### 9.2 复位原因

`capture_reset` 必须在板级初始化前读取并缓存 CMC0 current status、sticky status 和
reset count。`reset_reason()` 只返回缓存后的稳定 CDFW 位标志，不再读取寄存器。

映射原则：POR/VBAT→POWER_ON，PIN→EXTERNAL，SW→SOFTWARE，WWDT/WDOG/CDOG→
WATCHDOG，电压检测→BROWNOUT，WAKEUP→LOW_POWER_WAKE，安全/篡改→SECURITY；
WARM/FATAL 只表示类别，若没有具体来源则返回 OTHER；没有任何来源返回 UNKNOWN。
未识别但置位的原生位附加 OTHER。实际宏必须以 SDK 26.06.00 的 MCXN947 CMC 头文件
为准，使用 `#if defined` 处理该器件未提供的可选位。

### 9.3 时间、电源和 watchdog

SysTick 按 `SystemCoreClock / 1000` 配置。时钟不能整除 1000 时初始化返回
`CDFW_E_UNSUPPORTED`。ISR 只增加 `volatile uint32_t tick_ms`；回调只读取缓存值。

`power_is_safe()` 只有在以下条件同时成立时返回 true：

- Platform 已初始化；
- SPC 低电压监测已按板级阈值启用；
- SPC 当前没有 core/system/IO low-voltage 状态；
- ANACTRL 当前没有 VDD main brownout/power fault；
- 本次读取的状态完整可用。

Platform 初始化对 IO/System/Core 三路选择 `kSPC_LowVoltageHighRange`，配置完成后仅在
ANACTRL 当前电源状态正常时清除旧的 detect flags，再把监测标记为 armed。运行期任一
LVD/BOD 标志出现后保持 unsafe，直到系统复位重新初始化。未知状态一律 false。Boot
不使用 ADC 猜测供电安全。

WWDT0 使用 FRO 1 MHz 时钟，输入时钟按硬件除以 4，因此 4 秒超时的
`timeoutValue` 等于 `watchdog_clock_hz`；`windowValue` 使用无窗口最大值。启用 timeout
system reset、配置保护和 oscillator lock。初始化后不得在 `watchdog_kick()` 中改变
配置；该函数只执行 `WWDT_Refresh()`。`prepare_app_jump()` 不关闭 WWDT，mailbox
flags 告知 APP watchdog 已启用，参考 APP 必须在早期启动代码中继续 refresh。

### 9.4 系统复位、平台清理和跳转

`system_reset()` 禁止中断、执行 DSB，调用 `NVIC_SystemReset()`；若意外返回则留在
不再服务 watchdog 的失败关闭循环，让已启用的 WWDT 提供第二条复位路径，绝不恢复
普通 Boot 流程。

`prepare_app_jump()` 固定执行：

1. 停止 SysTick 并清除 pending SysTick/PendSV；
2. 停止 Boot 自有调试串口、DMA、定时器和加密外设会话；
3. 清理并禁用 Boot 使用的 cache；
4. 禁止全局中断，清除所有 NVIC enable 和 pending bank；
5. 保持 WWDT 和 mailbox；
6. 执行 DSB/ISB，并把 context 标记为 prepared。

`jump_to_image()` 仍必须二次检查：context 已 prepared；地址可由 64 位无损转换到
32 位；地址恰好等于 Slot A 或 Slot B 的 `base + 0x400`；地址按 1 KiB 对齐；从该
地址重新读取的 MSP、Thumb 位和 reset handler 仍满足 Platform config 中冻结的 RAM
和槽范围。Platform Port 没有接收 Core 验证结果副本，因此不虚构“与旧值比较”的
接口；启动集成层对槽写入的互斥保证验证后镜像不变。任何检查失败都执行系统复位，
绝不跳转未知地址。

实际切换由私有 `cdfw_mcxn947_jump.S` 完成，避免 C 函数修改 MSP 后执行函数尾声：

1. 写 VTOR，DSB/ISB；
2. 清 CONTROL、PSP、MSPLIM、BASEPRI、FAULTMASK；
3. 写 MSP；
4. 在所有 NVIC 源已禁用/清 pending 后清 PRIMASK，使 APP 以接近硬件复位的中断状态
   进入 Reset_Handler；
5. `BX reset_handler`，永不返回。

## 10. Handoff mailbox 设计

`cdfw_mcxn947_handoff_port.h` 公开 context 和
`cdfw_mcxn947_handoff_initialize()`；`publish` 是 `.c` 内私有回调。编码固定为 64 字节：

| 偏移 | 长度 | 字段 |
| ---: | ---: | --- |
| 0 | 4 | magic `0x57464443`，小端内存字节为 `CDFW` |
| 4 | 2 | format version = 1 |
| 6 | 2 | encoded length = 64 |
| 8 | 4 | slot |
| 12 | 4 | flags：bit0 trial boot，bit1 watchdog active |
| 16 | 4 | normalized reset reason |
| 20 | 4 | authoritative BCB sequence |
| 24 | 8 | Boot version：四个 little-endian `uint16_t` |
| 32 | 4 | persistent `last_error` |
| 36 | 24 | reserved，必须清零 |
| 60 | 4 | CRC32，覆盖 `[0, 60)` |

初始化校验 mailbox 地址、256 字节保留容量和 32 位对齐。`publish()` 先在栈上构造
完整 64 字节临时编码并计算 CRC，最后一次性复制到 volatile mailbox，执行 DMB/DSB
后返回。失败不得留下带有效 magic 的半成品；写入前先清 magic，magic 最后发布。
APP 先检查 magic/version/length/reserved/CRC/slot，再使用内容；消费后可清 magic。

## 11. 产品策略与板级配置

`board/frdm_mcxn947/config/cdfw_frdm_mcxn947_boot_config.*` 是所有产品参数的唯一来源，
必须定义：区域表、Platform config、Boot Image policy、Boot Startup policy、mailbox
配置和 build-time 静态断言。

首版策略固定：

```text
product_id              = 0x4D43584E  /* “MCXN”的产品数值标识 */
device_hardware_mask    = 0x00000001  /* 参考平台兼容位 0 */
required_signature_type = 1
slot_a_base             = 0x00040000
slot_b_base             = 0x00140000
required_vector_offset  = 0x00000400
vector_alignment        = 0x00000400
ram_base                = 0x20000000
ram_size                = 0x0004DF00
stack_alignment         = 8
entry_required_mask     = 1
entry_required_value    = 1
entry_address_mask      = 0xFFFFFFFE
port_busy_timeout_ms    = 100
bcb_load_max_attempts   = 3
bcb_load_timeout_ms     = 100
boot_version            = 0.1.0.0
```

`product_id` 和 hardware bit 是本产品首版协议值，后续不得因营销名称变化而静默复用。
不同硬件修订若镜像兼容，可增加 hardware mask 位；不兼容时使用新的产品策略。

生产公钥的具体坐标不是本文可代填的数据。其来源冻结为受控 release 配置生成的
`cdfw_frdm_mcxn947_trust_anchor.c`，代码评审必须核对指纹；缺少生产密钥材料时生产
目标构建失败。这是密钥注入内容，不是未定的接口设计。

## 12. 链接与构建

Boot 链接范围为 `0x00000000`–`0x0003FFFF`。Core1 必须保持 reset，Boot 链接脚本
不得继续保留 SDK 示例的 `0x000C0000` Core1 image 窗口。Boot RAM 上界为 mailbox
起始地址。

APP A/B 链接脚本分别把向量放到 `0x00040400` 和 `0x00140400`，向量 section 均保留
1 KiB，text 从相应 `+0x800` 开始。链接后检查：

- 向量地址和大小；
- reset handler 位于同一槽 payload；
- load image 不越过 768 KiB 槽；
- RAM 段、heap 和 stack 不进入 mailbox；
- Boot 不越过 256 KiB；
- 不存在 Core1 image 或意外外部 Flash section。

Boot `prj.conf` 至少启用：

```text
CONFIG_MCUX_COMPONENT_driver.romapi_flashiap=y
CONFIG_MCUX_COMPONENT_driver.mcx_cmc=y
CONFIG_MCUX_COMPONENT_driver.mcx_spc=y
CONFIG_MCUX_COMPONENT_driver.anactrl=y
CONFIG_MCUX_COMPONENT_driver.wwdt=y
CONFIG_MCUX_COMPONENT_driver.cache_lpcac_n4a_mcxn=y
CONFIG_MCUX_COMPONENT_component.els_pkc=y
CONFIG_MCUX_COMPONENT_component.psa_crypto_driver.els_pkc=y
```

现有工程来自 `led_blinky`，不能把旧 ELF 当作基线。CMake 必须显式加入 CDFW
Shared、Boot Core、MCXN947 Port 和板级 config 源文件及 include root；生产目标移除
`cdfw_mcxn947_boot_validation.*` 和 LED 测试逻辑。生成的 SDK 板级文件保留在工程内，
手写 Port 源码不复制到生成目录。

## 13. 错误映射和诊断

Port 返回值只使用现有 `cdfw_result_t`：

| 情况 | 结果 |
| --- | --- |
| NULL、非空操作缺少 buffer/data | `CDFW_E_PARAM` |
| 未初始化、错误生命周期、无活动 poll | `CDFW_E_STATE` |
| region/range/overflow/alignment 失败 | `CDFW_E_RANGE` |
| 未配置 region 或算法 | `CDFW_E_UNSUPPORTED` |
| 已有未完成擦写 | `CDFW_BUSY` |
| 破坏性操作已接收、仍有单元未完成 | `CDFW_PENDING` |
| LVD/BOD 不安全或未知 | `CDFW_E_POWER` |
| 擦除/编程读回不一致 | `CDFW_E_VERIFY` |
| 签名不匹配 | `CDFW_E_AUTH` |
| SDK 控制器/PSA 其他失败 | `CDFW_E_IO` |
| 受控等待超过时限 | `CDFW_E_TIMEOUT` |

`native_detail` 保存 SDK 原始 `status_t` 或 PSA status 的无损 32 位表示，仅用于调试，
不得改变 Portable 结果语义。Platform 的标量/void 回调没有错误通道，因此初始化必须
先证明其前置条件；运行期异常只能失败关闭。

## 14. 测试和验收

### 14.1 Codex 编写的测试代码

测试代码不进入生产镜像。用户完成每个生产实现单元并通过 review 后，由 Codex 增加：

- 区域表边界、重叠、对齐和精确常量测试；
- Storage 参数、空操作、状态迁移、每 poll 一个单元、调用顺序、断电点和错误映射；
- Crypto SHA-256 金样、分块等价、签名有效/无效/错误编码和 busy 恢复；
- Platform 复位映射、时间回绕、电源状态组合、watchdog 与跳转前置条件；
- mailbox 字节级金样、CRC、reserved、magic 最后发布和失败输出；
- C11/C++17 头文件检查、严格告警构建和 sanitizer 主机测试。

对 ROM Flash、CMC、SPC、WWDT、cache 和跳转汇编使用可替换的私有 wrapper fake，
生产 Port 测试必须经真实 `cdfw_*_port_t` 回调进入，不能复制实现逻辑。

### 14.2 目标板验证

至少执行：

1. geometry 与芯片宏、区域表和 map 三方一致。
2. 两个 BCB sector 擦除、512 字节 padding 编程、sync、读回和双副本选择。
3. 在每个 erase/program 单元和 BCB 提交阶段复位，重启后只能选择旧或新完整记录。
4. Slot A/B confirmed 启动，向量地址分别为 `0x00040400`/`0x00140400`。
5. pending 尝试在跳转前持久化；APP 确认后进入 confirmed；超限回滚。
6. confirmed 损坏时建立 fallback pending，不能直接 confirmed。
7. 头、payload、签名、向量、MSP、entry 任一损坏均失败关闭。
8. 上电、外部、软件、watchdog、brownout 等可制造复位原因映射正确且本次启动稳定。
9. LVD/BOD 不安全时所有破坏性 Flash 调用被阻止。
10. mailbox 字节级内容和 CRC 正确；APP 可读取并继续服务 watchdog。
11. 真实 APP 跳转后 VTOR、MSP、中断、cache 和外设状态正确；跳转返回路径触发复位。
12. 记录 Boot 时间、最大 Flash 单元时间、ROM/RAM、最大栈水位和 watchdog 裕量。

## 15. 实施顺序与分工

唯一实施顺序如下，前一项 review 和测试通过后才进入后一项：

1. 修正 Boot Image vector policy、详细设计和 PC 测试。
2. 创建板级区域表、三个链接脚本和静态布局检查。
3. 实现 Storage 初始化、geometry、read。
4. 实现 private Flash wrapper 与 Storage erase/program/poll/sync。
5. 实现 Crypto Port。
6. 完成 Platform reset/time/power/watchdog。
7. 实现 Handoff mailbox。
8. 实现 prepare/jump 和汇编跳转。
9. 重建 Boot 工程并接入完整 Core。
10. 建立参考 APP A/B，执行目标板闭环和掉电测试。

分工保持不变：Codex 先依据本文讲清一个生产实现单元的文件、结构体、变量、函数、
前置条件、状态迁移和错误语义；用户编写生产函数；Codex review 通过后编写并运行
测试。Codex 不用测试代码替用户实现生产函数，也不在下一步偷偷改变本文冻结值。

## 16. 设计评审记录

### 16.1 评审范围

本次已对照：

- `cdfw_storage_port.h`、`cdfw_crypto_port.h`、`cdfw_platform_port.h`；
- `cdfw_boot_image.h/.c`、`cdfw_boot_startup.h`；
- Boot Startup and Recovery Detailed Design V1.0；
- MCUXpresso SDK 26.06.00 的 MCXN947 feature、ROM Flash、CMC、SPC、WWDT、LPCAC、
  PSA/ELS-PKC 组件；
- 当前 FRDM-MCXN947 工程、链接布局、CMake/Kconfig 和未完成 Port 源码。

### 16.2 评审结论

| 评审项 | 结论 |
| --- | --- |
| Core/Port 职责 | 通过；没有把选槽、BCB 或镜像业务逻辑放入平台层 |
| Flash 区域与几何 | 通过；总量、block、sector 和 program/read unit 可由本地 SDK 复核 |
| Storage 异步契约 | 通过；用每次 poll 一个同步 ROM 单元满足有界推进 |
| Crypto 算法与编码 | 通过；算法、签名、公钥格式和密钥来源已固定 |
| reset/time/power/watchdog | 通过；未知电源失败关闭，watchdog 交接已定义 |
| mailbox ABI | 通过；64 字节线格式、CRC 和发布顺序已固定 |
| 跳转 ABI | 条件通过；必须先完成第 2 节 vector policy 门禁 |
| 构建与文件边界 | 通过；生产代码、SDK 生成代码和验证代码已分开 |
| 生产完成条件 | 通过；恢复服务和生产密钥是发布门禁，不能用临时代码代替 |

总体结论：本设计可作为 MCXN947 Boot 平台实施基线。第 2 节 Boot Image Policy
修正、上位机打包器同步和 PC 回归已经完成；板级区域表、编译期布局断言和主机
区域测试已经完成 review，86/86 项检查通过。平台实施进入三个链接脚本和静态布局
检查。其余参数和模块边界已冻结，后续 review 只能指出实现是否符合
本文，不能另起一套流程。硬件事实若被实板或新版 SDK 否定，必须提供证据并修订本文。

### 16.3 变更控制

以下内容发生变化时必须重新评审本文：Flash 分区、向量偏移、mailbox ABI、签名算法
或编码、可信密钥来源、安全域/Core1 策略、watchdog 继承规则、供电安全判据。普通
函数内部重构不需要改变设计，但仍须保持公共 Port 契约和测试证据。

### 16.4 当前代码处置结论

当前工作区代码按本文评审后的状态如下：

- `cdfw_mcxn947_platform_port.h/.c` 是可保留的草稿，不是已通过实现。reset snapshot、
  原因规范化和 SysTick 方向可沿用；公共初始化需增加 Platform config，context 需增加
  jump/watchdog 状态。
- 当前 `power_is_safe()` 恒 false、`watchdog_kick()` 空实现、`prepare_app_jump()` 只停
  SysTick、`jump_to_image()` 永久循环，均不满足本文生产完成条件，必须由用户按第 9 节
  实现后再 review。
- 板级内部 Flash 区域表、编译期布局断言及 region test 已完成 review；真实 ARM C11
  严格编译、C++17 公共头检查和 86/86 项主机检查通过。三个链接脚本和静态布局检查
  尚未创建。
- Storage 和 internal Flash 实现仍为空；handoff 仍为空，Crypto、jump assembly 尚未
  创建，必须按第 5 节继续实现。
- 现有 `cdfw_mcxn947_boot_validation.*`、LED main 和旧 ELF 只属于验证脚手架；其中错误
  include 和未接入 CDFW 源码的问题不影响设计结论，但在第 9 个实施单元重建工程时
  必须清理。
- 本次没有修改上述生产函数体，也没有把草稿标记为通过。
