// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2 or Later
// Refer to the license.txt file included.

#pragma once

#include "VideoCommon/TextureCacheBase.h"

namespace CrossPhire
{
class TextureCache : public ::TextureCache
{

private:
	struct TCacheEntry : TCacheEntryBase
	{
		TCacheEntry(const TCacheEntryConfig& config) : TCacheEntryBase(config) {}

		void Load(unsigned int width, unsigned int height,
				  unsigned int expanded_width, unsigned int levels) override {}

		void FromRenderTarget(u32 dstAddr, unsigned int dstFormat,
							  PEControl::PixelFormat srcFormat, const EFBRectangle& srcRect,
							  bool isIntensity, bool scaleByHalf, unsigned int cbufid,
							  const float *colmat) override {}

		void Bind(unsigned int stage) override {}
		bool Save(const std::string& filename, unsigned int level) override { return false; }
	};

	TCacheEntryBase* CreateTexture(const TCacheEntryConfig& config) override { return new TCacheEntry(config); }

	u64 EncodeToRamFromTexture(u32 address, void* source_texture, u32 SourceW, u32 SourceH, bool bFromZBuffer, bool bIsIntensityFmt, u32 copyfmt, int bScaleByHalf, const EFBRectangle& source) { return 0; };

	void ConvertTexture(TCacheEntryBase* entry, TCacheEntryBase* unconverted, void* palette, TlutFormat format) override {}

	void CompileShaders() override { }
	void DeleteShaders() override { }
};


}