#include "SingleInstanceController.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

namespace {
// GUI 单实例协议只存在于当前用户的会话总线上，不与系统守护进程共用名称。
constexpr auto ServiceName = "org.linuxthermal.Control.Gui";
constexpr auto ObjectPath = "/org/linuxthermal/Control/Gui";
constexpr auto InterfaceName = "org.linuxthermal.Control.Gui1";
} // namespace

SingleInstanceController::SingleInstanceController(QObject *parent)
    : QObject(parent) {
  auto bus = QDBusConnection::sessionBus();
  if (!bus.isConnected())
    return;

  if (!bus.registerObject(QString::fromLatin1(ObjectPath), this,
                          QDBusConnection::ExportAllSlots))
    return;

  if (!bus.registerService(QString::fromLatin1(ServiceName))) {
    bus.unregisterObject(QString::fromLatin1(ObjectPath));
    m_primary = false;
    return;
  }

  m_serviceRegistered = true;
}

SingleInstanceController::~SingleInstanceController() {
  if (!m_serviceRegistered)
    return;
  auto bus = QDBusConnection::sessionBus();
  bus.unregisterObject(QString::fromLatin1(ObjectPath));
  bus.unregisterService(QString::fromLatin1(ServiceName));
}

bool SingleInstanceController::activatePrimary() {
  if (m_primary)
    return false;

  QDBusInterface primary(QString::fromLatin1(ServiceName),
                         QString::fromLatin1(ObjectPath),
                         QString::fromLatin1(InterfaceName),
                         QDBusConnection::sessionBus());
  primary.setTimeout(3000);
  const QDBusReply<bool> reply = primary.call(QStringLiteral("Activate"));
  return reply.isValid() && reply.value();
}

bool SingleInstanceController::takePendingActivation() noexcept {
  const bool pending = m_activationPending;
  m_activationPending = false;
  return pending;
}

bool SingleInstanceController::Activate() {
  if (!m_primary)
    return false;
  m_activationPending = true;
  emit activationRequested();
  return true;
}
