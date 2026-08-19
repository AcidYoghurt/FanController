#pragma once

#include <QDBusArgument>
#include <QDBusVariant>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace LinuxThermal::Dbus {
inline QVariant unwrap(const QVariant &value) {
  if (value.metaType() == QMetaType::fromType<QDBusVariant>())
    return unwrap(value.value<QDBusVariant>().variant());

  if (value.metaType() == QMetaType::fromType<QDBusArgument>()) {
    const auto argument = value.value<QDBusArgument>();
    switch (argument.currentType()) {
    case QDBusArgument::ArrayType: {
      QVariantList list;
      argument.beginArray();
      while (!argument.atEnd()) {
        QDBusVariant item;
        argument >> item;
        list.push_back(unwrap(item.variant()));
      }
      argument.endArray();
      return list;
    }
    case QDBusArgument::MapType: {
      QVariantMap map;
      argument.beginMap();
      while (!argument.atEnd()) {
        argument.beginMapEntry();
        QString key;
        QDBusVariant item;
        argument >> key >> item;
        argument.endMapEntry();
        map.insert(key, unwrap(item.variant()));
      }
      argument.endMap();
      return map;
    }
    default:
      return value;
    }
  }

  if (value.metaType().id() == QMetaType::QVariantList) {
    QVariantList result;
    const auto list = value.toList();
    result.reserve(list.size());
    for (const auto &item : list)
      result.push_back(unwrap(item));
    return result;
  }

  if (value.metaType().id() == QMetaType::QVariantMap) {
    QVariantMap result;
    const auto map = value.toMap();
    for (auto iterator = map.cbegin(); iterator != map.cend(); ++iterator)
      result.insert(iterator.key(), unwrap(iterator.value()));
    return result;
  }

  return value;
}

inline QVariantMap unwrapMap(const QVariantMap &map) {
  return unwrap(map).toMap();
}

inline QVariantList unwrapList(const QVariantList &list) {
  return unwrap(list).toList();
}
} // namespace LinuxThermal::Dbus
