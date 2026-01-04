#pragma once
#include <string>

namespace Engine::Scripting
{
	class IScript
	{
	public:
		virtual ~IScript() = default;

		//共通ライフサイクル
		virtual void OnStart(){}
		virtual void OnUpdate(float deltaTime){}
		virtual void OnDestroy(){}

		//デバッグ用にスクリプト名を返す
		virtual std::string GetName() const {return  "UnnamedScript";}
	};
}