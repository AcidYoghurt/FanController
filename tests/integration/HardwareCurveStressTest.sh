#!/usr/bin/env bash
set -euo pipefail

readonly service=org.linuxthermal.Control1
readonly object_path=/org/linuxthermal/Control1
readonly interface=org.linuxthermal.Control1
readonly zone_id='clevo:CLV0001:cooling-zone'
readonly expected_vendor=COLORFUL
readonly expected_product='P16 TA 24'

usage() {
  echo '用法：sudo ./tests/integration/HardwareCurveStressTest.sh --yes [每档反馈秒数，默认 4]' >&2
}

if (( EUID != 0 )); then
  usage
  exit 1
fi
if [[ "${1:-}" != --yes ]]; then
  echo '该测试会通过正式 D-Bus API 逐档控制真实风扇；确认后请传入 --yes。' >&2
  usage
  exit 2
fi

settle_seconds="${2:-4}"
if [[ ! "${settle_seconds}" =~ ^[1-9][0-9]*$ ]] ||
   (( settle_seconds > 30 )); then
  echo '每档反馈时间必须是 1–30 秒的整数。' >&2
  exit 2
fi

vendor="$(< /sys/class/dmi/id/sys_vendor)"
product="$(< /sys/class/dmi/id/product_name)"
acpi_path="$(cat /sys/bus/acpi/devices/CLV0001:00/path 2>/dev/null || true)"
if [[ "${vendor}" != "${expected_vendor}" ||
      "${product}" != "${expected_product}" ||
      ( "${acpi_path}" != '\_SB_.DCHU' &&
        "${acpi_path}" != '\_SB.DCHU' ) ]]; then
  echo "拒绝运行：仅支持已验证的 ${expected_vendor} ${expected_product} / CLV0001。" >&2
  exit 3
fi

command -v gdbus >/dev/null || {
  echo '缺少 gdbus，无法执行 D-Bus 闭环测试。' >&2
  exit 4
}
systemctl is-active --quiet linux-thermal-daemon.service || {
  echo 'linux-thermal-daemon.service 未运行。' >&2
  exit 4
}

dbus_call() {
  local method="$1"
  shift
  gdbus call --system --dest "${service}" --object-path "${object_path}" \
    --method "${interface}.${method}" "$@"
}

expect_true_reply() {
  local operation="$1" reply="$2"
  if [[ "${reply}" != '(true,)' ]]; then
    echo "${operation} 未被守护进程确认：${reply}" >&2
    return 1
  fi
}

status="$(dbus_call GetStatus)"
[[ "${status}" == *"'backendId': <'clevo-clv0001'>"* &&
   "${status}" == *"'status': <'ready'>"* &&
   "${status}" == *"'zoneId': <'${zone_id}'>"* ]] || {
  echo 'Clevo 后端或目标控制区域未处于 ready 状态。' >&2
  exit 5
}

original_mode="$(grep -oE "'mode': <'[^']+'>" <<<"${status}" | head -n 1 | cut -d "'" -f 4)"
minimum_temperature="$(grep -oE "'minimumTemperature': <[-0-9]+>" <<<"${status}" | head -n 1 | grep -oE '[-0-9]+')"
safety_temperature="$(grep -oE "'safetyTemperature': <[-0-9]+>" <<<"${status}" | head -n 1 | grep -oE '[-0-9]+')"
minimum_duty="$(grep -oE "'minimumDuty': <[-0-9]+>" <<<"${status}" | head -n 1 | grep -oE '[-0-9]+')"
if [[ -z "${original_mode}" || -z "${minimum_temperature}" ||
      -z "${safety_temperature}" || -z "${minimum_duty}" ]]; then
  echo '无法从 GetStatus 解析原模式或控制区域边界。' >&2
  exit 5
fi

original_curve_reply="$(dbus_call GetZoneCurve "${zone_id}")"
original_curve="${original_curve_reply#(}"
original_curve="${original_curve%,)}"
[[ "${original_curve}" == \[*\] ]] || {
  echo '无法保存原始曲线，拒绝开始测试。' >&2
  exit 5
}

restore_original_state() {
  local exit_status=$?
  trap - EXIT INT TERM
  dbus_call SetZoneCurve "${zone_id}" "${original_curve}" >/dev/null 2>&1 || true
  dbus_call SetMode "${original_mode}" >/dev/null 2>&1 ||
    dbus_call SetMode automatic >/dev/null 2>&1 || true
  exit "${exit_status}"
}
trap restore_original_state EXIT INT TERM

# Start the staged test from firmware control so the first 40% curve entry has
# no stale software-duty state. The EXIT trap restores the user's real mode.
expect_true_reply "SetMode(automatic)" "$(dbus_call SetMode automatic)"

# 每项对应默认曲线相邻节点的中点输出。每次都通过 ApplyZoneCurve 发送，
# 再通过 GetStatus 接收 mode/currentDuty/RPM，形成完整的应用层闭环。
readonly -a segment_names=(
  '35-40' '40-45' '45-50' '50-60' '60-65'
  '65-70' '70-80' '80-90-safety-covered' '90-100-safety-covered' 'safety'
)
readonly -a requested_duties=(40 40 43 48 55 65 75 85 95 100)

first_average=-1
last_average=-1
previous_average=-1
flat_stages=0
plateau_end=$((safety_temperature - 1))

printf 'segment,requested_duty,reported_duty,cpu_fan_rpm,gpu_fan_rpm,average_rpm\n'
for index in "${!requested_duties[@]}"; do
  requested_duty="${requested_duties[index]}"
  if (( requested_duty < minimum_duty )); then
    requested_duty="${minimum_duty}"
  fi
  curve="[<{'temperature': <${minimum_temperature}>, 'duty': <${requested_duty}>}>, <{'temperature': <${plateau_end}>, 'duty': <${requested_duty}>}>, <{'temperature': <${safety_temperature}>, 'duty': <100>}>]"

  reply="$(dbus_call ApplyZoneCurve "${zone_id}" "${curve}")"
  expect_true_reply "ApplyZoneCurve(${requested_duty}%)" "${reply}"
  feedback="$(dbus_call GetStatus)"
  reported_duty="$(grep -oE "'currentDuty': <[-0-9]+>" <<<"${feedback}" | head -n 1 | grep -oE '[-0-9]+')"
  for ((second = 0; second < settle_seconds; ++second)); do
    sleep 1
    feedback="$(dbus_call GetStatus)"
    reported_duty="$(grep -oE "'currentDuty': <[-0-9]+>" <<<"${feedback}" | head -n 1 | grep -oE '[-0-9]+')"
    if [[ -n "${reported_duty}" ]] &&
       (( reported_duty >= requested_duty - 3 && reported_duty <= requested_duty + 3 )); then
      break
    fi
  done

  [[ "${feedback}" == *"'mode': <'custom-curve'>"* ]] || {
    echo "${segment_names[index]}：回读模式不是 custom-curve。" >&2
    exit 6
  }
  if [[ -z "${reported_duty}" ]] ||
     (( reported_duty < requested_duty - 3 || reported_duty > requested_duty + 3 )); then
    echo "${segment_names[index]}：发送 ${requested_duty}%，回读 ${reported_duty:-N/A}%。" >&2
    exit 6
  fi

  mapfile -t rpms < <(
    grep -oE "'rpm': <[-0-9]+>" <<<"${feedback}" |
      grep -oE '[-0-9]+'
  )
  if (( ${#rpms[@]} < 2 )); then
    echo "${segment_names[index]}：GetStatus 未返回两路物理风扇 RPM。" >&2
    exit 6
  fi
  sum=0
  count=0
  for rpm in "${rpms[@]:0:2}"; do
    if (( rpm >= 0 )); then
      sum=$((sum + rpm))
      count=$((count + 1))
    fi
  done
  if (( count == 0 )); then
    echo "${segment_names[index]}：没有可用 RPM 反馈。" >&2
    exit 6
  fi
  average=$((sum / count))
  printf '%s,%s,%s,%s,%s,%s\n' \
    "${segment_names[index]}" "${requested_duty}" "${reported_duty}" \
    "${rpms[0]}" "${rpms[1]}" "${average}"

  if (( first_average < 0 )); then
    first_average="${average}"
  fi
  if (( previous_average >= 0 && average <= previous_average + 25 )); then
    flat_stages=$((flat_stages + 1))
  fi
  previous_average="${average}"
  last_average="${average}"
done

if (( last_average < first_average + 200 )); then
  echo "闭环失败：100% 平均转速 ${last_average} RPM 未明显高于 40% 的 ${first_average} RPM。" >&2
  exit 7
fi

echo "闭环通过：10 次 ApplyZoneCurve 均收到确认，GetStatus 占空比均进入迟滞容差，平均转速提升 $((last_average - first_average)) RPM。"
if (( flat_stages > 0 )); then
  echo "提示：${flat_stages} 个相邻阶段的平均 RPM 变化不超过 25 RPM，EC 可能存在量化或反馈延迟。"
fi
