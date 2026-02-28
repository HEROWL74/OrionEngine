-- assets/scripts/bgm.lua
-- BGM管理スクリプト

local Script = {}

function Script.onStart(obj)
    print("BGM initialized")
end

function Script.onUpdate(obj, dt)
    local audio = obj:getAudio()
    if audio == nil then return end

    audio:setLoop(true)
    audio:setVolume(0.5)

    if GameState.isPlaying and not GameState.isGameOver then
        if not audio:isPlaying() then
            audio:play()
        end
    else
        if audio:isPlaying() then
            audio:stop()
        end
    end
end


return Script