local Pointers = {}

-- This is hardcoded (for now)
-- Need to adjust based on what version you are playing 
-- (E = USA, P = Europe, J = Japan, K = Korea)
function GetGameID() return "RMCE01" end

function GetPointerNormal(ctx, main_pointer, ...)
  -- This reimplements the pointer-chaseing behaviour of
  -- the old luacore branch
  local ptr = main_pointer
  for i,v in ipairs{...} do
    local val = ctx:ReadU32(ptr)
    --print(string.format("%i [%x]+%x = %x+%x", i, ptr, v, val, v))
    ptr = val + v
    if val == 0 then return 0 end
  end
  ptr = ctx:ReadU32(ptr)
  --print(string.format("Final: %x", ptr))
  return ptr
end
Pointers.GetPointerNormal = GetPointerNormal

local function getInputDataPointer(ctx)
  local inputData
  if GetGameID() == "RMCP01" then inputData = 0x809BD70C
  elseif GetGameID() == "RMCE01"then inputData = 0x809B8F4C
  elseif GetGameID() == "RMCJ01" then inputData = 0x809BC76C
  elseif GetGameID() == "RMCK01" then inputData = 0x809ABD4C
  end
  return inputData
end
Pointers.getInputDataPointer = getInputDataPointer

local function getRaceDataPointer(ctx)
  local raceData
  if GetGameID() == "RMCP01" then raceData = 0x809BD728
  elseif GetGameID() == "RMCE01"then raceData = 0x809B8F68
  elseif GetGameID() == "RMCJ01" then raceData = 0x809BC788
  elseif GetGameID() == "RMCK01" then raceData = 0x809ABD68
  end
  return raceData
end
Pointers.getRaceDataPointer = getRaceDataPointer

local function getRaceData2Pointer(ctx, Offset)
  local raceData2
  if GetGameID() == "RMCP01" then raceData2 = 0x809BD730
  elseif GetGameID() == "RMCE01"then raceData2 = 0x809B8F70
  elseif GetGameID() == "RMCJ01" then raceData2 = 0x809BC790
  elseif GetGameID() == "RMCK01" then raceData2 = 0x809ABD70
  end
  return GetPointerNormal(ctx, raceData2, 0xC, Offset)
end
Pointers.getRaceData2Pointer = getRaceData2Pointer

local function getInputPointer(ctx, Offset)
  return ctx:ReadU32(GetPointerNormal(ctx, getRaceData2Pointer(ctx, Offset), 0x48, 0x4))
end
Pointers.getInputPointer = getInputPointer

local function getRKSYSPointer(ctx)
  local saveData
  if GetGameID() == "RMCP01" then saveData = 0x809BD748
  elseif GetGameID() == "RMCE01"then saveData = 0x809B8F88
  elseif GetGameID() == "RMCJ01" then saveData = 0x809BC7A8
  elseif GetGameID() == "RMCK01" then saveData = 0x809ABD88
  end
  return GetPointerNormal(ctx, saveData, 0x14, 0x0)
end
Pointers.getRKSYSPointer = getRKSYSPointer

local function getPrevPositionPointer(ctx, Offset)
  local pointer
  if GetGameID() == "RMCP01" then pointer = 0x809C18F8
  elseif GetGameID() == "RMCE01"then pointer = 0x809BD110
  elseif GetGameID() == "RMCJ01" then pointer = 0x809C0958
  elseif GetGameID() == "RMCK01" then pointer = 0x809AFF38
  end
  return GetPointerNormal(ctx, pointer, 0xC, 0x10, Offset, 0x0, 0x8, 0x90)
end
Pointers.getPrevPositionPointer = getPrevPositionPointer

local function getPositionPointer(ctx, Offset)
  local pointer
  if GetGameID() == "RMCP01" then pointer = 0x809C18F8
  elseif GetGameID() == "RMCE01"then pointer = 0x809BD110
  elseif GetGameID() == "RMCJ01" then pointer = 0x809C0958
  elseif GetGameID() == "RMCK01" then pointer = 0x809AFF38
  end
  return GetPointerNormal(ctx, pointer, 0xC, 0x10, Offset, 0x0, 0x8, 0x90, 0x4)
end
Pointers.getPositionPointer = getPositionPointer

local function getPlayerBasePointer(ctx, Offset)
  local pointer
  if GetGameID() == "RMCP01" then pointer = 0x809C18F8
  elseif GetGameID() == "RMCE01"then pointer = 0x809BD110
  elseif GetGameID() == "RMCJ01" then pointer = 0x809C0958
  elseif GetGameID() == "RMCK01" then pointer = 0x809AFF38
  end
  return GetPointerNormal(ctx, pointer, 0xC, 0x10, Offset, 0x10, 0x10)
end
Pointers.getPlayerBasePointer = getPlayerBasePointer

local function getFrameOfInputAddress(ctx)
  local frameaddress
  if GetGameID() == "RMCP01" then frameaddress = 0x809C38C0
  elseif GetGameID() == "RMCE01" then frameaddress = 0x809BF0B8
  elseif GetGameID() == "RMCJ01" then frameaddress = 0x809C2920
  elseif GetGameID() == "RMCK01" then frameaddress = 0x809B1F00
  end
  return frameaddress
end
Pointers.getFrameOfInputAddress = getFrameOfInputAddress

return Pointers
