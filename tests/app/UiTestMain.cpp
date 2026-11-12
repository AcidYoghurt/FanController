#include <QQuickStyle>
#include <QSettings>
#include <QTemporaryDir>
#include <QtQuickTest>

class UiTestSetup : public QObject {
  Q_OBJECT
public slots:
  void applicationAvailable() {
    QCoreApplication::setOrganizationName(QStringLiteral("LinuxThermalTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ui-tests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       m_config.path());
    QQuickStyle::setStyle(QStringLiteral("Basic"));
  }

private:
  QTemporaryDir m_config;
};

QUICK_TEST_MAIN_WITH_SETUP(thermal_ui, UiTestSetup)
#include "UiTestMain.moc"
