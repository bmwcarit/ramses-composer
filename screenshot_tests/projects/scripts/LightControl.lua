
-- This script controls the lighting of a Phong-like shading model
-- The light position is static, but can be switched from three
-- different positions: left, right, and top (use numbers 0, 1, 2 on light_id to toggle)
-- Optionally specify a diffuse_color to override the default (green-ish) color

function interface(IN,OUT)
    -- Input: index into an array static light positions
    IN.light_id = Type:Int32()
    -- Input: diffuse color (setting to zero causes the script to use its default value, see init())
    IN.diffuse_color = Type:Vec3f()
    -- Input: light color (setting to zero causes the script to use its default value, see init())
    IN.light_color = Type:Vec3f()

    -- Output: direction of light in that static position
    OUT.light_direction = Type:Vec3f()
    -- Output: light color
    OUT.light_color = Type:Vec3f()
    -- Output: diffuse color for material(s)
    OUT.diffuse_color = Type:Vec3f()
end

function init()
    -- Declares default colors which are used if no other value was provided in the INputs
    GLOBAL.default_color = {0.28, 0.82, 0.6}
    GLOBAL.default_light_color = {1.0, 1.0, 1.0}
end

function run(IN,OUT)
    local lightId = IN.light_id
    if lightId < 0 or lightId > 2 then
        lightId = 0
    end

    local lightDirections = {
        [0] = {1, 0, -2},
        [1] = {-1, 0, -2},
        [2] = {0, -3, -1}
    }

    OUT.light_direction = lightDirections[lightId]

    -- If a light color value was given (IN.light_color), forward it. Otherwise assign default color
    if IN.light_color[1] ~= 0 or IN.light_color[2] ~= 0 or IN.light_color[3] ~= 0 then
        OUT.light_color = IN.light_color
    else
        OUT.light_color = GLOBAL.default_light_color
    end

    -- If a diffuse color value was given (IN.diffuse_color), forward it. Otherwise assign default color
    if IN.diffuse_color[1] ~= 0 or IN.diffuse_color[2] ~= 0 or IN.diffuse_color[3] ~= 0 then
        OUT.diffuse_color = IN.diffuse_color
    else
        OUT.diffuse_color = GLOBAL.default_color
    end
end
