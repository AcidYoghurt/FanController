#pragma once

#include <QString>

class QQuickWindow;

// 由私有 mock 会话调用，录制真实交互产生的动画帧。
void captureMotionPreview(QQuickWindow *window, const QString &directory);
