// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include "VideoCommon/AbstractTexture.h"

class AbstractTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
  Q_OBJECT

public:
  ~AbstractTextureNode() override;

  // as we implement both QSGTextureProvider and QSGSimpleTextureNode, we need to point at the
  // correct underlying texture() method.
  QSGTexture* texture() const override { return QSGSimpleTextureNode::texture(); }

  void setBlankTexture();

  void setTexture(AbstractTexture* texture);

protected:
  AbstractTextureNode(QQuickItem* parent) : m_item(parent) {}

  virtual void setTextureImpl(AbstractTexture* texture) = 0;

  QQuickItem* m_item;  // Points back at DolphinItem
  QSize m_size;
};

class VkTextureNode : public AbstractTextureNode
{
  Q_OBJECT

public:
  VkTextureNode(QQuickItem* parent) : AbstractTextureNode(parent){};

  virtual void setTextureImpl(AbstractTexture* texture) override;
};

class GlTextureNode : public AbstractTextureNode
{
  Q_OBJECT

public:
  GlTextureNode(QQuickItem* parent) : AbstractTextureNode(parent){};
  static void Finish(AbstractTexture* texture);

  virtual void setTextureImpl(AbstractTexture* texture) override;
};