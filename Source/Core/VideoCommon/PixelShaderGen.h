// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/CommonTypes.h"
#include "VideoCommon/LightingShaderGen.h"
#include "VideoCommon/ShaderGenCommon.h"
#include "VideoCommon/PixelShaderUid.h"

enum class APIType;

ShaderCode GeneratePixelShaderCode(APIType ApiType, const ShaderHostConfig& host_config,
                                   const pixel_shader_uid_data* uid_data);
void WritePixelShaderCommonHeader(ShaderCode& out, APIType ApiType, u32 num_texgens,
                                  const ShaderHostConfig& host_config, bool bounding_box);
void ClearUnusedPixelShaderUidBits(APIType ApiType, const ShaderHostConfig& host_config,
                                   PixelShaderUid* uid);