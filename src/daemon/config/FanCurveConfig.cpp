#include "FanCurveConfig.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

namespace {
bool parseInteger(const QString &text, int *value) {
  bool ok = false;
  const int parsed = text.trimmed().toInt(&ok);
  if (ok)
    *value = parsed;
  return ok;
}

bool parseField(const QString &line, const QString &field, int *value) {
  const auto prefix = field + u':';
  if (!line.startsWith(prefix))
    return false;
  return parseInteger(line.sliced(prefix.size()), value);
}

bool appendPoint(QVariantList *points, QVariantMap *point, QString *error,
                 int lineNumber) {
  if (point->isEmpty())
    return true;
  if (!point->contains(QStringLiteral("temperature")) ||
      !point->contains(QStringLiteral("duty"))) {
    if (error)
      *error = QStringLiteral("Incomplete curve point near YAML line %1.")
                   .arg(lineNumber);
    return false;
  }
  points->push_back(*point);
  point->clear();
  return true;
}
} // namespace

QString FanCurveConfig::editableConfigPath() {
  const auto overridePath =
      qEnvironmentVariable("LINUX_THERMAL_DEFAULT_CURVE_CONFIG");
  if (!overridePath.isEmpty())
    return overridePath;

  const auto installedPath =
      QStringLiteral("/etc/linux-thermal-control/default-fan-curve.yaml");
  const auto installedBackup =
      QFileInfo(installedPath)
          .dir()
          .filePath(QStringLiteral(".default-fan-curve.yaml.backup"));
  if (QFileInfo::exists(installedPath) || QFileInfo::exists(installedBackup))
    return installedPath;

#ifdef LINUX_THERMAL_DEFAULT_CURVE_SOURCE_PATH
  return QStringLiteral(LINUX_THERMAL_DEFAULT_CURVE_SOURCE_PATH);
#else
  return installedPath;
#endif
}

QString FanCurveConfig::backupConfigPath() {
  return QFileInfo(editableConfigPath())
      .dir()
      .filePath(QStringLiteral(".default-fan-curve.yaml.backup"));
}

bool FanCurveConfig::ensureDefaultBackup(QString *error) {
  const auto backupPath = backupConfigPath();
  if (QFileInfo::exists(backupPath))
    return true;

  QFile source(editableConfigPath());
  if (!source.open(QIODevice::ReadOnly)) {
    if (error)
      *error = QStringLiteral("Unable to open editable fan curve YAML %1: %2")
                   .arg(source.fileName(), source.errorString());
    return false;
  }
  const auto contents = source.readAll();
  const auto directory = QFileInfo(backupPath).dir().absolutePath();
  if (!QDir().mkpath(directory)) {
    if (error)
      *error = QStringLiteral("Unable to create fan curve config directory %1.")
                   .arg(directory);
    return false;
  }

  QSaveFile backup(backupPath);
  if (!backup.open(QIODevice::WriteOnly) || backup.write(contents) != contents.size() ||
      !backup.commit()) {
    if (error)
      *error = QStringLiteral("Unable to create default fan curve backup %1: %2")
                   .arg(backupPath, backup.errorString());
    return false;
  }
  return true;
}

QVariantList FanCurveConfig::loadFile(const QString &path, QString *error) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (error)
      *error = QStringLiteral("Unable to open default fan curve YAML %1: %2")
                   .arg(path, file.errorString());
    return {};
  }

  QVariantList points;
  QVariantMap point;
  bool inPoints = false;
  int lineNumber = 0;
  while (!file.atEnd()) {
    ++lineNumber;
    auto line = QString::fromUtf8(file.readLine());
    const auto comment = line.indexOf(u'#');
    if (comment >= 0)
      line.truncate(comment);
    line = line.trimmed();
    if (line.isEmpty())
      continue;
    if (line == QStringLiteral("points:")) {
      inPoints = true;
      continue;
    }
    if (!inPoints)
      continue;
    if (!line.startsWith(u'-') && !line.startsWith(QStringLiteral("temperature:")) &&
        !line.startsWith(QStringLiteral("duty:")))
      break;

    if (line.startsWith(u'-')) {
      if (!appendPoint(&points, &point, error, lineNumber))
        return {};
      line = line.sliced(1).trimmed();
      if (line.isEmpty())
        continue;
    }

    int value = 0;
    if (parseField(line, QStringLiteral("temperature"), &value)) {
      point.insert(QStringLiteral("temperature"), value);
    } else if (parseField(line, QStringLiteral("duty"), &value)) {
      point.insert(QStringLiteral("duty"), value);
    } else {
      if (error)
        *error = QStringLiteral("Invalid curve field at YAML line %1.")
                     .arg(lineNumber);
      return {};
    }
  }

  if (!appendPoint(&points, &point, error, lineNumber + 1))
    return {};
  if (points.isEmpty() && error)
    *error = QStringLiteral("The default fan curve YAML contains no points.");
  return points;
}

QVariantList FanCurveConfig::loadDefault(QString *error) {
  if (!ensureDefaultBackup(error))
    return {};
  return loadFile(backupConfigPath(), error);
}
