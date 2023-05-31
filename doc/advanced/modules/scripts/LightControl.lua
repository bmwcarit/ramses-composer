
-- This script controls the lighting of a Phong-like shading model
-- The light position is static, but can be switched from three
-- different positions: left, right, and top (use numbers 0, 1, 2 on light_id to toggle)
-- Optionally specify a diffuse_color to override the default (green-ish) color

modules("light")

function interface(IN,OUT)
    -- Input: index into an array static light positions
    IN.light_id = Type:Int32()
    -- Input: diffuse color (setting to zero causes the script to use its default value, see init())
    IN.diffuse_color = Type:Vec3f()
    -- Input: light color (setting to zero causes the script to use its default value, see init())
    IN.light_color = Type:Vec3f()

    -- Output: direction of light in that static position
    OUT.light_direction = Type:Vec3f()
    -- Output: diffuse color for material(s)
    OUT.diffuse_color = Type:Vec3f()
    -- Output: light color
    OUT.light_color = Type:Vec3f()
end

function run(IN,OUT)
    -- use the light module to store/resolve light data (colors, direction)
    OUT.light_direction = light.getLightDirection(IN.light_id)
    OUT.light_color     = light.resolveColor("light", IN.light_color)
    OUT.diffuse_color   = light.resolveColor("diffuse", IN.diffuse_color)
end
