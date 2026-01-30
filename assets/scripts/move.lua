-- assets\scripts\move.lua

---@type OnStartFunc
---@diagnostic disable-next-line: lowercase-global
function onStart(obj)
    print("Hello lua")
end

---@type OnUpdateFunc
---@diagnostic disable-next-line: lowercase-global
function onUpdate(obj, dt)
    local input = InputSystem.get()
    local transform = obj:getTransform()
    local speed = 3.0
if GameState.isPlaying == true then
     if input:isKeyA() then
        transform:moveX(-speed * dt)
     end
        if input:isKeyD() then
        transform:moveX(speed * dt)
        end
    end 
end
