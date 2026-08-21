#include "FanService.h"

#include "Constants.h"
#include "DbusUtils.h"
#include "FanController.h"

#include <QDBusError>
#include <QDBusMessage>
#include <QProcess>

FanService::FanService(FanController *controller, QObject *parent)
    : QObject(parent), m_controller(controller) {
  connect(m_controller, &FanController::statusChanged, this,
          &FanService::StatusChanged);
  connect(m_controller, &FanController::modeChanged, this,
          &FanService::ModeChanged);
  connect(m_controller, &FanController::curveChanged, this,
          &FanService::CurveChanged);
  connect(m_controller, &FanController::curvesChanged, this,
          &FanService::CurvesChanged);
}

QVariantMap FanService::GetStatus() const { return m_controller->status(); }

QVariantList FanService::GetCurve() const { return m_controller->curve(); }

QVariantList FanService::GetCurveProfiles() const {
  return m_controller->status().value(QStringLiteral("curveProfiles")).toList();
}

QVariantList FanService::GetZoneCurve(const QString &zoneId) const {
  return m_controller->curveForZone(zoneId);
}

QString FanService::GetVersion() const {
  return QString::fromLatin1(LinuxThermal::Constants::Version);
}

bool FanService::SetMode(const QString &mode) {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }

  const auto targetMode = FanController::modeFromString(mode);
  if (targetMode == FanController::Mode::Unknown) {
    sendErrorReply(QDBusError::InvalidArgs,
                   QStringLiteral("Only automatic, full-speed and custom-curve are supported."));
    return false;
  }

  if (!m_controller->setMode(targetMode, &error)) {
    sendErrorReply(QDBusError::Failed, error);
    return false;
  }
  return true;
}

bool FanService::SetCurve(const QVariantList &curve) {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }
  if (!m_controller->setCurve(LinuxThermal::Dbus::unwrapList(curve), &error)) {
    sendErrorReply(QDBusError::InvalidArgs, error);
    return false;
  }
  return true;
}


bool FanService::SetZoneCurve(const QString &zoneId,
                              const QVariantList &curve) {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }
  if (!m_controller->setCurveForZone(
          zoneId, LinuxThermal::Dbus::unwrapList(curve), &error)) {
    sendErrorReply(QDBusError::InvalidArgs, error);
    return false;
  }
  return true;
}

bool FanService::ApplyCurve(const QVariantList &curve) {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }
  if (!m_controller->setCurve(LinuxThermal::Dbus::unwrapList(curve), &error)) {
    sendErrorReply(QDBusError::InvalidArgs, error);
    return false;
  }
  if (!m_controller->setMode(FanController::Mode::CustomCurve, &error)) {
    sendErrorReply(QDBusError::Failed, error);
    return false;
  }
  return true;
}


bool FanService::ApplyZoneCurve(const QString &zoneId,
                                const QVariantList &curve) {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }
  if (!m_controller->setCurveForZone(
          zoneId, LinuxThermal::Dbus::unwrapList(curve), &error)) {
    sendErrorReply(QDBusError::InvalidArgs, error);
    return false;
  }
  if (!m_controller->setMode(FanController::Mode::CustomCurve, &error)) {
    sendErrorReply(QDBusError::Failed, error);
    return false;
  }
  return true;
}

bool FanService::ResetCurve() {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }
  if (!m_controller->resetCurve(&error)) {
    sendErrorReply(QDBusError::Failed, error);
    return false;
  }
  return true;
}


bool FanService::ResetZoneCurve(const QString &zoneId) {
  QString error;
  if (!authorizeCaller(&error)) {
    sendErrorReply(QDBusError::AccessDenied, error);
    return false;
  }
  if (!m_controller->resetCurveForZone(zoneId, &error)) {
    sendErrorReply(QDBusError::Failed, error);
    return false;
  }
  return true;
}

bool FanService::MockSetSensorFailure(bool enabled) {
  if (!m_controller->isMockMode()) {
    sendErrorReply(QDBusError::NotSupported,
                   QStringLiteral("该方法仅供开发模拟后端使用。"));
    return false;
  }
  return m_controller->setMockSensorFailure(enabled);
}

bool FanService::MockFailNextCommand() {
  if (!m_controller->isMockMode()) {
    sendErrorReply(QDBusError::NotSupported,
                   QStringLiteral("该方法仅供开发模拟后端使用。"));
    return false;
  }
  return m_controller->setMockCommandFailure();
}

bool FanService::authorizeCaller(QString *error) const {
  if (m_controller->isMockMode())
    return true;

  if (!calledFromDBus()) {
    if (error)
      *error = QStringLiteral("拒绝非 D-Bus 控制请求。");
    return false;
  }

  // pkcheck 直接解析系统总线唯一名称，可避免信任调用方提交的 PID 或 UID。
  const auto busName = message().service();
  if (busName.isEmpty()) {
    if (error)
      *error = QStringLiteral("无法识别请求进程。");
    return false;
  }

  QProcess process;
  process.start(QStringLiteral("/usr/bin/pkcheck"),
                {
                    QStringLiteral("--action-id"),
                    QString::fromLatin1(LinuxThermal::Constants::PolkitAction),
                    QStringLiteral("--system-bus-name"),
                    busName,
                    QStringLiteral("--allow-user-interaction"),
                });

  if (!process.waitForStarted(2000)) {
    if (error)
      *error = QStringLiteral("无法启动 Polkit 授权检查。");
    return false;
  }
  if (!process.waitForFinished(120000)) {
    process.kill();
    process.waitForFinished();
    if (error)
      *error = QStringLiteral("管理员授权超时。");
    return false;
  }

  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    const auto detail =
        QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (error) {
      *error = detail.isEmpty() ? QStringLiteral("管理员授权被拒绝或取消。")
                                : detail;
    }
    return false;
  }
  return true;
}
