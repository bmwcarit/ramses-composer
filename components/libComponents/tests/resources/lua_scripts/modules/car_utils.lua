local car_utils = {}


car_utils.paintSettings =
{
    Name = Type:String(), -- for debugging
    BaseColor = Type:Vec4f(),
    MetallicRoughness = Type:Vec2f(),
    SheenRoughness = Type:Float(),
    SheenScale = Type:Float(),
    NormalScale = Type:Float(),
}

function car_utils.getPaintSettings()
    return car_utils.paintSettings
end


car_utils.perspectiveSettings =
{
    CraneGimbal = {
        Yaw = Type:Float(),
        Pitch = Type:Float(),
        Roll = Type:Float(),
        Distance = Type:Float(),
    },
    Origin = Type:Vec3f(),
}

function car_utils.getPerspectiveSettings()
    return car_utils.perspectiveSettings
end


-- -- Clone table and make it unique (allows modifying of tables from modules)
-- function car_utils.clone_table(t)
--     -- return { unpack(org) }
--     local nt = {}
--     for i = 1, rl_len(t) do
--         nt[i] = t[i]
--     end
--     return nt
-- end


-- Clamp f (number) between 'minimum' (number) and 'maximum' (number)
-- returns number
function car_utils.clamp(f, minimum, maximum)
    if minimum > maximum then minimum, maximum = maximum, minimum end -- swap arguments if boundaries are provided in the wrong order
    return math.max(minimum, math.min(maximum, f))
end


-- Return 1 if f (number) is greater than zero, -1 if f is less than zero, or 0 if f is zero
function car_utils.sign(f)
    if (f > 0) then return 1 elseif (f < 0) then return -1 else return 0 end
end


return car_utils
