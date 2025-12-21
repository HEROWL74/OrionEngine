#pragma once
#include "../Core/GameObject.hpp"
#include "Device.hpp"
#include "Camera.hpp"
#include "TriangleRenderer.hpp"
#include "CubeRenderer.hpp"
#include "Material.hpp"
#include "RenderComponent.hpp"
#include "ShaderManager.hpp"

namespace Engine::Graphics
{
	//==================================================================================
     //シーン管理クラス
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

		//ライフサイクル
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);

		//レンダリング
		void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//ゲームオブジェクト一覧取得
		const std::vector<std::unique_ptr<Core::GameObject>>& getGameObjects() const { return m_gameObjects; }
		Core::GameObject* findObjectByName(const std::string& name);

	private:
		Device* m_device = nullptr;
		std::vector<std::unique_ptr<Core::GameObject>> m_gameObjects;
		bool m_initialized = false;
	};
}

