#pragma once
#include "IScript.hpp"
#include "../Core/GameObject.hpp"

namespace Engine::Scripting
{
	class CppScriptComponent : public Core::Component, public IScript
	{
	public:
		explicit CppScriptComponent(Core::GameObject* owner)
			: m_owner(owner) { }

		virtual ~CppScriptComponent() = default;

		//Component のライフサイクルと同期
		void start()override { OnStart(); }
		void update(float dt)override { OnUpdate(dt); }
		void lateUpdate(float dt) override {}
		void onDestroy()override { OnDestroy(); }

		//IScriptの継承メソッド
		virtual void OnStart() override {};
		virtual void OnUpdate(float dt) override{}
		virtual void OnDestroy() override {}

	private:
		Core::GameObject* m_owner = nullptr;
	}; 
}