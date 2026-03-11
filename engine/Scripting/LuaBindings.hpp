//src/Scripting/LuaBindings.hpp
#pragma once

#include <sol/sol.hpp>
#include "../World/Camera.hpp"
#include "../Math/Math.hpp"
#include "../Core/GameObject.hpp"
#include "../Input/InputSystem.hpp"
#include "../Physics/PhysicsSystem.hpp"
#include "../UI/UIComponent.hpp"
#include "../Audio/AudioComponent.hpp"
#include "GameObjectHandle.hpp"

namespace Engine::Scripting
{
	class LuaBindings
	{
	private:
	     void bindMath(sol::state& lua);

		 void bindCamera(sol::state& lua);

		 void bindInput(sol::state& lua);

		 void bindPhysics(sol::state& lua);

		 void bindAudio(sol::state& lua);

		 void bindUIText(sol::state& lua);

		 void bindGameObjectHandle(sol::state& lua);
	public:
	    void registerBindings(sol::state& lua);
	};
}

