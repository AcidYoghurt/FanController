#include "InventoryModels.h"

MapListModel::MapListModel(QList<QByteArray> roles, QByteArray identityRole,
                               QObject *parent)
    : QAbstractListModel(parent), m_identityRole(std::move(identityRole)) {
  int role = Qt::UserRole + 1;
  for (const auto &name : roles)
    m_roles.insert(role++, name);
}

int MapListModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}
QVariant MapListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
    return {};
  return m_items.at(index.row()).toMap().value(QString::fromUtf8(m_roles.value(role)));
}
QHash<int, QByteArray> MapListModel::roleNames() const { return m_roles; }
void MapListModel::replace(const QVariantList &items) {
  if (m_items == items)
    return;
  const auto identityKey = QString::fromUtf8(m_identityRole);
  bool sameStructure = m_items.size() == items.size();
  if (sameStructure) {
    for (qsizetype row = 0; row < items.size(); ++row) {
      if (m_items.at(row).toMap().value(identityKey) !=
          items.at(row).toMap().value(identityKey)) {
        sameStructure = false;
        break;
      }
    }
  }
  if (!sameStructure) {
    const bool countChangedValue = m_items.size() != items.size();
    beginResetModel();
    m_items = items;
    endResetModel();
    if (countChangedValue)
      emit countChanged();
    return;
  }

  for (qsizetype row = 0; row < items.size(); ++row) {
    const auto oldItem = m_items.at(row).toMap();
    const auto newItem = items.at(row).toMap();
    QList<int> changedRoles;
    for (auto iterator = m_roles.cbegin(); iterator != m_roles.cend(); ++iterator) {
      const auto key = QString::fromUtf8(iterator.value());
      if (oldItem.value(key) != newItem.value(key))
        changedRoles.push_back(iterator.key());
    }
    m_items[row] = items.at(row);
    if (!changedRoles.isEmpty())
      emit dataChanged(index(row), index(row), changedRoles);
  }
}
QVariantMap MapListModel::itemAt(int index) const {
  return index >= 0 && index < m_items.size() ? m_items.at(index).toMap()
                                              : QVariantMap{};
}

SensorListModel::SensorListModel(QObject *parent)
    : MapListModel({"sensorId", "backendId", "deviceId", "displayName",
                    "deviceKind", "temperature", "available"},
                   "sensorId", parent) {}
FanListModel::FanListModel(QObject *parent)
    : MapListModel({"fanId", "backendId", "controllerId", "deviceId",
                    "displayName", "deviceKind", "rpm", "duty",
                    "rpmAvailable", "controllable", "automaticSupported",
                    "fullSpeedSupported", "curveSupported", "zeroRpmActive",
                    "available", "status"},
                   "fanId", parent) {}
ZoneListModel::ZoneListModel(QObject *parent)
    : MapListModel({"zoneId", "backendId", "controllerId", "displayName",
                    "sensorId", "targetFanIds", "deviceKind",
                    "automaticSupported", "fullSpeedSupported",
                    "curveSupported", "minimumDuty", "maximumDuty",
                    "minimumTemperature", "maximumTemperature",
                    "safetyTemperature", "zeroRpmAllowed", "currentDuty",
                    "controlTemperature"},
                   "zoneId", parent) {}
BackendListModel::BackendListModel(QObject *parent)
    : MapListModel({"backendId", "displayName", "driver", "status", "detail",
                    "capabilities"},
                   "backendId", parent) {}

CurvePointModel::CurvePointModel(QObject *parent) : QAbstractListModel(parent) {}
int CurvePointModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : static_cast<int>(m_points.size());
}
QVariant CurvePointModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_points.size())
    return {};
  const auto point = m_points.at(index.row()).toMap();
  if (role == TemperatureRole)
    return point.value(QStringLiteral("temperature"));
  if (role == DutyRole)
    return point.value(QStringLiteral("duty"));
  if (role == FixedRole)
    return index.row() == m_points.size() - 1;
  return {};
}
QHash<int, QByteArray> CurvePointModel::roleNames() const {
  return {{TemperatureRole, "temperature"}, {DutyRole, "duty"},
          {FixedRole, "fixed"}};
}
QVariantList CurvePointModel::toVariantList() const { return m_points; }
void CurvePointModel::load(const QVariantList &points, int revision) {
  if (m_points == points && m_revision == revision) {
    if (m_dirty) {
      m_dirty = false;
      emit dirtyChanged();
    }
    return;
  }
  const bool countChangedValue = m_points.size() != points.size();
  beginResetModel();
  m_points = points;
  endResetModel();
  if (countChangedValue)
    emit countChanged();
  emit pointsChanged();
  if (m_dirty) {
    m_dirty = false;
    emit dirtyChanged();
  }
  if (m_revision != revision) {
    m_revision = revision;
    emit revisionChanged();
  }
}
void CurvePointModel::setPoint(int row, int temperature, int duty) {
  if (row < 0 || row >= m_points.size() || row == m_points.size() - 1)
    return;
  auto point = m_points.at(row).toMap();
  if (point.value(QStringLiteral("temperature")).toInt() == temperature &&
      point.value(QStringLiteral("duty")).toInt() == duty)
    return;
  point.insert(QStringLiteral("temperature"), temperature);
  point.insert(QStringLiteral("duty"), duty);
  m_points[row] = point;
  emit dataChanged(index(row), index(row), {TemperatureRole, DutyRole});
  emit pointsChanged();
  if (!m_dirty) {
    m_dirty = true;
    emit dirtyChanged();
  }
}
QVariantMap CurvePointModel::pointAt(int row) const {
  return row >= 0 && row < m_points.size() ? m_points.at(row).toMap()
                                           : QVariantMap{};
}
void CurvePointModel::markClean() {
  if (!m_dirty)
    return;
  m_dirty = false;
  emit dirtyChanged();
}
