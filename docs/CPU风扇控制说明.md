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
sudo apt install ./dist/linux-thermal-control_0.5.3-1_amd64.deb
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
- `acpi-call-dkms` 仅是 Clevo 专用后端的建议依赖，不是通用 hwmon 的强制依赖。

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

模式切换会触发 Polkit：

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

## 5. NVIDIA、AMD 与 Intel GPU

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

联想支持页面中的 `ThinkBook 14 G7 AHP` 与本机 DMI 的 `ThinkBook 14 G7+ AHP (21U4)` 是两个不同机型。本机由 `ideapad_laptop` 提供：

```text
/sys/firmware/acpi/platform_profile
/sys/firmware/acpi/platform_profile_choices
/sys/bus/acpi/devices/VPC2004:00/physical_node/fan_mode
```

实测三种 platform profile 均可安全切换并恢复，但 `fan_mode` 在写入内核允许的值 `4` 后仍读回 `133`，也没有 RPM 可用于验证。因此程序只显示该风扇由固件管理，不把 profile 或 fan_mode 误报为“满速”。

ThinkPad 与 ThinkBook/IdeaPad 不使用同一控制接口。真正的 ThinkPad 若提供 `/proc/acpi/ibm/fan`，程序只在内核模块已经明确开启 `fan_control=1` 时使用 `level auto`、`level 1..7` 和驱动列出的 `full-speed`。

## 8. COLORFUL P16 TA 24 专用恢复工具

仅用于已验证的该机型：

```bash
sudo /usr/libexec/linux-thermal-control/linux-thermal-recovery status
sudo /usr/libexec/linux-thermal-control/linux-thermal-recovery full-speed
sudo /usr/libexec/linux-thermal-control/linux-thermal-recovery automatic
```

不要在其他机型执行该工具。

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
