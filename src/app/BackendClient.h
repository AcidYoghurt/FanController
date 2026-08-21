#pragma once

#include "InventoryModels.h"

#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <functional>

class BackendClient final : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(FanBackend)
  QML_SINGLETON
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged FINAL)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
  Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged FINAL)
  Q_PROPERTY(QString state READ state NOTIFY stateChanged FINAL)
  Q_PROPERTY(QString mode READ mode NOTIFY modeChanged FINAL)
  Q_PROPERTY(bool partialCapability READ partialCapability NOTIFY stateChanged FINAL)
  Q_PROPERTY(bool safetyOverride READ safetyOverride NOTIFY safetyChanged FINAL)
  Q_PROPERTY(QString safetyMessage READ safetyMessage NOTIFY safetyChanged FINAL)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
  Q_PROPERTY(QString daemonVersion READ daemonVersion NOTIFY versionChanged FINAL)
  Q_PROPERTY(QString systemName READ systemName NOTIFY systemNameChanged FINAL)
  Q_PROPERTY(QString secureBootState READ secureBootState NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(QString secureBootRequirement READ secureBootRequirement NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool secureBootKnown READ secureBootKnown NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool secureBootEnabled READ secureBootEnabled NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool secureBootDisabled READ secureBootDisabled NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool secureBootEnrollmentRequired READ secureBootEnrollmentRequired NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(QString secureBootMokState READ secureBootMokState NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool secureBootMokKnown READ secureBootMokKnown NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool secureBootMokEnrolled READ secureBootMokEnrolled NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(SensorListModel *sensors READ sensors CONSTANT FINAL)
  Q_PROPERTY(FanListModel *fans READ fans CONSTANT FINAL)
  Q_PROPERTY(ZoneListModel *zones READ zones CONSTANT FINAL)
  Q_PROPERTY(BackendListModel *backends READ backends CONSTANT FINAL)
  Q_PROPERTY(CurvePointModel *curvePoints READ curvePoints CONSTANT FINAL)
  Q_PROPERTY(QString selectedZoneId READ selectedZoneId NOTIFY selectedZoneChanged FINAL)
  Q_PROPERTY(QString selectedZoneName READ selectedZoneName NOTIFY selectedZoneChanged FINAL)
  Q_PROPERTY(int minimumDuty READ minimumDuty NOTIFY selectedZoneChanged FINAL)
  Q_PROPERTY(int safetyTemperature READ safetyTemperature NOTIFY selectedZoneChanged FINAL)
  Q_PROPERTY(int maximumTemperature READ maximumTemperature NOTIFY selectedZoneChanged FINAL)
  Q_PROPERTY(bool curveSupported READ curveSupported NOTIFY selectedZoneChanged FINAL)
  Q_PROPERTY(double selectedTemperature READ selectedTemperature NOTIFY selectedTelemetryChanged FINAL)
  Q_PROPERTY(int selectedDuty READ selectedDuty NOTIFY selectedTelemetryChanged FINAL)
  Q_PROPERTY(bool curveConflict READ curveConflict NOTIFY curveConflictChanged FINAL)
  Q_PROPERTY(bool automaticAvailable READ automaticAvailable NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool fullSpeedAvailable READ fullSpeedAvailable NOTIFY inventoryChanged FINAL)
  Q_PROPERTY(bool customCurveAvailable READ customCurveAvailable NOTIFY inventoryChanged FINAL)

public:
  explicit BackendClient(QObject *parent = nullptr);

  [[nodiscard]] bool connected() const noexcept { return m_connected; }
  [[nodiscard]] bool busy() const noexcept { return m_busy; }
  [[nodiscard]] bool refreshing() const noexcept { return m_refreshing; }
  [[nodiscard]] QString state() const { return m_status.value(QStringLiteral("state"), QStringLiteral("disconnected")).toString(); }
  [[nodiscard]] QString mode() const { return m_status.value(QStringLiteral("mode"), QStringLiteral("unknown")).toString(); }
  [[nodiscard]] bool partialCapability() const { return m_status.value(QStringLiteral("partialCapability")).toBool(); }
  [[nodiscard]] bool safetyOverride() const { return m_status.value(QStringLiteral("safetyOverride")).toBool(); }
  [[nodiscard]] QString safetyMessage() const;
  [[nodiscard]] QString lastError() const { return m_lastError; }
  [[nodiscard]] QString daemonVersion() const { return m_status.value(QStringLiteral("version")).toString(); }
  [[nodiscard]] QString systemName() const;
  [[nodiscard]] QString secureBootState() const;
  [[nodiscard]] QString secureBootRequirement() const;
  [[nodiscard]] bool secureBootKnown() const;
  [[nodiscard]] bool secureBootEnabled() const;
  [[nodiscard]] bool secureBootDisabled() const;
  [[nodiscard]] bool secureBootEnrollmentRequired() const;
  [[nodiscard]] QString secureBootMokState() const;
  [[nodiscard]] bool secureBootMokKnown() const;
  [[nodiscard]] bool secureBootMokEnrolled() const;
  [[nodiscard]] SensorListModel *sensors() { return &m_sensors; }
  [[nodiscard]] FanListModel *fans() { return &m_fans; }
  [[nodiscard]] ZoneListModel *zones() { return &m_zones; }
  [[nodiscard]] BackendListModel *backends() { return &m_backends; }
  [[nodiscard]] CurvePointModel *curvePoints() { return &m_curvePoints; }
  [[nodiscard]] QString selectedZoneId() const { return m_selectedZoneId; }
  [[nodiscard]] QString selectedZoneName() const;
  [[nodiscard]] int minimumDuty() const;
  [[nodiscard]] int safetyTemperature() const;
  [[nodiscard]] int maximumTemperature() const;
  [[nodiscard]] bool curveSupported() const;
  [[nodiscard]] double selectedTemperature() const;
  [[nodiscard]] int selectedDuty() const;
  [[nodiscard]] bool curveConflict() const noexcept { return m_curveConflict; }
  [[nodiscard]] bool automaticAvailable() const;
  [[nodiscard]] bool fullSpeedAvailable() const;
  [[nodiscard]] bool customCurveAvailable() const;

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void setMode(const QString &mode);
  Q_INVOKABLE bool canSetMode(const QString &mode) const;
  Q_INVOKABLE bool selectZone(const QString &zoneId);
  Q_INVOKABLE void setCurvePoint(int row, int temperature, int duty);
  Q_INVOKABLE void saveCurve();
  Q_INVOKABLE void applyCurve();
  Q_INVOKABLE void resetCurve();
  Q_INVOKABLE void discardCurveEdits();
  Q_INVOKABLE void clearError();

signals:
  void connectedChanged();
  void busyChanged();
  void refreshingChanged();
  void stateChanged();
  void modeChanged();
  void safetyChanged();
  void versionChanged();
  void systemNameChanged();
  void inventoryChanged();
  void selectedZoneChanged();
  void selectedTelemetryChanged();
  void curveConflictChanged();
  void lastErrorChanged();
  void modeRequestFinished(bool success, const QString &mode);
  void curveRequestFinished(bool success);

private slots:
  void handleRemoteStatusChanged(const QVariantMap &status);
  void handleRemoteCurvesChanged(const QVariantList &profiles);
  void handleServiceOwnerChanged(const QString &name, const QString &oldOwner,
                                 const QString &newOwner);

private:
  void setBusy(bool value);
  void setRefreshing(bool value);
  void requestRefresh(bool visible);
  void setConnected(bool value);
  void setError(const QString &message);
  void applyStatus(const QVariantMap &status);
  void loadSelectedCurve(bool force);
  [[nodiscard]] QVariantMap selectedProfile() const;
  [[nodiscard]] QVariantMap secureBootMap() const;
  void watchBooleanReply(const QDBusPendingCall &call,
                         const std::function<void(bool)> &finished);

  QDBusInterface m_interface;
  QTimer m_refreshTimer;
  QVariantMap m_status;
  QVariantList m_profiles;
  QVariantList m_curveBaseline;
  int m_selectedCurveRevision = 0;
  SensorListModel m_sensors;
  FanListModel m_fans;
  ZoneListModel m_zones;
  BackendListModel m_backends;
  CurvePointModel m_curvePoints;
  QString m_selectedZoneId;
  QString m_lastError;
  bool m_connected = false;
  bool m_busy = false;
  bool m_refreshInFlight = false;
  bool m_refreshing = false;
  bool m_curveConflict = false;
};
