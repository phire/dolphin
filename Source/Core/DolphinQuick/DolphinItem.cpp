// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQuick/DolphinItem.h"
#include "DolphinQuick/AbstractTextureNode.h"

#include "VideoCommon/VideoBackendBase.h"
#include "VideoCommon/RenderBase.h"

#include <QtQuick/QQuickWindow>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include "DolphinQuick/Emulator.h"

DolphinItem::DolphinItem() {
  // Lets scene graph know that it should call updatePaintNode
  setFlag(ItemHasContents, true);

  qWarning("Dolphin Item created");

  if (g_video_backend->GetName() == "Vulkan") {
    m_backend = Backend::Vulkan;
  } else if (g_video_backend->GetName() == "OGL") {
    m_backend = Backend::OpenGL;
  } else {
    qFatal("Unknown video backend");
  }

 // auto engine = QQmlEngine::contextForObject(this)->engine();
  auto emulator = Emulator::get();

  connect(emulator, &Emulator::dolphinBooted, this, &DolphinItem::dolphinBooted);

  if (g_renderer) {
    dolphinBooted();
  }
}

void DolphinItem::dolphinBooted() {
  qWarning("Dolphin booted");

  g_renderer->m_present_callback = [this](RcTcacheEntry entry) {
    // This will be called on dolphin's GPU thread.
    if (m_tcache_entry != entry) {
      if (m_tcache_entry)
        m_tcache_entry->ReleaseContentLock();

      m_tcache_entry = entry;

      // Acquiring the content lock ensures that texture cache won't reuse this texture entry
      // for something else.
      m_tcache_entry->AcquireContentLock();
      m_dirty = true;
      if (m_backend == Backend::OpenGL)
        GlTextureNode::Finish(entry->texture.get());
      QMetaObject::invokeMethod(this, &DolphinItem::update, Qt::QueuedConnection);
    }
  };
}

AbstractTextureNode* DolphinItem::makeTextureNode() {
  switch (m_backend) {
  case Backend::Vulkan:
    m_node = new VkTextureNode(this);
    break;
  case Backend::OpenGL:
    m_node = new GlTextureNode(this);
    break;
  }
  m_dirty = true;
  return m_node;
}

void DolphinItem::invalidateSceneGraph() {
  // Called on render thread when the scenegraph is invalidated
  m_node = nullptr;
}

void DolphinItem::releaseResources() {
  // called on the gui thread if the item is removed from scene
  m_node = nullptr;
}

QSGNode *DolphinItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
  // Called on the render thread when it is time to sync the state of the item with the scene graph.
  AbstractTextureNode *n = static_cast<AbstractTextureNode *>(node);

  if (!n) {
    n = makeTextureNode();
    n->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    n->setFiltering(QSGTexture::Linear);
  }

  if (m_dirty) {
    if (m_tcache_entry)
      n->setTexture(m_tcache_entry->texture.get());
    else
      n->setBlankTexture();
    m_dirty = false;
    n->markDirty(QSGNode::DirtyMaterial);
  }

  n->setRect(0, 0, width(), height());
  window()->update();

  return n;
}

void DolphinItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
        update();
}

