#pragma once

#include <QDBusContext>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class FanController;

// 将 FanController 公开为系统 D-Bus 服务，并对所有改变硬件状态的调用
// 执行 Polkit 授权。只读方法不触发授权交互。
class FanService final : public QObject, protected QDBusContext {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.linuxthermal.Control1")

public:
  explicit FanService(FanController *controller, QObject *parent = nullptr);

public slots:
  // 返回控制器的完整状态快照。
  [[nodiscard]] QVariantMap GetStatus() const;
  // 返回主控制区域的曲线，适合只处理一个区域的调用方。
  [[nodiscard]] QVariantList GetCurve() const;
  // 返回全部控制区域的曲线及修订信息。
  [[nodiscard]] QVariantList GetCurveProfiles() const;
  // 返回 `zoneId` 的当前曲线。
  [[nodiscard]] QVariantList GetZoneCurve(const QString &zoneId) const;
  [[nodiscard]] QString GetVersion() const;
  // 将全部区域切换到 `mode`。
  [[nodiscard]] bool SetMode(const QString &mode);
  // 保存主区域曲线，但不改变模式。
  [[nodiscard]] bool SetCurve(const QVariantList &curve);
  // 保存 `zoneId` 的曲线，但不改变模式。
  [[nodiscard]] bool SetZoneCurve(const QString &zoneId,
                                  const QVariantList &curve);
  // 保存主区域曲线并启用自定义曲线模式。
  [[nodiscard]] bool ApplyCurve(const QVariantList &curve);
  // 保存 `zoneId` 的曲线并启用自定义曲线模式。
  [[nodiscard]] bool ApplyZoneCurve(const QString &zoneId,
                                    const QVariantList &curve);
  // 恢复主区域默认曲线。
  [[nodiscard]] bool ResetCurve();
  // 恢复 `zoneId` 的默认曲线。
  [[nodiscard]] bool ResetZoneCurve(const QString &zoneId);
  // 在模拟后端中启用或解除传感器故障；实机后端拒绝该调用。
  [[nodiscard]] bool MockSetSensorFailure(bool enabled);
  // 使模拟后端的下一条控制命令失败；实机后端拒绝该调用。
  [[nodiscard]] bool MockFailNextCommand();

signals:
  void StatusChanged(const QVariantMap &status);
  void ModeChanged(const QString &mode);
  void CurveChanged(const QVariantList &curve);
  void CurvesChanged(const QVariantList &profiles);

private:
  [[nodiscard]] bool authorizeCaller(QString *error) const;

  FanController *m_controller;  // 不持有；守护进程保证控制器后析构。
};
