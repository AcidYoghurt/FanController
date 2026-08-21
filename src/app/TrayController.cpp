#include "TrayController.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QWindow>

#include <utility>

TrayController::TrayController(QApplication *application, QIcon icon,
                               bool forceAvailableForTests, QObject *parent)
    : QObject(parent), m_application(application),
      m_trayIcon(new QSystemTrayIcon(std::move(icon), this)),
      m_menu(new QMenu()),
      m_showAction(m_menu->addAction(tr("显示主窗口"))),
      m_hideAction(m_menu->addAction(tr("隐藏主窗口"))),
      m_quitAction(nullptr),
      m_trayAvailable(forceAvailableForTests ||
                      QSystemTrayIcon::isSystemTrayAvailable()) {
  m_menu->addSeparator();
  m_quitAction = m_menu->addAction(tr("退出"));
  m_trayIcon->setContextMenu(m_menu);
  m_trayIcon->setToolTip(tr("Linux Thermal Control"));

  connect(m_showAction, &QAction::triggered, this,
          &TrayController::showWindow);
  connect(m_hideAction, &QAction::triggered, this,
          &TrayController::hideWindow);
  connect(m_quitAction, &QAction::triggered, this,
          &TrayController::quitApplication);
  connect(m_trayIcon, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
              updateActions();
              m_menu->popup(QCursor::pos());
            } else if (reason == QSystemTrayIcon::DoubleClick) {
              showWindow();
            }
          });

  if (m_trayAvailable)
    m_trayIcon->show();
  updateActions();
}

TrayController::~TrayController() {
  m_trayIcon->setContextMenu(nullptr);
  delete m_menu;
}

void TrayController::setWindow(QWindow *window) {
  if (m_window == window)
    return;
  if (m_window)
    m_window->removeEventFilter(this);
  m_window = window;
  if (m_window) {
    m_window->installEventFilter(this);
    connect(m_window, &QWindow::visibleChanged, this,
            &TrayController::updateActions, Qt::UniqueConnection);
  }
  updateActions();
}

void TrayController::setQuitPreparation(std::function<bool()> preparation) {
  m_quitPreparation = std::move(preparation);
}

void TrayController::showWindow() {
  if (!m_window)
    return;
  m_window->show();
  if (m_window->visibility() == QWindow::Minimized)
    m_window->setVisibility(QWindow::Windowed);
  m_window->raise();
  m_window->requestActivate();
  updateActions();
}

void TrayController::hideWindow() {
  if (!m_window)
    return;
  m_window->hide();
  updateActions();
}

void TrayController::quitApplication() {
  if (m_quitRequested)
    return;
  m_quitRequested = true;
  m_quitAction->setEnabled(false);
  if (m_quitPreparation && !m_quitPreparation()) {
    m_quitRequested = false;
    m_quitAction->setEnabled(true);
    return;
  }
  m_trayIcon->hide();
  m_application->quit();
}

bool TrayController::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_window && event->type() == QEvent::Close &&
      !m_quitRequested && m_trayAvailable) {
    static_cast<QCloseEvent *>(event)->ignore();
    hideWindow();
    return true;
  }
  return QObject::eventFilter(watched, event);
}

void TrayController::updateActions() {
  const bool visible = m_window && m_window->isVisible();
  m_showAction->setEnabled(m_window && !visible);
  m_hideAction->setEnabled(m_window && visible);
}
