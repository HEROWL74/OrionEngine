//src/engine/Input/InputManager
#pragma once

#include "InputManager.hpp"
#include <memory>

namespace Engine::Input
{
	// Luaから簡単にキー入力を取得するためのクラス
	class InputSystem
	{
	public:
		static InputSystem& get()
		{
			static InputSystem instance;
			return instance;
		}

		void setInputManager(InputManager* manager)
		{
			m_inputManager = manager;
		}

		// WASD, SPACEのみ返す
		bool isKeyW() const
		{
			return m_inputManager && m_inputManager->isKeyDown(KeyCode::W);
		}
		bool isKeyS() const
		{
			return m_inputManager && m_inputManager->isKeyDown(KeyCode::S);
		}
		bool isKeyA() const
		{
			return m_inputManager && m_inputManager->isKeyDown(KeyCode::A);
		}
		bool isKeyD() const
		{
			return m_inputManager && m_inputManager->isKeyDown(KeyCode::D);
		}

		bool isKeySpace() const
		{
			return m_inputManager && m_inputManager->isKeyDown(KeyCode::Space);
		}
	private: 
		InputSystem() = default;
		InputManager* m_inputManager = nullptr;
	};
} 
