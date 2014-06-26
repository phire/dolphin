// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include <map>

#include "VideoCommon/BPStructs.h"
#include "VideoCommon/TextureCacheBase.h"
#include "VideoCommon/VideoCommon.h"

namespace NullVideo
{

class TextureCache : public ::TextureCache
{
private:
	// Null Cache entry which gets returned to VideoCommon
	struct TCacheEntry : TCacheEntryBase
	{

		PC_TexFormat pcfmt;

		void Load(unsigned int width, unsigned int height,
			unsigned int expanded_width, unsigned int level) override {}

		void FromRenderTarget(u32 dstAddr, unsigned int dstFormat,
			PEControl::PixelFormat srcFormat, const EFBRectangle& srcRect,
			bool isIntensity, bool scaleByHalf, unsigned int cbufid,
			const float *colmat) override {}

		void Bind(unsigned int stage) override {}
		bool Save(const std::string& filename, unsigned int level) override {
			return false;
		}
	};

	TCacheEntryBase* CreateTexture(unsigned int width, unsigned int height,
		unsigned int expanded_width, unsigned int tex_levels, PC_TexFormat pcfmt) override {
		TCacheEntry &entry = *new TCacheEntry;
		return &entry;
	}

	TCacheEntryBase* CreateRenderTargetTexture(unsigned int scaled_tex_w,
											   unsigned int scaled_tex_h) override {
		TCacheEntry &entry = *new TCacheEntry;
		return &entry;
	}
};

}
