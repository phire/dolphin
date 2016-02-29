// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/VK/VkLoader.h"

namespace VK
{
	std::vector<unsigned int> CompileGlslToSpirv(std::string code, VkShaderStageFlagBits type);

	void InitilizeGlslang();
	void ShutdownGlslang();
}
