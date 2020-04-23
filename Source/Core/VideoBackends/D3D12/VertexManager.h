// Copyright 2019 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "VideoBackends/D3D12/DescriptorHeapManager.h"
#include "VideoBackends/D3D12/StreamBuffer.h"
#include "VideoCommon/VertexManagerBase.h"

namespace DX12
{
class VertexManager final : public VertexManagerBase
{
public:
  VertexManager();
  ~VertexManager();

  bool Initialize() override;

  void UploadUtilityUniforms(const void* uniforms, u32 uniforms_size) override;
  bool UploadTexelBuffer(const void* data, u32 data_size, TexelBufferFormat format,
                         u32* out_offset) override;
  bool UploadTexelBuffer(const void* data, u32 data_size, TexelBufferFormat format, u32* out_offset,
                         const void* palette_data, u32 palette_size,
                         TexelBufferFormat palette_format, u32* out_palette_offset) override;

protected:
  void ResetBuffer(u32 vertex_stride) override;
  void CommitBuffer(u32 num_vertices, u32 vertex_stride, u32 num_indices, u32* out_base_vertex,
                    u32* out_base_index) override;
  void UploadUniforms(VertexShaderActiveUniforms) override;

  void UpdateVertexShaderConstants(VertexShaderActiveUniforms);
  void UpdateGeometryShaderConstants();
  void UpdatePixelShaderConstants();

  // Allocates storage in the uniform buffer of the specified size. If this storage cannot be
  // allocated immediately, the current command buffer will be submitted and all stage's
  // constants will be re-uploaded. false will be returned in this case, otherwise true.
  bool ReserveConstantStorage();
  void UploadAllConstants();

  StreamBuffer m_vertex_stream_buffer;
  StreamBuffer m_index_stream_buffer;
  StreamBuffer m_uniform_stream_buffer;
  StreamBuffer m_texel_stream_buffer;
  std::array<DescriptorHandle, NUM_TEXEL_BUFFER_FORMATS> m_texel_buffer_views = {};
};

}  // namespace DX12
