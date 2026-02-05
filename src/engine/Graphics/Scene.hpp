#pragma once
#include "../Core/GameObject.hpp"
#include "Device.hpp"
#include "Camera.hpp"
#include "TriangleRenderer.hpp"
#include "CubeRenderer.hpp"
#include "Material.hpp"
#include "RenderComponent.hpp"
#include "ShaderManager.hpp"
#include "../UI/UIComponent.hpp"

namespace Engine::Graphics
{
	//==================================================================================
    // シーン管理クラス
    //==================================================================================
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		//コピー・ムーブ禁止
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		//ゲームオブジェクト管理
		Core::GameObject* createGameObject(const std::string& name = "GameObject");
		void destroyGameObject(Core::GameObject* gameObject);
		Core::GameObject* findGameObject(const std::string& name) const;

		// 選択管理
		void setSelectedObject(Core::GameObject* object);
		Core::GameObject* getSelectedObject() const;
		void clearSelection();

		//ライフサイクル
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);
		void clear();

		//レンダリング
		[[deprecated("Use GameView instead")]]
		void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//ゲームオブジェクト一覧取得
		const std::vector<std::unique_ptr<Core::GameObject>>& getGameObjects() const { return m_gameObjects; }
		Core::GameObject* findObjectByName(const std::string& name);

		EngineUI::UIText* createUIText(const std::string& name = "UIText")
		{
			auto gameObject = createGameObject(name);
			auto* uitext = gameObject->addComponent<EngineUI::UIText>();
			uitext->setName(name);
			return uitext;
		}

		std::vector<EngineUI::UIText*> getAllUITexts()
		{
			std::vector<EngineUI::UIText*> texts;
			for (const auto& obj : m_gameObjects)
			{
				if (auto* text = obj->getComponent<EngineUI::UIText>())
				{
					texts.push_back(text);
				}
			}
			return texts;
		}

		void removeUIText(EngineUI::UIText* text)
		{
			if (!text) return;

			// GameObjectからコンポーネントを削除
			auto* gameObject = text->getGameObject();
			if (gameObject)
			{
				gameObject->removeComponent<EngineUI::UIText>();
			}
		}

		std::vector<EngineUI::UIText*> getUITexts() const
		{
			std::vector<EngineUI::UIText*> texts;
			for (const auto& obj : m_gameObjects)
			{
				if (obj && obj->isActive() && !obj->isDestroyed())
				{
					if (auto* text = obj->getComponent<EngineUI::UIText>())
					{
						texts.push_back(text);
					}
				}
			}
			return texts;
		}
		Core::GameObject* findGameObjectById(Core::GameObject::ObjectID id) const;

		void clearUITexts()
		{
			m_uiTexts.clear();
		}

	private:
		Device* m_device = nullptr;
		std::vector<std::unique_ptr<Core::GameObject>> m_gameObjects;
		std::vector<Core::GameObject*> m_pendingDestroy;
		Core::GameObject* m_selectedObject = nullptr;
		bool m_initialized = false;
		std::vector<std::unique_ptr<Engine::EngineUI::UIText>> m_uiTexts;
	private:
		void processPendingDestroy();
	};
}
