# Linux 风扇探测、控制与 systemd 服务使用

## 1. 先确认内核暴露了什么

```bash
find /sys/class/hwmon -maxdepth 2 -type f \
  \( -name 'name' -o -name 'temp*_input' -o -name 'fan*_input' \
     -o -name 'pwm[0-9]*' -o -name 'pwm*_enable' \) -print

for hwmon in /sys/class/hwmon/hwmon*; do
  printf '\n[%s] %s\n' "$hwmon" "$(cat "$hwmon/name" 2>/dev/null)"
  grep -H . "$hwmon"/temp*_input "$hwmon"/temp*_label \
             "$hwmon"/fan*_input "$hwmon"/pwm[0-9]* \
             "$hwmon"/pwm*_enable 2>/dev/null || true
done
```

常见 CPU 驱动：

```text
Intel: coretemp
AMD:   k10temp、zenpower
```

常见 GPU/平台驱动：

```text
amdgpu、i915、nouveau、thinkpad、dell_smm、asus、nct6775 等
```

出现温度或 RPM 只代表“能读”。必须同时存在可写 `pwmN` 与 `pwmN_enable`，本软件才会声明该通道可控。

## 2. 安装与启动

```bash
sudo apt install ./dist/linux-thermal-control_VERSION_amd64.deb
sudo systemctl enable --now linux-thermal-daemon.service
systemctl status linux-thermal-daemon.service
journalctl -u linux-thermal-daemon.service -b
```

图形界面：

```bash
linux-thermal-control
```

## 3. systemd 服务行为

服务文件：

```text
/usr/lib/systemd/system/linux-thermal-daemon.service
```

关键行为：

- 以 root 身份访问 hwmon/ACPI；
- 通过 D-Bus 名称 `org.linuxthermal.Control1` 提供服务；
- `StateDirectory=linux-thermal-control` 创建配置目录；
- 崩溃时自动重启；
- 正常停止时尝试恢复自动模式；
- 不再使用 `ConditionPathExists=CLV0001`，因此通用 hwmon 机器也能启动；
- 安装包强制依赖 `acpi-call-dkms`、`gcc-12`、`mokutil` 和 `shim-signed`，确保 Clevo 专用后端在 Ubuntu 22.04 HWE 内核上能够编译 `acpi_call`，并支持在不关闭 Secure Boot 与 Shim 验证的情况下注册模块签名密钥。

APT 安装时会自动检查 MOK 注册状态：已经注册时直接加载 `acpi_call`；尚未
注册时自动启动 Ubuntu 的注册向导。只有在无交互终端等环境中无法启动向导
时，才需要手动继续：

```bash
sudo linux-thermal-secure-boot-setup --prepare
```

助手会使用 Ubuntu 的 DKMS MOK 重新构建并签名 `acpi_call`，然后发起公钥注册。重启进入 MokManager 后选择 `Enroll MOK → Continue → Yes`，输入临时密码并再次重启。这个启动阶段确认是 Secure Boot 信任链的一部分，不能由 deb 安装脚本静默跳过。状态检查命令为：

```bash
linux-thermal-secure-boot-setup --status
```

图形界面的 Info 页面会在“安全启动状态”下面单独显示“安全启动密钥”，区分
`已注册`、`等待重启确认`、`未注册`、`尚未生成`和`无法判断`。

重启和查看日志：

```bash
sudo systemctl restart linux-thermal-daemon.service
systemctl show linux-thermal-daemon.service -p ActiveState -p SubState
journalctl -fu linux-thermal-daemon.service
```

## 4. D-Bus 诊断

```bash
gdbus call --system \
  --dest org.linuxthermal.Control1 \
  --object-path /org/linuxthermal/Control1 \
  --method org.linuxthermal.Control1.GetStatus
```

安装软件包时已经写入 Polkit 授权策略。当前活动的本地桌面会话切换模式时会直接获得授权，不再弹出管理员密码窗口；非活动会话和远程会话仍然被拒绝：

```bash
gdbus call --system \
  --dest org.linuxthermal.Control1 \
  --object-path /org/linuxthermal/Control1 \
  --method org.linuxthermal.Control1.SetMode automatic

gdbus call --system \
  --dest org.linuxthermal.Control1 \
  --object-path /org/linuxthermal/Control1 \
  --method org.linuxthermal.Control1.SetMode full-speed
```

自定义曲线建议通过 GUI 编辑，因为 GUI 会应用区域最低占空比、点顺序和安全端点限制。

默认曲线由 `/etc/linux-thermal-control/default-fan-curve.yaml` 描述。安装或首次启动时，如果配置目录中不存在 `.风扇曲线配置.backup`，程序会从 YAML 创建备份；“恢复默认”读取该备份。如果备份已经存在，则不会覆盖它。项目源码中的对应文件位于 `config/default-fan-curve.yaml`。

## 5. NVIDIA、AMD 与 Intel GPU

若 `nvidia-smi` 能读取 GPU 温度，但 Info 页面没有 NVIDIA 后端，请检查守护进程日志与 NVML 动态库。软件包提供的 systemd 单元允许 NVML 访问所需的时钟设备，同时保留其他服务沙箱限制。

### NVIDIA

```bash
nvidia-smi --query-gpu=pci.bus_id,name,temperature.gpu,fan.speed \
  --format=csv,noheader,nounits
```

程序优先动态加载 NVML。若驱动同时支持逐风扇设置、默认策略恢复和速度范围查询，则启用 NVIDIA 风扇区域；否则保持只读，尤其是笔记本 GPU。`nvidia-smi` 仅作为没有可用 NVML 查询接口时的遥测回退。

### AMDGPU

```bash
find /sys/class/drm -path '*/device/hwmon/hwmon*' -print
```

AMDGPU 的温度、RPM 和 PWM 通常最终链接到 `/sys/class/hwmon`，由通用后端处理。


### Apple Mac

Intel Mac 的 `applesmc` 与 Apple Silicon 的 `macsmc_hwmon` 通过标准 hwmon 属性工作。程序要求 `fanN_min/max/manual` 和 `fanN_output` 或 `fanN_target` 完整存在，按目标 RPM控制并通过 `manual=0` 恢复固件自动策略；不会直接读写 SMC 原始寄存器。

### Intel iGPU

Intel iGPU 可能只有温度，没有独立风扇。此时界面显示温度，但不会创建不存在的 GPU 风扇。

## 6. ThinkPad、Dell、ASUS、Acer、MSI

本软件不按品牌猜测 EC 地址，而是等待相应内核驱动公开标准 hwmon 属性：

- ThinkPad：`thinkpad_acpi` 需要内核/模块允许 fan control 才能出现可写控制；
- Dell：优先使用 `dell_smm_hwmon` 提供的标准属性；
- ASUS/Acer/MSI：使用平台驱动公开的 hwmon PWM；
- 只有私有接口或未知 EC 地址时显示不支持，不执行盲写。


## 7. Lenovo IdeaPad / ThinkBook 固件接口

IdeaPad 与 ThinkBook 即使名称相近，也可能使用不同的固件接口。`ideapad_laptop` 驱动常见的只读诊断节点包括：

```text
/sys/firmware/acpi/platform_profile
/sys/firmware/acpi/platform_profile_choices
/sys/bus/acpi/devices/VPC2004:00/physical_node/fan_mode
```

`platform_profile` 描述整机性能策略，不等同于风扇占空比。部分固件的 `fan_mode` 读回值也不对应写入值，并且没有 RPM 可供校验。因此程序只报告固件管理状态，不把 profile 或 fan_mode 推断为“满速”。

ThinkPad 与 ThinkBook/IdeaPad 不使用同一控制接口。真正的 ThinkPad 若提供 `/proc/acpi/ibm/fan`，程序只在内核模块已经明确开启 `fan_control=1` 时使用 `level auto`、`level 1..7` 和驱动列出的 `full-speed`。

## 8. COLORFUL P16 TA 24 专用恢复工具

仅用于已验证的该机型：

```bash
sudo /usr/libexec/linux-thermal-control/linux-thermal-recovery status
sudo /usr/libexec/linux-thermal-control/linux-thermal-recovery full-speed
sudo /usr/libexec/linux-thermal-control/linux-thermal-recovery automatic
```

不要在其他机型执行该工具。

开发树中还提供实机曲线闭环压力测试。它通过正式 D-Bus `ApplyZoneCurve` 接口依次发送默认曲线各区间的中点占空比，再通过 `GetStatus` 接收并校验模式、平滑占空比与 CPU/GPU 两路真实 RPM。测试会保存原曲线与模式，在退出、失败或中断时恢复：

```bash
sudo ./tests/HardwareCurveStressTest.sh --yes 4
```

该脚本仅允许在 DMI 与 `CLV0001/DCHU` 同时匹配的 `COLORFUL P16 TA 24` 上运行。`85–90°C` 和 `90–100°C` 两段在正常曲线控制中会被 85°C 安全全速覆盖；阶梯测试直接验证 EC 仍能接受它们对应的占空比。

## 9. 故障排查

### 服务运行但没有可控区域

```bash
sudo test -w /sys/class/hwmon/hwmonX/pwm1 && echo writable
cat /sys/class/hwmon/hwmonX/pwm1_enable
```

若只有 `fan1_input`，这就是只读转速通道。

### 自定义曲线触发全速

检查：

```bash
journalctl -u linux-thermal-daemon.service -b
cat /sys/class/hwmon/hwmonX/temp1_input
```

传感器缺失、读数失败或达到区域安全温度都会触发保护。

### 卸载前恢复自动

```bash
sudo systemctl stop linux-thermal-daemon.service
sudo apt remove linux-thermal-control
```

服务停止和卸载脚本都会尝试恢复自动模式，但固件行为因硬件而异；卸载后应观察风扇转速是否重新由 BIOS/驱动调节。

### GUI 显示“服务未连接”

安装脚本会刷新 systemd 与系统 D-Bus 配置，并启动守护进程。正常情况下只需通过 APT 安装本地软件包：

```bash
sudo apt install ./linux-thermal-control_VERSION_amd64.deb
```

诊断命令：

```bash
systemctl status linux-thermal-daemon.service
journalctl -u linux-thermal-daemon.service -b
busctl status org.linuxthermal.Control1
```

若日志出现 `Unexpected ACPI response`，请确认 `acpi_call` 已正常加载，并检查 `/proc/acpi/call` 是否返回完整响应。守护进程会在首个 NUL 字节处截断内核缓冲区内容，避免把残留字节当成 ACPI 返回值。
