//src/UI/ContextMenu.hpp
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include "engine/Core/GameObject.hpp"
#include "engine/Graphics/RenderComponent.hpp"
#include "engine/Scripting/LuaScriptUtility.hpp"
#include "engine/Scripting/CppScriptUtility.hpp"

namespace Editor::UI
{
	//3Dオブジェクト作成の種類
	enum class PrimitiveType
	{
		Cube,
		Sphere,
		Plane,
		Cylinder
	};

	//コンテキストメニュー項目の種類
	enum class ContextMenuAction
	{
		CreateCube,
		CreateSphere,
		CreatePlane,
		CreateCylinder,
		DeleteObject,
		DuplicateObject,
		RenameObject
	};

	//コンテキストメニュークラス
	class ContextMenu
	{
	public:
		ContextMenu() = default;
		~ContextMenu() = default;

		//右クリックメニュー表示
		bool drawHierarchyContextMenu();
		bool drawGameObjectContextMenu(Engine::Core::GameObject* selectedObject);

		//コールバック設定
		void setCreateObjectCallback(std::function<Engine::Core::GameObject* (PrimitiveType, const std::string&)> callback)
		{
			m_createObjectCallback = callback;
		}

		void setDeleteObjectCallback(std::function<void(Engine::Core::GameObject*)> callback)
		{
			m_deleteObjectCallback = callback;
		}

		void setDuplicateObjectCallback(std::function<Engine::Core::GameObject* (Engine::Core::GameObject*)> callback)
		{
			m_duplicateObjectCallback = callback;
		}

		void setRenameObjectCallback(std::function<void(Engine::Core::GameObject*, const std::string&)> callback)
		{
			m_renameObjectCallback = callback;
		}
		void drawModals();
	private:
		//コールバック関数
		std::function<Engine::Core::GameObject* (PrimitiveType, const std::string&)> m_createObjectCallback;
		std::function<void(Engine::Core::GameObject*)> m_deleteObjectCallback;
		std::function<Engine::Core::GameObject* (Engine::Core::GameObject*)> m_duplicateObjectCallback;
		std::function<void(Engine::Core::GameObject*, const std::string&)> m_renameObjectCallback;

		//内部メソッド
		void drawCreateMenu();
		void draw3DObjectMenu();
		std::string generateUniqueName(const std::string& baseName);

		bool m_showRenameDialog = false;
		bool m_showDeleteConfirm = false;
		char m_renameBuffer[256] = "";
		Engine::Core::GameObject* m_renameTarget = nullptr;
		Engine::Core::GameObject* m_deleteTarget = nullptr;
	};
}
