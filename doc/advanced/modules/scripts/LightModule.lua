local light = {}

light.lightDirections = {
    {1, 0, -2},
    {-1, 0, -2},
    {0, -3, -1}
}

light.default_colors = {
    light = {1.0, 1.0, 1.0},
    diffuse = {0.28, 0.82, 0.6}
}

function light.getLightDirection(lightId)
    local lightDirection = light.lightDirections[lightId]
    if not lightDirection then
        error("No light direction with id '" .. lightId .. "' provided by the light module!")
    end
    return lightDirection
end

function light.resolveColor(colorName, userColor)
    -- If a user color value was given, forward it. Otherwise assign default color
    if userColor ~= nil and (userColor[1] ~= 0 or userColor[2] ~= 0 or userColor[3] ~= 0) then
        return userColor
    else
        local color = light.default_colors[colorName]
        if not color then
            error("No color with name '" .. colorName .. "' provided by the light module!")
        end

        return color
    end
end

return light
