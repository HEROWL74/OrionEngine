//src/UI/ContextMenu.hpp
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include "../engine/Core/GameObject.hpp"
#include "../engine/Scripting/LuaScriptUtility.hpp"
#include "../engine/Scripting/CppScriptUtility.hpp"
#include "../engine/UI/UIComponent.hpp"

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

	//UIオブジェクト作成の種類
	enum class UIElementType
	{
		Text,
		Image,
		Button
	};

	//コンテキストメニュー項目の種類
	enum class ContextMenuAction
	{
		CreateCube,
		CreateSphere,
		CreatePlane,
		CreateCylinder,
		CreateUIText,
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
		bool drawUITextContextMenu(Engine::EngineUI::UIText* selectedText);

		//コールバック設定
		void setCreateObjectCallback(std::function<Engine::Core::GameObject* (PrimitiveType, const std::string&)> callback)
		{
			m_createObjectCallback = callback;
		}

		void setCreateUIElementCallback(std::function<Engine::EngineUI::UIText* (UIElementType, const std::string&)> callback)
		{
			m_createUIElementCallback = callback;
		}

		void setDeleteObjectCallback(std::function<void(Engine::Core::GameObject*)> callback)
		{
			m_deleteObjectCallback = callback;
		}

		void setDeleteUITextCallback(std::function<void(Engine::EngineUI::UIText*)> callback)
		{
			m_deleteUITextCallback = callback;
		}

		void setDuplicateObjectCallback(std::function<Engine::Core::GameObject* (Engine::Core::GameObject*)> callback)
		{
			m_duplicateObjectCallback = callback;
		}

		void setRenameObjectCallback(std::function<void(Engine::Core::GameObject*, const std::string&)> callback)
		{
			m_renameObjectCallback = callback;
		}

		void setRenameUITextCallback(std::function<void(Engine::EngineUI::UIText*, const std::string&)> callback)
		{
			m_renameUITextCallback = callback;
		}

		auto getCreateUIElementCallback() const { return m_createUIElementCallback; }
		auto getDeleteUITextCallback() const { return m_deleteUITextCallback; }

		auto getCreateObjectCallback() const { return m_createObjectCallback; }
		auto getDeleteObjectCallback() const { return m_deleteObjectCallback; }
		void drawModals();
		void drawCreateMenu();
	private:
		//コールバック関数
		std::function<Engine::Core::GameObject* (PrimitiveType, const std::string&)> m_createObjectCallback;
		std::function<Engine::EngineUI::UIText* (UIElementType, const std::string&)> m_createUIElementCallback;
		std::function<void(Engine::Core::GameObject*)> m_deleteObjectCallback;
		std::function<void(Engine::EngineUI::UIText*)> m_deleteUITextCallback;
		std::function<Engine::Core::GameObject* (Engine::Core::GameObject*)> m_duplicateObjectCallback;
		std::function<void(Engine::Core::GameObject*, const std::string&)> m_renameObjectCallback;
		std::function<void(Engine::EngineUI::UIText*, const std::string&)> m_renameUITextCallback;

		//内部メソッド
		void draw3DObjectMenu();
		std::string generateUniqueName(const std::string& baseName);

		bool m_showRenameDialog = false;
		bool m_showDeleteConfirm = false;
		bool m_showUITextRenameDialog = false;
		bool m_showUITextEditDialog = false;
		bool m_showUITextDeleteConfirm = false;
		char m_renameBuffer[256] = "";
		char m_editBuffer[512] = "";
		Engine::Core::GameObject* m_renameTarget = nullptr;
		Engine::Core::GameObject* m_deleteTarget = nullptr;
		Engine::EngineUI::UIText* m_uiTextRenameTarget = nullptr;
		Engine::EngineUI::UIText* m_uiTextEditTarget = nullptr;
		Engine::EngineUI::UIText* m_uiTextDeleteTarget = nullptr;
	};
}

