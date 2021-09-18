// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later


// Simple module that just dumps a message into dolphin's log

#include <APIDiscovery.h>

#include "Common/Logging/Log.h"

#include "Plugins/BasicTypes.h"
#include "Plugins/ModuleManager.h"

#include <map>

// TODO: In the future, hope to generate these structures automatically though reflection.
//       but until we get a good idea why they should look like, they are hand-written

struct LogStreamInfo {
    mod_handle_t mod_id;
    std::string streamName;
};

static std::map<uint64_t, LogStreamInfo> LogStreams;


extern "C" {

// We don't want scripts/mods just randomly shouting into the log stream
// We want log linked with the script/mod that created them.
// So before they log, scripts/mods are expected to use their mod_id to create
// one or more log streams that they can log into later.
EXPORTED uint64_t CreateLogStream(mod_handle_t mod_id, String *StreamName) {
    // Find a unique stream id
    uint64_t stream_id = mod_id ^ 0xaa55;
    while (LogStreams.find(stream_id) != LogStreams.end())
    {
        stream_id = (stream_id + 555555) ^ 0x5a5a5a5a5a5a5a;
    }
    LogStreams[stream_id] = { mod_id, StreamName->to_string() };
    return stream_id;
}

// However, we don't currently have anywhere to send the various log streams.
// They just get merged together with a different function name
EXPORTED void LogMsg(uint64_t StreamHandle, Common::Log::LogLevel level, String* msg) {
    auto it = LogStreams.find(StreamHandle);
    if (it == LogStreams.end()) {
        ERROR_LOG_FMT(SCRIPT_HOST, "LogMsg to invalid stream ({:x}) - \"{}\"", StreamHandle, msg->to_string());
        return;
    }
    // This is a bit of a hack. We probably want a dedicated script log window.
    std::string padded_file = "                                        " + it->second.streamName;
    Common::Log::GenericLogFmt<1>(level, Common::Log::LogType::SCRIPT, padded_file.c_str(), 0, FMT_STRING("{}"), msg->to_string());
}
}

static Eumerator LogLevelsEumerators[] {
    {
        .Name = "Notice",
        .Value = static_cast<u_int64_t>(Common::Log::LogLevel::LNOTICE)
    },
    {
        .Name = "Error",
        .Value = static_cast<u_int64_t>(Common::Log::LogLevel::LERROR)
    },
    {
        .Name = "Warning",
        .Value = static_cast<u_int64_t>(Common::Log::LogLevel::LWARNING)
    },
    {
        .Name = "Info",
        .Value = static_cast<u_int64_t>(Common::Log::LogLevel::LINFO)
    },
    {
        .Name = "Debug",
        .Value = static_cast<u_int64_t>(Common::Log::LogLevel::LDEBUG)
    },
};

static Enum LogLevel {
    .EnumName = "LogLevel",
    .UnderlyingType = "uint32_t",
    .NumEumerators = 5,
    .Eumerators = LogLevelsEumerators,
};

static Argument CreateLogStreamArgs[] = {
    {
        .Name = "ModID",
        .Type = "uint64_t",
    },
    {
        .Name = "StreamName",
        .Type = "String",
    }
};

static Argument LogMsgArgs[] = {
    {
        .Name = "StreamHandle",
        .Type = "uint64_t",
    },
    {
        .Name = "Level",
        .Type = "LogLevel"
    },
    {
        .Name = "Msg",
        .Type = "String",
    }
};

static Function functions[] = {
    {
        .FunctionName = "CreateLogStream",
        .ReturnType = "uint64_t",
        .ReturnOwnership = false,
        .ArgumentCount = 2,
        .Arguments = CreateLogStreamArgs,
        .Symbol = SYMBOL(CreateLogStream),
        .FnPtr = reinterpret_cast<void* (*)(void*)>(&CreateLogStream)
    },
    {
        .FunctionName = "LogMsg",
        .ReturnType = "void",
        .ReturnOwnership = false,
        .ArgumentCount = 3,
        .Arguments = LogMsgArgs,
        .Symbol = SYMBOL(LogMsg),
        .FnPtr = reinterpret_cast<void* (*)(void*)>(&LogMsg)
    }
};

static Module Logging = {
    .Version = 1,
    .GlobalFunctionsCount = 2,
    .CallbackCount = 0,
    .ClassCount = 0,
    .EnumCount = 1,
    .GlobalFunctions = functions,
    .Callbacks = {},
    .Classes = {},
    .Enums = { &LogLevel }
};

void InitLoggingModule() {
    RegisterModuleDefintion(&Logging, ModuleInfo {
        String("Logging"),
        String("Provides a global way to log messages"),
        VersionInfo{1, 1}
    });
}