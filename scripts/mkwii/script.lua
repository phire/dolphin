----- GLOBAL VARIABLES -----
-- package.path = GetScriptsDir() .. "/MKW/MKW_core.lua"
local core = require("MKW_core")


-- package.path = GetScriptsDir() .. "/MKW/MKW_Pointers.lua"
local Pointers = require("MKW_Pointers")
--Add an underscore (_) to the beginning of the filename if you want the script to auto launch once you start a game!

local quaterframe = 0

function onScriptStart(ctx)
	millPrev = ctx:ReadFloat(0x800001b0)
    print(millPrev)
    print(ctx:ReadFloat(0x80000000))
end

local quaterframe = 0
local screentext = ""

function onScriptUpdate(ctx)
    quaterframe = quaterframe + 1

        local speed = core.getSpd(ctx)
        local pos = core.getPos(ctx)
		ctx:WriteFloat(0x808b1ccc, -1.2)
		local text = ""
			text = text .. string.format("Frame: %d ", quaterframe // 4)
			text = text .. "\n\n===== Speed ====="
			text = text .. string.format("\nX: %8.4f | Y: %8.4f | Z: %8.4f \nXZ: %12.8f | XYZ: %12.8f", speed.X, speed.Y, speed.Z, speed.XZ, speed.XYZ)
            text = text .. string.format("\n%f %f %f", pos.X, pos.Y, pos.Z)
			if core.isSinglePlayer(ctx) == false then
				text = text .. "\n\n===== Time Difference ====="
				text = text .. string.format("\nFacing: %11.6f (%s) \nMoving: %11.6f (%s) \nZ (Finish): %11.6f (%s) \nX (Perpend): %11.6f (%s)", core.getRotDifference(ctx).D, core.isAhead(ctx).RD, core.getDirectDifference(ctx).D, core.isAhead(ctx).DD, core.getFinishDifference(ctx).D, core.isAhead(ctx).FD, core.getOtherDifference(ctx).D, core.isAhead(ctx).OD)
			end
			text = text .. "\n\n===== Rotation ====="
			text = text .. string.format("\nFacing X (Pitch): %8.4f \nFacing Y (Yaw):   %8.4f \nMoving Y (Yaw):   %8.4f \nFacing Z (Roll):  %8.4f", core.calculateEuler(ctx).X, core.calculateEuler(ctx).Y, core.calculateDirection(ctx).Y, core.calculateEuler(ctx).Z)
        screentext = text

    -- Schedule next callback in 1/4 frame
    ctx:BreakOnCycle(728 * 1000000 // 180, onScriptUpdate)
end


Cpu.BreakOnRun(function(ctx)
	-- This function runs at the very first cycle of emulation (or when script is started)
    onScriptStart(ctx)
    ctx:BreakOnCycle(100, onScriptUpdate)
end)

BasicGui.RegisterDrawHook(function(ctx)
	-- This runs on the gpu thread
	-- And gets called once per frame just before Dolphin draws the ImGUI UI
	local offset = ctx:DrawText(screentext, 30, 30, 0xffffffff)
end)