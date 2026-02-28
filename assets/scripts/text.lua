-- assets/scripts/text.lua
-- ゲームUIスクリプト

local Script = {}

function Script.onStart(obj)
    print("UI Script started on: " .. obj:getName())
end

function Script.onUpdate(obj, dt)
    local uiText = obj:getUIText()
    if uiText == nil then return end

    local input = InputSystem.get()

    if not GameState.isPlaying and not GameState.isGameOver then
        uiText:setText("Press SPACE to Start")

        if input:isKeySpacePressed() then
            GameState.isPlaying = true
            GameState.score = 0
            print("Game Started!")
        end
    elseif GameState.isPlaying and not GameState.isGameOver then
        uiText:setText("Score: " .. GameState.score)
    else
        uiText:setText("GAME OVER! Score: " .. GameState.score)

        if input:isKeySpacePressed() then
            GameState.isGameOver = false
            GameState.score = 0
            GameState.isPlaying = false
            Editor.restartGame()
        end
    end
end


return Script