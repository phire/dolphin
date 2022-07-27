// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtQuick/QQuickItem>
#include "VideoCommon/TextureCacheBase.h"

class AbstractTextureNode;
struct TCacheEntry;

class DolphinItem : public ::QQuickItem
{
  Q_OBJECT
  QML_ELEMENT

public:
  DolphinItem();

  enum class Backend {
    Vulkan,
    OpenGL,
  };
  Q_ENUM(Backend)

  Q_PROPERTY(Backend backend CONSTANT MEMBER m_backend)

protected:
  QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
  void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private slots:
  void invalidateSceneGraph();
  void dolphinBooted();

private:
  AbstractTextureNode* makeTextureNode();
  void releaseResources() override;

  // will be deleted by the scene graph
  AbstractTextureNode* m_node;
  Backend m_backend;
  Common::rc_ptr<TCacheEntry> m_tcache_entry;
  bool m_dirty = false;
};
