// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QQmlEngine>
class QQuickView;

// a Wrapper singleton to act as a bridge between Dolphin's emulator core and qml objects
class Emulator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  static Emulator* get() { return instance; }

signals:
  void dolphinBooted();

private:
  friend class QuickGui;
  static Emulator* instance;
  static int typeId;
};

