-- assets/scripts/Obstacle.lua
-- 敵オブジェクトスクリプト

local Script = {}

-- スクリプト内プライベート変数（グローバルに漏れない）
local spawnZ = -10.0
local moveSpeed = 3.0
local minX = -5.0
local maxX = 5.0

function Script.onStart(obj)
    math.randomseed(os.time())
    local transform = obj:getTransform()
    local x = math.random() * (maxX - minX) + minX
    transform:setPosition(x, 0, spawnZ)
end

function Script.onUpdate(obj, dt)
    if not GameState.isPlaying or GameState.isGameOver then
        return
    end

    local transform = obj:getTransform()
    if transform == nil then return end

    transform:move(0, 0, moveSpeed * dt)

    local pos = transform:getPosition()
    if pos.z > 10.0 then
        local x = math.random() * (maxX - minX) + minX
        transform:setPosition(x, pos.y, spawnZ)

        GameState.score = GameState.score + 1
        print("Score: " .. GameState.score)
    end
end


return Script