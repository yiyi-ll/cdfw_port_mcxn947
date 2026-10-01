# CDFW MCXN947 Platform Port

`cdfw_port_mcxn947` 是 CDFW 在 NXP MCXN947 平台上的适配模块。当前目标开发板
为 FRDM-MCXN947。该模块把 `cdfw_shared` 和 `cdfw_boot` 的公共契约连接到
MCUXpresso SDK、板级启动代码、链接脚本和真实硬件。

MCXN947 与 SAMV71 在 CDFW 中地位相同，分别提供独立的平台实现、板级工程、
链接布局和验证证据。两个平台可以穿插开发，任何一方的结果都不能自动代替另一方
的目标板验证。

## 支持范围

| 项目 | 当前约定 |
| --- | --- |
| MCU | NXP MCXN947 |
| 开发板 | FRDM-MCXN947，具体板卡版本待冻结 |
| IDE/构建工具 | MCUXpresso，具体版本和工程形式待冻结 |
| SDK | MCUXpresso SDK，具体版本待冻结 |
| Core 接口 | `cdfw_storage_port_t`、`cdfw_platform_port_t` |
| Boot 交接接口 | `cdfw_boot_startup_handoff_port_t` |
| 当前阶段 | 平台适配和目标板工程脚手架 |

具体内部 Flash 地址、Bank 配置、擦除单元、编程单元、APP 槽容量、执行安全属性
和 mailbox 地址必须依据芯片手册、开发板版本及链接结果评审后冻结，本 README
不提前给出猜测值。

## 职责边界

本模块负责：

- 把 CDFW 逻辑区域映射到 MCXN947 的实际存储区域；
- 实现 Storage Port 的 geometry、read、erase、program、poll 和 sync；
- 把 MCUXpresso SDK 或底层 Flash 驱动结果转换为 `cdfw_result_t`；
- 捕获并规范化复位原因；
- 提供单调毫秒时间、电源安全检查和看门狗服务；
- 在跳转前完成中断、Cache、MPU、安全属性、外设和向量环境处理；
- 校验目标向量地址并完成 APP 跳转；
- 将 Boot 交接信息编码到固定格式的受信任 RAM mailbox；
- 提供 Boot、APP A、APP B 的板级配置和链接脚本；
- 保存 MCUXpresso 工程、SDK 配置及目标板验证证据。

本模块不负责：

- 选择 pending、confirmed 或 fallback 槽；
- 解析 BCB 或镜像业务字段；
- 实现镜像认证、Boot 状态机、升级状态机或循环日志算法；
- 修改 `cdfw_shared` 中已冻结的 Port 结构；
- 向平台无关 Core 暴露寄存器、物理地址或厂商 SDK 类型。

## 目录结构

```text
cdfw_port_mcxn947/
├─ README.md
├─ inc/
│  └─ cdfw/mcxn947/
│     ├─ cdfw_mcxn947_config.h
│     ├─ cdfw_mcxn947_storage_port.h
│     ├─ cdfw_mcxn947_platform_port.h
│     └─ cdfw_mcxn947_handoff_port.h
├─ src/
│  ├─ cdfw_mcxn947_internal_flash.c
│  ├─ cdfw_mcxn947_storage_port.c
│  ├─ cdfw_mcxn947_platform_port.c
│  └─ cdfw_mcxn947_handoff_port.c
├─ test/                              MCU Port 契约测试
└─ board/
   └─ frdm_mcxn947/
      ├─ config/                      区域表、mailbox 和产品配置
      ├─ linker/                      Boot、APP A、APP B 链接脚本
      ├─ test/                        板级区域和链接布局测试
      └─ firmware/
         ├─ boot/                     MCUXpresso Boot 工程
         └─ app_reference/            MCUXpresso 参考 APP 工程
```

`inc/` 只保存平台模块的公共声明。内部 Flash 控制器辅助函数应保持在 `src/` 的
私有边界内。区域地址、槽位大小、安全配置和 mailbox 地址属于板级配置，不应
写入公共 MCU 头文件。

公共头文件使用 `inc` 作为包含根：

```c
#include "cdfw/mcxn947/cdfw_mcxn947_storage_port.h"
#include "cdfw/mcxn947/cdfw_mcxn947_platform_port.h"
#include "cdfw/mcxn947/cdfw_mcxn947_handoff_port.h"
```

## 源文件分工

| 文件 | 分工 |
| --- | --- |
| `cdfw_mcxn947_internal_flash.c` | Flash 控制器操作、硬件状态、超时和原生错误转换 |
| `cdfw_mcxn947_storage_port.c` | 逻辑区域、范围、对齐及 Storage Port 生命周期 |
| `cdfw_mcxn947_platform_port.c` | 时间、复位原因、电源、看门狗、复位和安全跳转 |
| `cdfw_mcxn947_handoff_port.c` | mailbox 编码、完整性字段和内存可见性屏障 |

Storage Port 必须符合“一次最多一个异步擦除或编程操作”的公共契约。平台实现
需要在调用底层驱动前再次检查区域、范围、对齐和状态，并保证同步 read/sync 不
返回 `CDFW_PENDING`。

## 板级内存布局

`board/frdm_mcxn947/config/` 应提供经过评审的区域表，至少明确：

- `CDFW_REGION_BOOT`；
- `CDFW_REGION_BCB0` 和 `CDFW_REGION_BCB1`；
- `CDFW_REGION_SLOT_A` 和 `CDFW_REGION_SLOT_B`；
- 使用时的 CONFIG、LOG 等其他区域；
- Boot-to-APP mailbox 的 RAM 地址、大小、对齐和保留规则；
- 影响 Flash、向量跳转和 APP 执行的安全域、核或 Bank 配置。

区域表与链接脚本必须来自同一份受控内存布局。构建或测试需要检查区域互不
重叠、均处于物理存储范围内，并满足真实读、擦除和编程粒度。Boot 不得写 APP
槽；Update 只能写非活动槽；APP 不得直接写 BCB。

## MCUXpresso 工程

Boot 和参考 APP 是两个独立可执行工程：

```text
board/frdm_mcxn947/firmware/boot/
board/frdm_mcxn947/firmware/app_reference/
```

参考 APP 使用一套业务源代码，通过两个构建配置或链接脚本分别生成 Slot A 和
Slot B 产物。Flash 驱动属于平台模块，不建立生产用的独立 Flash 工程；如需验证
底层控制器，可在板级测试目录增加专用测试工程。

SDK 生成或导入的启动、时钟和外设代码保存在对应的 Boot 或 APP 工程内，不进入
`cdfw_boot`、`cdfw_shared` 或本模块的手写 `src/`。Boot 与 APP 不共享可写的
生成目录。可复现构建所需的 `.project`、`.cproject`、`.settings`、SDK 配置、
链接脚本和必要生成源码应纳入版本控制；IDE 本机状态和构建产物由上层
`.gitignore` 排除。

## 依赖关系

```text
MCXN947 Boot/APP 工程
        │
        ├── cdfw_port_mcxn947
        ├── cdfw_boot / 后续 APP Core
        ├── cdfw_shared
        └── MCUXpresso SDK 驱动

cdfw_port_mcxn947 ──► cdfw_shared 公共 Port 契约
                    └─► cdfw_boot 公开 handoff 契约
```

Core 不得反向包含本模块的私有头文件。本模块也不得依赖 SAMV71 的实现。

## 目标板验收

MCXN947 平台至少需要保存以下验证证据：

- 严格编译无告警，IDE、工具链和 SDK 版本有记录；
- region geometry 与芯片手册、区域表及链接 map 一致；
- BCB 双副本读、擦、写、sync 和精确读回；
- 在 BCB 擦除、编程、同步和读回阶段注入复位或断电；
- APP A/B confirmed 启动；
- pending 尝试、APP 健康确认、超限回滚和受控 fallback；
- 镜像头、载荷、签名或向量损坏时失败关闭；
- mailbox 内容、完整性检查及 Boot/APP 可见性；
- 看门狗、复位原因、软件复位和跳转返回故障路径；
- 中断、Cache、MPU、安全属性、向量、栈指针和入口地址处理；
- Boot 时间、Flash 操作时间、RAM、ROM 和最大栈水位。

只有通过完整目标板闭环，才能把 MCXN947 的 Boot 平台适配标记为完成。单独验证
能够读取 Flash 或跳转到 APP，不代表该平台已经完成。

## 当前状态

目录和接口文件目前处于脚手架阶段。下一次开始 MCXN947 代码开发或目标板验证
前，需要先冻结当天使用的板卡、工具链和本次功能范围；尚未冻结的硬件参数不得
直接写入公共接口。
