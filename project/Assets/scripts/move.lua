-- assets/scripts/move.lua
-- プレイヤー移動スクリプト

local Script = {}

function Script.onStart(obj)
    print("Player initialized")
end

function Script.onUpdate(obj, dt)
    local input = InputSystem.get()
    local transform = obj:getTransform()

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

return Script