# Linux Thermal Control

一个使用 **C++23、Qt 6.11.2、Qt Quick/QML、systemd、D-Bus 与 Polkit** 开发的 Linux 温度与风扇控制程序。

项目已完整更名为 **Linux Thermal Control**；旧 `colorful-fan-control` 软件包在安装新版本时会被替换，已有曲线配置会自动迁移。

## 当前能力

- 自动发现 Intel/AMD CPU 温度传感器：`coretemp`、`k10temp`、`zenpower` 等 hwmon 驱动；
- 自动发现 AMDGPU、Intel i915/Xe、主板和平台 hwmon 温度、转速与 PWM；
- 在 hwmon 缺少 CPU/GPU 温度时使用只读 thermal-zone 回退；
- 支持已启用内核 `fan_control=1` 的 ThinkPad ACPI 标准风扇等级；
- 识别 IdeaPad/ThinkBook `VPC2004/fan_mode`，但不会把型号相关数值误当成通用满速；
- 优先通过 NVML 读取 NVIDIA GPU 温度、风扇百分比和 RPM；systemd 沙箱保留 NVML 所需的时钟设备访问，驱动明确开放控制时支持逐风扇自动恢复、满速和曲线，否则退回 `nvidia-smi` 只读遥测；
- 支持 Linux 标准 PWM、ThinkPad ACPI、Apple `applesmc/macsmc_hwmon`、可控 NVIDIA NVML，以及经过实机验证的 Clevo `CLV0001`；
- 区分“只读遥测”“部分可控”“可使用自定义曲线”，不会因为能读取温度就声称能控制风扇；
- 每个风扇区域具有独立传感器、能力、安全约束和 YAML 可配置曲线；
- 传感器失效、达到安全温度或控制失败时执行安全保护；
- 图形界面以普通用户运行，写入硬件由 root systemd 服务完成；安装包通过 Polkit 预先授权当前活动的本地桌面会话，日常切换模式无需再次输入管理员密码。

## 三种模式

```text
Automatic    自动：交还内核驱动、固件或硬件控制器
FullSpeed    全速：所有“明确可控”的风扇区域设为 100%
CustomCurve  自定义曲线：按每个区域绑定的温度传感器动态控制
```

只读风扇不会被写入。若系统同时存在可控和只读风扇，界面会显示“部分硬件仅支持遥测”。

## 默认风扇曲线配置

项目中的默认曲线保存在：

```text
config/default-fan-curve.yaml
```

安装后对应路径为：

```text
/etc/linux-thermal-control/default-fan-curve.yaml
```

首次启动或安装时，如果同一目录中不存在隐藏文件 `.风扇曲线配置.backup`，程序会复制 YAML 配置创建该备份；如果备份已经存在，“恢复默认”始终读取备份，不会因后续编辑 YAML 或升级软件包而改变原默认值。需要重新建立默认值时，可先停止服务、删除备份，再重新启动服务。

YAML 中保存图片对应的十个节点：`35/0`、`40/30`、`45/40`、`50/45`、`60/50`、`65/60`、`70/70`、`80/80`、`90/90`、`100/100`。不支持停转的硬件会自动将低占空比节点提高到该区域允许的最低值，区域安全温度仍会提前触发 100% 全速保护。

## 项目结构

```text
src/
├── app/
│   ├── BackendClient.*       # D-Bus 客户端与界面状态
│   ├── InventoryModels.*     # 类型化 QAbstractListModel
│   └── qml/                  # Qt Quick 界面
├── daemon/
│   ├── FanController.*       # 后端聚合、模式与安全策略
│   ├── FanService.*          # D-Bus/Polkit API
│   └── backends/
│       ├── IFanBackend.h
│       ├── HwmonBackend.*
│       ├── ThermalZoneBackend.*
│       ├── ThinkPadBackend.*
│       ├── IdeaPadBackend.*
│       ├── ClevoBackend.*
│       ├── NvidiaBackend.*
│       └── MockBackend.*
└── common/
    ├── ThermalTypes.h
    └── Constants.h
```



## 页面结构

标题右侧提供两个主页面入口：

```text
Linux Thermal Control  [Control] [Info]
```

- `Control`（默认）：CPU/GPU 温度、运行模式、安全告警和自定义风扇曲线；
- `Info`：安全启动状态、风扇与控制能力、检测到的控制后端。

安全启动状态直接读取 EFI `SecureBoot-*` 变量；Info 页面只显示当前状态，不再显示是否需要关闭安全启动的判断与建议。

## 系统托盘与退出

关闭主窗口不会结束程序，而是隐藏到 Linux 系统托盘并继续接收温度与风扇状态：

```text
关闭窗口        → 隐藏到托盘
单击托盘图标    → 打开托盘菜单
双击托盘图标    → 恢复主窗口
“显示主窗口”    → 恢复并激活窗口
“隐藏主窗口”    → 隐藏窗口
“退出”          → 真正结束 GUI 进程
```

如果当前桌面环境完全不支持系统托盘，为避免程序隐藏后无法恢复，关闭窗口会退回普通退出行为。

GUI 在每个用户桌面会话中只允许一个实例。再次点击应用图标、Dock 中的“新建窗口”或重复执行 `linux-thermal-control` 时，不会创建第二个窗口，而是恢复并激活已经运行的窗口。

## Docker 开发

开发环境固定为 Ubuntu 22.04 amd64、GCC 12、C++23 和 Qt 6.11.2。代理、时区、X11/XWayland、Wayland 与 `/dev/dri` 由脚本自动探测。

```bash
./scripts/build.sh          # Docker 编译
./scripts/build.sh --pack   # 编译、测试并生成 deb
./scripts/quick_start.sh    # 私有 D-Bus + mock 后端 + GUI
```

软件包输出：

```text
dist/linux-thermal-control_0.5.12-1_amd64.deb
```

`quick_start.sh` 不访问真实 EC、ACPI 或宿主机 hwmon 写入接口，可安全测试多传感器、多风扇、只读设备、三种模式和独立曲线。

## 安全边界

1. 通用控制只使用内核明确暴露的 `pwmN` 与 `pwmN_enable`；两者不能安全写入时，该通道只显示遥测。
2. 不对未知机器执行未验证的直接 EC 地址写入。
3. Clevo ACPI 命令仅在 DMI 和 `CLV0001/DCHU` 同时匹配时启用。
4. 自定义曲线必须温度递增、占空比不下降，并以区域安全温度 / 100% 结束。
5. 服务停止、卸载或系统挂起时尽力恢复自动控制。

## 文档

- [硬件探测、风扇控制与 systemd 使用](docs/CPU风扇控制说明.md)
- [软件架构](docs/架构设计.md)
- [Docker 开发环境](docs/开发环境.md)
