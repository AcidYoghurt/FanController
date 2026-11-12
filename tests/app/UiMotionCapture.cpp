#include "UiMotionCapture.h"

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QTimer>

namespace {
QQuickItem *findVisualItem(QQuickItem *root, const QString &name) {
  if (root->objectName() == name)
    return root;
  for (auto *child : root->childItems()) {
    if (auto *match = findVisualItem(child, name))
      return match;
  }
  return nullptr;
}

QPoint itemCenter(QQuickItem *item) {
  return item->mapToScene(QPointF(item->width() / 2, item->height() / 2))
      .toPoint();
}

void clickItem(QQuickWindow *window, const QString &name) {
  auto *item = findVisualItem(window->contentItem(), name);
  if (!item) {
    QCoreApplication::exit(8);
    return;
  }
  QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(item));
}
} // namespace

void captureMotionPreview(QQuickWindow *window, const QString &directory) {
  if (!QDir().mkpath(directory)) {
    QCoreApplication::exit(9);
    return;
  }
  auto *frames = new QTimer(window);
  frames->setTimerType(Qt::PreciseTimer);
  QObject::connect(frames, &QTimer::timeout, window,
                   [window, directory, frame = 0]() mutable {
                     const auto path =
                         QStringLiteral("%1/frame-%2.png")
                             .arg(directory)
                             .arg(frame++, 4, 10, QLatin1Char('0'));
                     if (!window->grabWindow().save(path))
                       QCoreApplication::exit(10);
                   });
  frames->start(40);

  QTimer::singleShot(750, window, [window] {
    auto *button =
        window->findChild<QQuickItem *>(QStringLiteral("mode-full-speed"));
    if (button)
      QTest::mouseMove(window, itemCenter(button));
  });
  QTimer::singleShot(1300, window, [window] {
    clickItem(window, QStringLiteral("mode-full-speed"));
  });
  QTimer::singleShot(2200, window, [window] {
    clickItem(window, QStringLiteral("mode-custom-curve"));
  });
  QTimer::singleShot(3000, window, [window] {
    auto *page = window->findChild<QObject *>(QStringLiteral("controlPage"));
    auto *flick = page ? qvariant_cast<QObject *>(page->property("contentItem"))
                       : nullptr;
    if (flick)
      flick->setProperty("contentY",
                         qMax(0., flick->property("contentHeight").toDouble() -
                                      flick->property("height").toDouble()));
  });
  QTimer::singleShot(3800, window, [window] {
    auto *input =
        findVisualItem(window->contentItem(), QStringLiteral("curveDuty4"));
    if (!input) {
      QCoreApplication::exit(11);
      return;
    }
    const int before = input->property("value").toInt();
    const auto increment =
        input->mapToScene(QPointF(input->width() - 16, input->height() / 2))
            .toPoint();
    for (int count = 0; count < 5; ++count)
      QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, increment);
    if (input->property("value").toInt() <= before)
      QCoreApplication::exit(12);
  });
  QTimer::singleShot(4800, window, [window] {
    clickItem(window, QStringLiteral("infoNavigationButton"));
  });
  QTimer::singleShot(5800, window, [window] {
    clickItem(window, QStringLiteral("controlNavigationButton"));
  });
  QTimer::singleShot(7000, window, [frames] {
    frames->stop();
    QCoreApplication::quit();
  });
}
