// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "VideoCommon/TextureCacheBase.h"

namespace VK
{

class TextureCache : public TextureCacheBase
{
	// TODO
	TCacheEntryBase* CreateTexture(const TCacheEntryConfig& config) override { return nullptr; };

	// TODO
	u64 EncodeToRamFromTexture(u32 address, void* source_texture, u32 source_width, u32 source_height, bool is_from_z_buffer, bool is_intensity_format, u32 copy_format, int scale_by_half, const EFBRectangle& source) { return 0; };

	// TODO
	void CopyEFB(u8* dst, u32 format, u32 native_width, u32 bytes_per_row, u32 num_blocks_y, u32 memory_stride,
				 PEControl::PixelFormat src_format, const EFBRectangle& src_rect,
				 bool is_intensity, bool scale_by_half) override { };

	// TODO
	virtual void ConvertTexture(TCacheEntryBase* entry, TCacheEntryBase* unconverted, void* palette, TlutFormat format) override { };
};

}