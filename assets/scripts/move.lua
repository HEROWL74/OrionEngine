-- assets/scripts/move.lua
-- プレイヤー移動スクリプト

local Script = {}

function Script.onStart(obj)
    print("Player initialized")
end

function Script.onUpdate(obj, dt)
    if not GameState.isPlaying or GameState.isGameOver then
        return
    end

    local input = InputSystem.get()
    local transform = obj:getTransform()
    if transform == nil then return end

    local speed = 5.0

    if input:isKeyA() then
        transform:moveX(-speed * dt)
    end
    if input:isKeyD() then
        transform:moveX(speed * dt)
    end

    local pos = transform:getPosition()
    if pos.x < -5.0 then
        transform:setPosition(-5.0, pos.y, pos.z)
    end
    if pos.x > 5.0 then
        transform:setPosition(5.0, pos.y, pos.z)
    end
end


function Script.onCollisionEnter(obj, other)
    if GameState.isGameOver then
        return
    end
    print("Player hit: " .. other:getName())

    if other:getName() == "Enemy" or other:getName() == "Obstacle" then
        GameState.isGameOver = true
        print("GAME OVER!")
    end
end

function Script.onCollisionExit(obj, other)
end

return Script