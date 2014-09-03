// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <fstream>

#include "Common/FileUtil.h"
#include "Common/StringUtil.h"
#include "Common/Timer.h"
#include "VideoCommon/FPSCounter.h"
#include "VideoCommon/VideoConfig.h"
#include "Common/Hash.h"

#include "stdlib.h"

#define FPS_REFRESH_INTERVAL 1000

FPSCounter::FPSCounter()
	: m_fps(0)
	, m_counter(0)
	, m_fps_last_counter(0)
{
	m_update_time.Update();
	m_render_time.Update();
}

void FPSCounter::LogRenderTimeToFile(u64 val)
{
	if (!m_bench_file.is_open())
		m_bench_file.open(File::GetUserPath(D_LOGS_IDX) + "render_time.txt");

	m_bench_file << val << std::endl;
}

int FPSCounter::Update()
{
	if (m_update_time.GetTimeDifference() >= FPS_REFRESH_INTERVAL)
	{
		m_update_time.Update();
		m_fps = m_counter - m_fps_last_counter;
		m_fps_last_counter = m_counter;
		m_bench_file.flush();

		printf("Hashed %luMB/s\n", hashed / (1024*1024));
		hashed = 0;
	}

	if (g_ActiveConfig.bLogRenderTimeToFile)
	{
		LogRenderTimeToFile(m_render_time.GetTimeDifference());
		m_render_time.Update();
	}

	m_counter++;
	return m_fps;
}
