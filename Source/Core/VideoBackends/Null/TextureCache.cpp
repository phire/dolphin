// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cmath>
#include <fstream>
#include <vector>

#ifdef _WIN32
#define _interlockedbittestandset workaround_ms_header_bug_platform_sdk6_set
#define _interlockedbittestandreset workaround_ms_header_bug_platform_sdk6_reset
#define _interlockedbittestandset64 workaround_ms_header_bug_platform_sdk6_set64
#define _interlockedbittestandreset64 workaround_ms_header_bug_platform_sdk6_reset64
#include <intrin.h>
#undef _interlockedbittestandset
#undef _interlockedbittestandreset
#undef _interlockedbittestandset64
#undef _interlockedbittestandreset64
#endif

#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Common/Hash.h"
#include "Common/MemoryUtil.h"
#include "Common/StringUtil.h"

#include "Core/HW/Memmap.h"

#include "VideoBackends/Null/FramebufferManager.h"
#include "VideoBackends/Null/ProgramShaderCache.h"
#include "VideoBackends/Null/Render.h"
#include "VideoBackends/Null/TextureCache.h"

#include "VideoCommon/BPStructs.h"
#include "VideoCommon/HiresTextures.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/TextureDecoder.h"
#include "VideoCommon/VideoConfig.h"

namespace NullVideo
{

TextureCache::TCacheEntry::~TCacheEntry()
{
}

TextureCache::TCacheEntry::TCacheEntry()
{

}

void TextureCache::TCacheEntry::Bind(unsigned int stage)
{

}

bool TextureCache::TCacheEntry::Save(const std::string& filename, unsigned int level)
{
	return false;
}

TextureCache::TCacheEntryBase* TextureCache::CreateTexture(unsigned int width,
	unsigned int height, unsigned int expanded_width,
	unsigned int tex_levels, PC_TexFormat pcfmt)
{


	TCacheEntry &entry = *new TCacheEntry;

	return &entry;
}

void TextureCache::TCacheEntry::Load(unsigned int width, unsigned int height,
	unsigned int expanded_width, unsigned int level)
{

}

TextureCache::TCacheEntryBase* TextureCache::CreateRenderTargetTexture(
	unsigned int scaled_tex_w, unsigned int scaled_tex_h)
{
	// We need to return an entry to Videocommon.
	// TODO: We could advoid this code getting called if we disabled efb copies.
	TCacheEntry *const entry = new TCacheEntry;

	return entry;
}

void TextureCache::TCacheEntry::FromRenderTarget(u32 dstAddr, unsigned int dstFormat,
	PEControl::PixelFormat srcFormat, const EFBRectangle& srcRect,
	bool isIntensity, bool scaleByHalf, unsigned int cbufid,
	const float *colmat)
{

}

TextureCache::TextureCache()
{

}


TextureCache::~TextureCache()
{

}

}
