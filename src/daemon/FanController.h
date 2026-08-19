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

class FanController final : public QObject {
  Q_OBJECT

public:
  enum class Mode { Automatic, FullSpeed, CustomCurve, Unknown };
  Q_ENUM(Mode)

  explicit FanController(bool mockMode = false, QString curvePath = {},
                         QObject *parent = nullptr);

  [[nodiscard]] bool initialize(QString *error = nullptr);
  void startTelemetry();
  [[nodiscard]] QVariantMap status() const;
  [[nodiscard]] QVariantList curve() const;
  [[nodiscard]] QVariantList curveForZone(const QString &zoneId) const;
  [[nodiscard]] bool setMode(Mode mode, QString *error = nullptr);
  [[nodiscard]] bool setCurve(const QVariantList &curve,
                              QString *error = nullptr);
  [[nodiscard]] bool setCurveForZone(const QString &zoneId,
                                     const QVariantList &curve,
                                     QString *error = nullptr);
  [[nodiscard]] bool resetCurve(QString *error = nullptr);
  [[nodiscard]] bool resetCurveForZone(const QString &zoneId,
                                       QString *error = nullptr);
  [[nodiscard]] bool restoreAuto(QString *error = nullptr);
  [[nodiscard]] Mode mode() const noexcept { return m_mode; }
  [[nodiscard]] bool isMockMode() const noexcept { return m_mockMode; }
  [[nodiscard]] bool setMockSensorFailure(bool enabled);
  [[nodiscard]] bool setMockCommandFailure();

  [[nodiscard]] static Mode modeFromString(QStringView value);
  [[nodiscard]] static QVariantMap detectSecureBoot(QString sysfsRoot = {});
  [[nodiscard]] static QString modeToString(Mode mode);
  [[nodiscard]] static QVariantList defaultCurve();
  [[nodiscard]] static bool validateCurve(const QVariantList &curve,
                                          QString *error = nullptr);
  [[nodiscard]] static int dutyForTemperature(const QVariantList &curve,
                                              double temperature,
                                              int minimumDuty = 40);

signals:
  void statusChanged(const QVariantMap &status);
  void modeChanged(const QString &mode);
  void curveChanged(const QVariantList &curve);
  void curvesChanged(const QVariantList &profiles);

public slots:
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

  QTimer m_telemetryTimer;
  std::vector<std::unique_ptr<IFanBackend>> m_backends;
  MockBackend *m_mockBackend = nullptr;
  QHash<QString, QVariantList> m_curves;
  QHash<QString, int> m_curveRevisions;
  QHash<QString, double> m_controlTemperatures;
  QHash<QString, int> m_zoneDuties;
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
};
