#pragma once

#include "ThermalTypes.h"
#include "backends/IFanBackend.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <vector>

class MockBackend;

// 聚合所有硬件后端，并实现模式切换、曲线持久化、温度滤波和安全保护。
// 所有方法均在守护进程主线程调用；后端列表及运行时控制状态不跨线程共享。
class FanController final : public QObject {
  Q_OBJECT

public:
  enum class Mode { Automatic, FullSpeed, CustomCurve, Unknown };
  Q_ENUM(Mode)

  explicit FanController(bool mockMode = false, QString curvePath = {},
                         QObject *parent = nullptr);

  // 探测后端、加载曲线并建立初始状态；没有可用遥测时返回 false。
  [[nodiscard]] bool initialize(QString *error = nullptr);
  // 启动周期遥测和自定义曲线控制。
  void startTelemetry();
  // 返回适合通过 D-Bus 传输的完整状态快照。
  [[nodiscard]] QVariantMap status() const;
  // 返回主控制区域的曲线；没有控制区域时返回空列表。
  [[nodiscard]] QVariantList curve() const;
  // 返回 `zoneId` 的曲线；区域不存在时返回空列表。
  [[nodiscard]] QVariantList curveForZone(const QString &zoneId) const;
  // 原子地把所有支持的控制区域切换到 `mode`。
  [[nodiscard]] bool setMode(Mode mode, QString *error = nullptr);
  // 验证并保存主控制区域的曲线，但不改变当前模式。
  [[nodiscard]] bool setCurve(const QVariantList &curve,
                              QString *error = nullptr);
  // 验证并保存 `zoneId` 的曲线，但不改变当前模式。
  [[nodiscard]] bool setCurveForZone(const QString &zoneId,
                                     const QVariantList &curve,
                                     QString *error = nullptr);
  // 将主控制区域恢复为按其硬件边界归一化后的默认曲线。
  [[nodiscard]] bool resetCurve(QString *error = nullptr);
  // 将 `zoneId` 恢复为按其硬件边界归一化后的默认曲线。
  [[nodiscard]] bool resetCurveForZone(const QString &zoneId,
                                       QString *error = nullptr);
  // 尽可能把所有区域恢复为固件自动控制。
  [[nodiscard]] bool restoreAuto(QString *error = nullptr);
  [[nodiscard]] Mode mode() const noexcept { return m_mode; }
  [[nodiscard]] bool isMockMode() const noexcept { return m_mockMode; }
  [[nodiscard]] bool setMockSensorFailure(bool enabled);
  [[nodiscard]] bool setMockSensorTemperature(const QString &sensorId,
                                              double temperature);
  [[nodiscard]] bool setMockCommandFailure();

  // 将公开的模式名称解析为内部枚举；未知名称返回 Mode::Unknown。
  [[nodiscard]] static Mode modeFromString(QStringView value);
  // 从 `sysfsRoot` 读取安全启动状态，并结合系统 MOK 信息生成状态映射。
  [[nodiscard]] static QVariantMap detectSecureBoot(QString sysfsRoot = {});
  // 将 mokutil 输出解析为稳定的密钥注册状态，供测试和状态接口复用。
  [[nodiscard]] static QVariantMap parseMokEnrollmentResult(
      const QByteArray &testKeyOutput, const QByteArray &pendingOutput,
      bool certificateExists);
  [[nodiscard]] static QString modeToString(Mode mode);
  [[nodiscard]] static QVariantList defaultCurve();
  // 按通用控制区域边界验证 `curve`，并在失败时填写 `error`。
  [[nodiscard]] static bool validateCurve(const QVariantList &curve,
                                          QString *error = nullptr);
  // 对 `curve` 进行分段线性插值，并将结果限制在最低安全占空比以上。
  [[nodiscard]] static int dutyForTemperature(const QVariantList &curve,
                                              double temperature,
                                              int minimumDuty = 40);

signals:
  void statusChanged(const QVariantMap &status);
  void modeChanged(const QString &mode);
  void curveChanged(const QVariantList &curve);
  void curvesChanged(const QVariantList &profiles);

public slots:
  // 睡眠前恢复自动模式，唤醒后重新探测并恢复运行时状态。
  void prepareForSleep(bool sleeping);

private slots:
  void refreshTelemetry();

private:
  [[nodiscard]] QList<Thermal::TemperatureSensor> sensors() const;
  [[nodiscard]] QList<Thermal::FanDevice> fans() const;
  [[nodiscard]] QList<Thermal::FanZone> zones() const;
  [[nodiscard]] IFanBackend *backendForZone(const QString &zoneId) const;
  [[nodiscard]] Thermal::FanZone zoneById(const QString &zoneId) const;
  [[nodiscard]] QVariantList defaultCurveForZone(const Thermal::FanZone &zone) const;
  [[nodiscard]] bool validateCurveForZone(const Thermal::FanZone &zone,
                                          const QVariantList &curve,
                                          QString *error) const;
  [[nodiscard]] bool applyModeToZones(Mode mode, QString *error);
  [[nodiscard]] bool applyCustomControl(QString *error = nullptr);
  [[nodiscard]] bool loadCurves();
  [[nodiscard]] bool saveCurves(QString *error = nullptr) const;
  [[nodiscard]] QVariantList curveProfiles() const;
  [[nodiscard]] QString primaryZoneId() const;
  [[nodiscard]] static QVariantList normalizedCurve(const QVariantList &curve);
  void refreshMokEnrollment();

  QTimer m_telemetryTimer;
  std::vector<std::unique_ptr<IFanBackend>> m_backends;
  // 不持有；非空时指向 m_backends 中的模拟后端。
  MockBackend *m_mockBackend = nullptr;
  // 下列运行时映射均以稳定的 zoneId 为键。
  QHash<QString, QVariantList> m_curves;
  QHash<QString, int> m_curveRevisions;
  QHash<QString, double> m_controlTemperatures;
  // 每个列表保存最近八次有效温度，用于控制平均值。
  QHash<QString, QList<double>> m_temperatureSamples;
  // -1 表示区域当前由固件自动控制，软件没有已知占空比。
  QHash<QString, int> m_zoneDuties;
  QHash<QString, QElapsedTimer> m_lastDutyWrite;
  QHash<QString, QElapsedTimer> m_lastDutyDecrease;
  QString m_curvePath;
  Mode m_mode = Mode::Automatic;
  bool m_mockMode = false;
  bool m_safetyOverride = false;
  bool m_partialCapability = false;
  bool m_preparingForSleep = false;
  QString m_state = QStringLiteral("probing");
  QString m_lastErrorCode;
  QString m_lastError;
  QString m_mokState = QStringLiteral("unknown");
  bool m_mokKnown = false;
  bool m_mokEnrolled = false;
};
