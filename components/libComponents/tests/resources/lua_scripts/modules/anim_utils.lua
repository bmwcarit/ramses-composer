local anim_utils = {}


anim_utils.animationSettings =
{
    SkipAnimations = Type:Bool(),
    SpeedModifier = Type:Float(),
}

function anim_utils.getAnimationSettings()
    return anim_utils.animationSettings
end


anim_utils.turntableSettings =
{
    TurntableMode = Type:Bool(),
    TurntableDuration_s = Type:Float(), -- duration of one full rotation [seconds]
}

function anim_utils.getTurntableSettings()
    return anim_utils.turntableSettings
end


-- Linear interpolation between x (number) and y (number) using an interpolation value t (number, 0.0 to 1.0)
-- output value is not clamped; returns number
function anim_utils.lerp(x, y, t)
    return x * (1.0 - t) + y * t
end


function anim_utils.animateFromTo(from, to, progress, interpolation)
    interpolation = interpolation or "linear"   -- if no interpolation arg is specified, assume linear

    --if type(to) == "string" then
    --    return progress < 1.0 and from or to -- interpolation for strings is only sensible as 'constant'
    --end

    if (interpolation == "constant") then

        if type(to) == "number" then
            return progress < 1.0 and from or to

        elseif type(to) == "table" then
            local t = {}
            for i = 1, #to do
                t[i] = progress < 1.0 and from[i] or to[i]
            end
            return t
        end

    elseif (interpolation == "linear") then

        if type(to) == "number" then
            return anim_utils.lerp(from, to, progress)

        elseif type(to) == "table" then
            local t = {}
            for i = 1, #to do
                t[i] = anim_utils.lerp(from[i], to[i], progress)
            end
            return t
        end

    elseif (interpolation == "smooth") then
        local smoothstep = progress * progress * (3.0 - 2.0 * progress)

        if type(to) == "number" then
            return smoothstep * to + (1.0 - smoothstep) * from

        elseif type(to) == "table" then
            local t = {}
            for i = 1, #to do
                t[i] = smoothstep * to[i] + (1.0 - smoothstep) * from[i]
            end
            return t
        end

    elseif (interpolation == "quadratic") then
        local quadratic = math.pow(progress, 2.0)

        if type(to) == "number" then
            return quadratic * to + (1.0 - quadratic) * from

        elseif type(to) == "table" then
            local t = {}
            for i = 1, #to do
                t[i] = quadratic * to[i] + (1.0 - quadratic) * from[i]
            end
            return t
        end
    end
end


return anim_utils
