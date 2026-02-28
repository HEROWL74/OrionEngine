//src/engine/Input/InputSystem.hpp
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

		bool isKeyWPressed() const
		{
			return m_inputManager && m_inputManager->isKeyPressed(KeyCode::W);
		}

		bool isKeySPressed() const
		{
			return m_inputManager && m_inputManager->isKeyPressed(KeyCode::S);
		}

		bool isKeyAPressed() const
		{
			return m_inputManager && m_inputManager->isKeyPressed(KeyCode::A);
		}

		bool isKeyDPressed() const
		{
			return m_inputManager && m_inputManager->isKeyPressed(KeyCode::D);
		}

		bool isKeySpacePressed() const
		{
			return m_inputManager && m_inputManager->isKeyPressed(KeyCode::Space);
		}

		bool isKeyWReleased() const
		{
			return m_inputManager && m_inputManager->isKeyReleased(KeyCode::W);
		}

		bool isKeySReleased() const
		{
			return m_inputManager && m_inputManager->isKeyReleased(KeyCode::S);
		}

		bool isKeyAReleased() const
		{
			return m_inputManager && m_inputManager->isKeyReleased(KeyCode::A);
		}

		bool isKeyDReleased() const
		{
			return m_inputManager && m_inputManager->isKeyReleased(KeyCode::D);
		}

		bool isKeySpaceReleased() const
		{
			return m_inputManager && m_inputManager->isKeyReleased(KeyCode::Space);
		}

	private:
		InputSystem() = default;
		InputManager* m_inputManager = nullptr;
	};
}

