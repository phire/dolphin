local core = {}

-- package.path = GetScriptsDir() .. "/MKW/MKW_Pointers.lua"
local Pointers = require("MKW_Pointers")

local function getPos(ctx)
  local address = Pointers.getPositionPointer(ctx, 0x0) -- 0x0 first player in the array, to get the most accurate, read playerindex first
  if address == 0 then
    return {X = 0, Y = 0, Z = 0}
  end
  return {X = ctx:ReadFloat(address + 0x68), Y = ctx:ReadFloat(address + 0x6c), Z = ctx:ReadFloat(address + 0x70)}
end
core.getPos = getPos

local function getPosGhost(ctx)
  local address = Pointers.getPositionPointer(ctx, 0x4)
  if address == 0 then
    return {X = 0, Y = 0, Z = 0}
  end
  return {X = ctx:ReadFloat(address + 0x68), Y = ctx:ReadFloat(address + 0x6C), Z = ctx:ReadFloat(address + 0x70)}
end
core.getPosGhost = getPosGhost

local function getPrevPos(ctx)
  local address = Pointers.getPrevPositionPointer(ctx, 0x0)
  if address == 0 then
    return {X = 0, Y = 0, Z = 0}
  end
  return {X = ctx:ReadFloat(address + 0x18), Y = ctx:ReadFloat(address + 0x1C), Z = ctx:ReadFloat(address + 0x20)}
end
core.getPrevPos = getPrevPos

local function getSpd(ctx)
  local PrevXpos = getPrevPos(ctx).X
  local PrevYpos = getPrevPos(ctx).Y
  local PrevZpos = getPrevPos(ctx).Z
  local Xpos = getPos(ctx).X
  local Ypos = getPos(ctx).Y
  local Zpos = getPos(ctx).Z
  return {X = (Xpos - PrevXpos), Y = (Ypos - PrevYpos), Z = (Zpos - PrevZpos), XZ = math.sqrt(((Xpos - PrevXpos)^2) + (Zpos - PrevZpos)^2), XYZ = math.sqrt(((Xpos - PrevXpos)^2) + ((Ypos - PrevYpos)^2) + (Zpos - PrevZpos)^2)}
end
core.getSpd = getSpd

local function getInput(ctx)
  local address = Pointers.getInputPointer(ctx, 0x0) -- change this to 0x4 for ghost
  local offset = 0x8 -- too lazy to adjust the values beneath...
	local ABLR = 0
	local X = 0
	local Y = 0
	local DPAD = 0

  if address == 0 then
	ABLR = 0
	X = 0
	Y = 0
	DPAD = 0
  else
	ABLR = ctx:ReadU8(address + offset + 0x1)
	X = ctx:ReadU8(address + offset + 0xC)
	Y = ctx:ReadU8(address + offset + 0xD)
	DPAD = ctx:ReadU8(address + offset + 0xF)
  end


	local text = ""
	local text2 = ""
	local text3 = ""
	local text4 = ""

	if ABLR > 0 % 2 then textA = "A"
	elseif ABLR < 1 % 2 then textA = "."
	end

	if ABLR > 1 % 4 then textB = "B"
	elseif ABLR < 2 % 4 then textB = "."
	end

	if ABLR > 3 % 8 then textL = "L"
	elseif ABLR < 4 % 8 then textL = "."
	end

	if ABLR > 7 % 16 then textR = "R"
	elseif ABLR < 8 % 16 then textR = "."
	end

	if X == 0 then textX = "." -- these need to be fixed because they're nil atm
	else textX = "X"
	end

	if Y == 0 then textY = "." -- yes this too
	else textY = "Y"
	end

	if DPAD == 0 then textD = "." -- and this
	elseif DPAD == 1 then textD = "UP"
	elseif DPAD == 2 then textD = "DOWN"
	elseif DPAD == 3 then textD = "LEFT"
	elseif DPAD == 4 then textD = "RIGHT"
	end

	H = ctx:ReadU8(Pointers.GetPointerNormal(ctx, 0x809B8F70, 0xC, 0x0, 0x48, 0x38))
	V = ctx:ReadU8(Pointers.GetPointerNormal(ctx, 0x909B8F70, 0xC, 0x0, 0x48, 0x39))

	return {A = textA, B = textB, L = textL, R = textR, X = textX, Y = textY, DPAD = textD, H = H, V = V}
end
core.getInput = getInput

local function PosToAngle(ctx)
  local Xspd = getSpd(ctx).X
  if getSpd(ctx).X == 0 then Xspd = math.pi / 2
  end
  local angle = math.atan(getSpd(ctx).Z * Xspd)
  local finalAngle = angle * 65536 % 360
  local sine = math.sin(finalAngle)
  return {A = finalAngle, S = sine}
end
core.PosToAngle = PosToAngle

local function calculateDirection(ctx)
	local pz = core.getPos(ctx).Z
	local py = core.getPos(ctx).Y
	local px = core.getPos(ctx).X
	local prevz = core.getPrevPos(ctx).Z
	local prevy = core.getPrevPos(ctx).Y
	local prevx = core.getPrevPos(ctx).X
	local zdiff = 0
	local ydiff = 0
	local xdiff = 0
	local x_angle = 0
	local y_angle = 0
	local z_angle = 0
	local x = 0
	local y = 0
	local z = 0

	xdiff = px - prevx
	ydiff = py - prevy
	zdiff = pz - prevz

	x_angle = math.atan(math.abs(zdiff)/math.abs(ydiff)) * 180 / math.pi
	y_angle = math.atan(math.abs(zdiff)/math.abs(xdiff)) * 180 / math.pi
	z_angle = math.atan(math.abs(xdiff)/math.abs(ydiff)) * 180 / math.pi

	if ydiff > 0 and zdiff > 0 then x = (270 + x_angle) -- these 4 rows need to be fixed
	elseif ydiff > 0 and zdiff < 0 then x = (90 - x_angle)
	elseif ydiff < 0 and zdiff > 0 then x = (270 - x_angle)
	elseif ydiff < 0 and zdiff < 0 then x = (90 + x_angle)
	end

	if zdiff > 0 and xdiff > 0 then y = (270 + y_angle) -- these 4 rows are correct (finally)
	elseif zdiff > 0 and xdiff < 0 then y = (90 - y_angle)
	elseif zdiff < 0 and xdiff > 0 then y = (270 - y_angle)
	elseif zdiff < 0 and xdiff < 0 then y = (90 + y_angle)
	end

	if ydiff > 0 and xdiff > 0 then z = (270 + z_angle) -- these 4 rows need to be fixed
	elseif ydiff > 0 and xdiff < 0 then z = (90 - z_angle)
	elseif ydiff < 0 and xdiff > 0 then z = (270 - z_angle)
	elseif ydiff < 0 and xdiff < 0 then z = (90 + z_angle)
	end

	return {X = x, Y = y, Z = z}
end
core.calculateDirection = calculateDirection

local function calculateDirectionToGhost(ctx)
	local pz = core.getPos(ctx).Z
	local py = core.getPos(ctx).Y
	local px = core.getPos(ctx).X
	local gz = core.getPosGhost(ctx).Z
	local gy = core.getPosGhost(ctx).Y
	local gx = core.getPosGhost(ctx).X
	local zdiff = 0
	local ydiff = 0
	local xdiff = 0
	local x_angle = 0
	local y_angle = 0
	local z_angle = 0
	local x = 0
	local y = 0
	local z = 0

	xdiff = px - gx
	ydiff = py - gy
	zdiff = pz - gz

	x_angle = math.atan(math.abs(zdiff)/math.abs(ydiff)) * 180 / math.pi
	y_angle = math.atan(math.abs(zdiff)/math.abs(xdiff)) * 180 / math.pi
	z_angle = math.atan(math.abs(xdiff)/math.abs(ydiff)) * 180 / math.pi

	if ydiff > 0 and zdiff > 0 then x = (360 - x_angle)
	elseif ydiff > 0 and zdiff < 0 then x = (x_angle)
	elseif ydiff < 0 and zdiff > 0 then x = (180 - x_angle)
	elseif ydiff < 0 and zdiff < 0 then x = (180 + x_angle)
	end

	if zdiff > 0 and xdiff > 0 then y = (360 - y_angle)
	elseif zdiff > 0 and xdiff < 0 then y = (y_angle)
	elseif zdiff < 0 and xdiff > 0 then y = (180 - y_angle)
	elseif zdiff < 0 and xdiff < 0 then y = (180 + y_angle)
	end

	if ydiff > 0 and xdiff > 0 then z = (360 - z_angle)
	elseif ydiff > 0 and xdiff < 0 then z = (z_angle)
	elseif ydiff < 0 and xdiff > 0 then z = (180 - z_angle)
	elseif ydiff < 0 and xdiff < 0 then z = (180 + z_angle)
	end

	return {X = x, Y = y, Z = z}
end
core.calculateDirectionToGhost = calculateDirectionToGhost

--FrameCounter in Race
local function getFrameOfInput(ctx)
  return ctx:ReadU32(Pointers.getInputPointer(ctx, 0))
end
core.getFrameOfInput = getFrameOfInput

function math_atan2(x, y)
	local angle = math.asin( y / math.sqrt(x^2 + y^2 ) )

	if x < 0 then
		angle = math.pi - angle
	elseif y < 0 then
		angle = 2*math.pi + angle
	end

	return angle
end
core.math_atan2 = math_atan2

function getQuaternion(ctx)
  local offset2 = 0xF0
  local address2 = Pointers.getPositionPointer(ctx, 0x0)
  if(address2 == 0) then
    return {X = 0, Y = 0, Z = 0, W = 0}
  end
	return {X = ctx:ReadFloat(address2 + 0xF0), Y = ctx:ReadFloat(address2 + 0xF4),
			Z = ctx:ReadFloat(address2 + 0xF8), W = ctx:ReadFloat(address2 + 0xFC)}
end
core.getQuaternion = getQuaternion

local function calculateEuler(ctx)
  local qw = getQuaternion(ctx).W
  local qx = getQuaternion(ctx).X
  local qy = getQuaternion(ctx).Y
  local qz = getQuaternion(ctx).Z
  local qw2 = qw*qw
  local qx2 = qx*qx
  local qy2 = qy*qy
  local qz2 = qz*qz
  local test= qx*qy + qz*qw
  if (test > 0.499) then
    Yvalue = 360/math.pi*math_atan2(qx,qw)
    Zvalue = 90
    Xvalue = 0
    return {X = Xvalue, Y = Yvalue, Z = Zvalue}
  end
  if (test < -0.499) then
    Yvalue = -360/math.pi*math_atan2(qx,qw)
    Zvalue = -90
    Xvalue = 0
    return {X = Xvalue, Y = Yvalue, Z = Zvalue}
  end
  local h = math_atan2(2*qy*qw-2*qx*qz, 1-2*qy2-2*qz2)
  local a = math.asin(2*qx*qy + 2*qz*qw)
  local b = math_atan2(2*qx*qw-2*qy*qz, 1-2*qx2-2*qz2)
  Yvalue = (h*180/math.pi)
  Zvalue = (a*180/math.pi)
  Xvalue = (b*180/math.pi)
  return {X = (Xvalue - 90) % 360, Y = (Yvalue - 90) % 360, Z = Zvalue}
end
core.calculateEuler = calculateEuler

local function isSinglePlayer(ctx)
  local address = Pointers.getPositionPointer(ctx, 0x4)
  if address == 0 then return true
  else return false
  end
end
core.isSinglePlayer = isSinglePlayer

local function getDirectDifference(ctx)
	local xz = getSpd(ctx).XZ
	local px = getPos(ctx).X
	local pz = getPos(ctx).Z
	local gx = getPosGhost(ctx).X
	local gz = getPosGhost(ctx).Z
	local prevx = getPrevPos(ctx).X
	local prevz = getPrevPos(ctx).Z
	local direction = calculateDirection(ctx).Y
	local direction_id = 0
	local direction_cardinal = 0

	length = pz - gz
	width = px - gx
	angle = math.atan(math.abs(width)/math.abs(length))

	if length < 0 and direction > 270 and direction < 360 then direction_id = -1
	elseif length < 0 and direction >= 0 and direction < 90 then direction_id = -1
	elseif length > 0 and direction > 90 and direction < 270 then direction_id = -1
	else direction_id = 1
	end

	if length > 0 and width > 0 then direction_cardinal = 270
	elseif length > 0 and width < 0 then direction_cardinal = 0
	elseif length < 0 and width > 0 then direction_cardinal = 90
	elseif length < 0 and width < 0 then direction_cardinal = 180
	end

	angle_right = direction - (90*direction_cardinal)
	hypotenuse = math.sqrt(length^2 + width^2)
	angle_left = math.abs(angle - angle_right)
	distance = math.cos(angle_left)*hypotenuse

	if length == 0 then return {D = 0, I = 0}
	elseif xz == 0 then return {D = 0, I = 1}
	else return {D = (math.abs((distance/xz)/(60))*direction_id), I = 0}
	end
end
core.getDirectDifference = getDirectDifference

local function getRotDifference(ctx)
	local xz = getSpd(ctx).XZ
	local px = getPos(ctx).X
	local pz = getPos(ctx).Z
	local gx = getPosGhost(ctx).X
	local gz = getPosGhost(ctx).Z
	local prevx = getPrevPos(ctx).X
	local prevz = getPrevPos(ctx).Z
	local direction = calculateEuler(ctx).Y
	local direction_id = 0
	local direction_cardinal = 0

	length = pz - gz
	width = px - gx
	angle = math.atan(math.abs(width)/math.abs(length))

	if length < 0 and direction > 270 and direction < 360 then direction_id = -1
	elseif length < 0 and direction >= 0 and direction < 90 then direction_id = -1
	elseif length > 0 and direction > 90 and direction < 270 then direction_id = -1
	else direction_id = 1
	end

	if length > 0 and width > 0 then direction_cardinal = 270
	elseif length > 0 and width < 0 then direction_cardinal = 0
	elseif length < 0 and width > 0 then direction_cardinal = 90
	elseif length < 0 and width < 0 then direction_cardinal = 180
	end

	angle_right = direction - (90*direction_cardinal)
	hypotenuse = math.sqrt(length^2 + width^2)
	angle_left = math.abs(angle - angle_right)
	distance = math.cos(angle_left)*hypotenuse

	if length == 0 then return {D = 0, I = 0}
	elseif xz == 0 then return {D = 0, I = 1}
	else return {D = (math.abs((distance/xz)/(60))*direction_id), I = 0}
	end
end
core.getRotDifference = getRotDifference

local function getFinishDifference(ctx)
    local length = 0
	local zspd = getSpd(ctx).Z
	local zpos = getPos(ctx).Z
	local ghost_zpos = getPosGhost(ctx).Z

    length = zpos - ghost_zpos

	if length == 0 then return {D = 0, I = 0}
	elseif zspd == 0 then return {D = 0, I = 1}
	else return {D = ((length/zspd)/(60)), I = 0}
	end
end
core.getFinishDifference = getFinishDifference

local function getOtherDifference(ctx)
    local length = 0
	local xspd = getSpd(ctx).X
	local xpos = getPos(ctx).X
	local ghost_xpos = getPosGhost(ctx).X

    length = xpos - ghost_xpos

	if length == 0 then return {D = 0, I = 0}
	elseif xspd == 0 then return {D = 0, I = 1}
	else return {D = ((length/xspd)/(60)), I = 0}
	end
end
core.getOtherDifference = getOtherDifference

local function isAhead(ctx)
	local ddifftext = ""
	local rdifftext = ""
	local fdifftext = ""
	local odifftext = ""

	if core.getDirectDifference(ctx).I == 1 then ddifftext = "Not Moving"
	else
		if core.getDirectDifference(ctx).D > 0 then ddifftext = "Ahead"
		elseif core.getDirectDifference(ctx).D < 0 then ddifftext = "Behind"
		else ddifftext = "Tied"
		end
	end

	if core.getRotDifference(ctx).I == 1 then rdifftext = "Not Moving"
	else
		if core.getRotDifference(ctx).D > 0 then rdifftext = "Ahead"
		elseif core.getRotDifference(ctx).D < 0 then rdifftext = "Behind"
		else rdifftext = "Tied"
		end
	end

	if core.getFinishDifference(ctx).I == 1 then fdifftext = "Not Moving"
	else
		if core.getFinishDifference(ctx).D > 0 then fdifftext = "Ahead"
		elseif core.getFinishDifference(ctx).D < 0 then fdifftext = "Behind"
		else fdifftext = "Tied"
		end
	end

	if core.getOtherDifference(ctx).I == 1 then odifftext = "Not Moving"
	else
		if core.getOtherDifference(ctx).D > 0 then odifftext = "Ahead"
		elseif core.getOtherDifference(ctx).D < 0 then odifftext = "Behind"
		else odifftext = "Tied"
		end
	end

	return{DD = ddifftext, RD = rdifftext, FD = fdifftext, OD = odifftext}
end
core.isAhead = isAhead

local function translateKCL(ctx)

	local gameID = GetGameID()
		if gameID == "RMCP01" then
			collisionAddress1 = 0x809C38DC
			collisionAddress2 = 0x809C38E8
		elseif gameID == "RMCE01" then
			collisionAddress1 = 0x809BF0D4
			collisionAddress2 = 0x809BF0E0
		elseif gameID == "RMCJ01" then
			collisionAddress1 = 0x809C293C
			collisionAddress2 = 0x809C2948
		elseif gameID == "RMCK01" then
			collisionAddress1 = 0x809B1F1C
			collisionAddress2 = 0x809B1F28
	end

	local writeAddress1 = 0x1500000
	local writeAddress2 = 0x1500008
	local rawCollisionValue1 = ctx:ReadU16(collisionAddress1)
	local rawCollisionValue2 = ctx:ReadU16(collisionAddress2)
	local collisionValue1 = rawCollisionValue1 % 0x20
	local collisionValue2 = rawCollisionValue2 % 0x20
	local collisionType1 = ""
	local collisionType2 = ""

	if rawCollisionValue1/0x8000 >= 1 then collisionType1 = "SticWall"
	elseif rawCollisionValue1/0x4000 >= 1 then collisionType1 = "PushAway"
	elseif rawCollisionValue1/0x2000 >= 1 then collisionType1 = "TrickRod"
	else
		if collisionValue1 == 0x0 then collisionType1 = "Road"
		elseif collisionValue1 == 0x1 then collisionType1 = "SlipRoad"
		elseif collisionValue1 == 0x2 then collisionType1 = "WOffRoad"
		elseif collisionValue1 == 0x3 then collisionType1 = "Off-Road"
		elseif collisionValue1 == 0x4 then collisionType1 = "HOffRoad"
		elseif collisionValue1 == 0x5 then collisionType1 = "SlipRoad"
		elseif collisionValue1 == 0x6 then collisionType1 = "BoostPad"
		elseif collisionValue1 == 0x7 then collisionType1 = "B-Ramp"
		elseif collisionValue1 == 0x8 then collisionType1 = "JumpPad "
		elseif collisionValue1 == 0x9 then collisionType1 = "ItemRoad"
		elseif collisionValue1 == 0xA then collisionType1 = "SoldFall"
		elseif collisionValue1 == 0xB then collisionType1 = "MovWater" --KC River
		elseif collisionValue1 == 0xC then collisionType1 = "Wall"
		elseif collisionValue1 == 0xD then collisionType1 = "InvWall "
		elseif collisionValue1 == 0xE then collisionType1 = "ItemWall"
		elseif collisionValue1 == 0xF then collisionType1 = "Wall"
		elseif collisionValue1 == 0x10 then collisionType1 = "FallBoun"
		elseif collisionValue1 == 0x11 then collisionType1 = "CannonTr"
		elseif collisionValue1 == 0x12 then collisionType1 = "ReCalc" --enemy/item route based
		elseif collisionValue1 == 0x13 then collisionType1 = "HalfPipe"
		elseif collisionValue1 == 0x14 then collisionType1 = "Wall"
		elseif collisionValue1 == 0x15 then collisionType1 = "MoveRoad" --TF belt, CM escalator
		elseif collisionValue1 == 0x16 then collisionType1 = "SticRoad"
		elseif collisionValue1 == 0x17 then collisionType1 = "Road"
		elseif collisionValue1 == 0x18 then collisionType1 = "SoundTrg"
		elseif collisionValue1 == 0x19 then collisionType1 = "Unknown?"
		elseif collisionValue1 == 0x1A then collisionType1 = "EffectTr"
		elseif collisionValue1 == 0x1B then collisionType1 = "Unknown?"
		elseif collisionValue1 == 0x1C then collisionType1 = "Unknown?"
		elseif collisionValue1 == 0x1D then collisionType1 = "SlipRoad"
		elseif collisionValue1 == 0x1E then collisionType1 = "SpecWall"
		elseif collisionValue1 == 0x1F then collisionType1 = "Wall" --items can get through
		end
	end

	if rawCollisionValue2/0x8000 >= 1 then collisionType2 = "SticWall"
	elseif rawCollisionValue2/0x4000 >= 1 then collisionType2 = "PushAway"
	elseif rawCollisionValue2/0x2000 >= 1 then collisionType2 = "TrickRod"
	else
		if collisionValue2 == 0x0 then collisionType2 = "Road"
		elseif collisionValue2 == 0x1 then collisionType2 = "SlipRoad"
		elseif collisionValue2 == 0x2 then collisionType2 = "WOffRoad"
		elseif collisionValue2 == 0x3 then collisionType2 = "Off-Road"
		elseif collisionValue2 == 0x4 then collisionType2 = "HOffRoad"
		elseif collisionValue2 == 0x5 then collisionType2 = "SlipRoad"
		elseif collisionValue2 == 0x6 then collisionType2 = "BoostPad"
		elseif collisionValue2 == 0x7 then collisionType2 = "B-Ramp"
		elseif collisionValue2 == 0x8 then collisionType2 = "JumpPad "
		elseif collisionValue2 == 0x9 then collisionType2 = "ItemRoad"
		elseif collisionValue2 == 0xA then collisionType2 = "SoldFall"
		elseif collisionValue2 == 0xB then collisionType2 = "MovWater" --KC River
		elseif collisionValue2 == 0xC then collisionType2 = "Wall"
		elseif collisionValue2 == 0xD then collisionType2 = "InvWall "
		elseif collisionValue2 == 0xE then collisionType2 = "ItemWall"
		elseif collisionValue2 == 0xF then collisionType2 = "Wall"
		elseif collisionValue2 == 0x10 then collisionType2 = "FallBoun"
		elseif collisionValue2 == 0x11 then collisionType2 = "CannonTr"
		elseif collisionValue2 == 0x12 then collisionType2 = "ReCalc" --enemy/item route based
		elseif collisionValue2 == 0x13 then collisionType2 = "HalfPipe"
		elseif collisionValue2 == 0x14 then collisionType2 = "Wall"
		elseif collisionValue2 == 0x15 then collisionType2 = "MoveRoad" --TF belt, CM escalator
		elseif collisionValue2 == 0x16 then collisionType2 = "SticRoad"
		elseif collisionValue2 == 0x17 then collisionType2 = "Road"
		elseif collisionValue2 == 0x18 then collisionType2 = "SoundTrg"
		elseif collisionValue2 == 0x19 then collisionType2 = "Unknown?"
		elseif collisionValue2 == 0x1A then collisionType2 = "EffectTr"
		elseif collisionValue2 == 0x1B then collisionType2 = "Unknown?"
		elseif collisionValue2 == 0x1C then collisionType2 = "Unknown?"
		elseif collisionValue2 == 0x1D then collisionType2 = "SlipRoad"
		elseif collisionValue2 == 0x1E then collisionType2 = "SpecWall"
		elseif collisionValue2 == 0x1F then collisionType2 = "Wall" --items can get through
		end
	end

	return {A1 = writeAddress1, A2 = writeAddress2, T1 = collisionType1, T2 = collisionType2}
end
core.translateKCL = translateKCL

return core