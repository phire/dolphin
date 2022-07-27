// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQuick/AbstractTextureNode.h"

#include "VideoBackends/Vulkan/VKTexture.h"
#include "VideoBackends/Vulkan/VulkanContext.h"

#include "VideoBackends/OGL/OGLTexture.h"

#include <QtQuick/QQuickItem>
// #include <QOpenGLContext>
// #include <QOpenGLFunctions>
#include <QtQuick/qsgtexture_platform.h>

AbstractTextureNode::~AbstractTextureNode()
{
  // TODO: Free resources

  delete texture();
}

void AbstractTextureNode::setBlankTexture()
{
  auto image = QImage(1, 1, QImage::Format_ARGB32);
  m_size = QSize(1, 1);
  image.fill(Qt::black);
  auto blank_texture = m_item->window()->createTextureFromImage(image);
  QSGSimpleTextureNode::setTexture(blank_texture);
  return;
}

void AbstractTextureNode::setTexture(AbstractTexture* texture)
{
  if (texture)
  {
    m_size = QSize(texture->GetWidth(), texture->GetHeight());

    setTextureImpl(texture);
  }
  else
  {
    setBlankTexture();
  }
}

void VkTextureNode::setTextureImpl(AbstractTexture* texture)
{
  auto vk_texture = static_cast<Vulkan::VKTexture*>(texture);

  QSGTexture* wrapper = QNativeInterface::QSGVulkanTexture::fromNative(
      vk_texture->GetImage(), vk_texture->GetLayout(), m_item->window(), m_size);

  QSGSimpleTextureNode::setTexture(wrapper);
  Q_ASSERT(wrapper->nativeInterface<QNativeInterface::QSGVulkanTexture>()->nativeImage() == vk_texture->GetImage());
}

void GlTextureNode::Finish(AbstractTexture* texture)
{
  auto gl_texture = static_cast<OGL::OGLTexture*>(texture);

  // This will probably do nothing, we use GLsync when supported.
  // Needs to be called on dolphin's GLContent
  gl_texture->Finish();
}

void GlTextureNode::setTextureImpl(AbstractTexture* texture)
{
  auto gl_texture = static_cast<OGL::OGLTexture*>(texture);

  // We need to make sure Dolphin's GlContext is finished with the texture before qtquick's context
  // renders it
  gl_texture->Sync();

  m_size = QSize(gl_texture->GetWidth(), gl_texture->GetHeight());

  QSGTexture* wrapper = QNativeInterface::QSGOpenGLTexture::fromNative(
      gl_texture->GetGLTextureId(), m_item->window(), m_size, QQuickWindow::TextureHasAlphaChannel);

  QSGSimpleTextureNode::setTexture(wrapper);
  Q_ASSERT(wrapper->nativeInterface<QNativeInterface::QSGOpenGLTexture>()->nativeTexture() ==
           gl_texture->GetGLTextureId());
}