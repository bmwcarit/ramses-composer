
function interface(IN,OUT)
    IN.ticker = Type:Int64()
    OUT.animationProgress = Type:Float()
end

function run(IN,OUT)
    -- Total duration of the animation in seconds
    local durationInSeconds = 3
    -- How many microseconds are needed to fill the progress from 0 -> 1
    local normalizeFactor = 1000000 * durationInSeconds
    -- Convert timer ticks to progress and normalizing to [0, 1]
    local progress = (IN.ticker % normalizeFactor) / normalizeFactor
    OUT.animationProgress = progress
end
