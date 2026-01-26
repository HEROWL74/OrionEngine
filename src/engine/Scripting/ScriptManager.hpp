//src/Scripting/ScriptManager.hpp
#pragma once

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>

namespace Engine::Scripting
{
	// スクリプトの種類
	enum class ScriptType
	{
		Component,      // GameObjectにアタッチするスクリプト（onStart, onUpdateなど）
		ScriptableObject // グローバルに読み込まれるスクリプト（データ定義など）
	};

	class ScriptManager
	{
	public:
		ScriptManager() = default;
		~ScriptManager() = default;

		static ScriptManager& get();

		// Luaライブラリの初期化
		void initialize();

		// スクリプトを読み込み&キャッシュ
		bool loadScript(const std::string& path, ScriptType type = ScriptType::Component);

		// 指定ディレクトリ配下の全スクリプトをスキャンして、
		// ScriptableObjectマーカー（--@ScriptableObject）があるものを自動読み込み
		void scanAndLoadScriptableObjects(const std::string& rootDirectory = "scripts");

		// ファイル更新を監視して、変更があれば再読み込みする
		void checkForUpdates();

		// script_path -> 関数を取得
		sol::function getFunction(const std::string& path, const std::string& functionName) const;

		// グローバル変数へのアクセス
		sol::object getGlobal(const std::string& name) const;
		void setGlobal(const std::string& name, sol::object value);

		sol::state& getLuaState() { return m_lua; }

		// 全スクリプトを再読み込み（グローバル変数を保持）
		void reloadAll();

		// デバッグ用: グローバル変数を表示
		void dumpGlobals() const;

		// ScriptableObjectスクリプトのリストを取得
		const std::vector<std::string>& getScriptableObjects() const { return m_scriptableObjects; }

	private:
		sol::state m_lua;

		// スクリプトごとの関数キャッシュ
		struct ScriptData
		{
			std::filesystem::file_time_type lastWriteTime;
			std::unordered_map<std::string, sol::function> functions;
			ScriptType type;
		};

		std::unordered_map<std::string, ScriptData> m_scripts;
		std::vector<std::string> m_scriptableObjects;  // ScriptableObjectのパスリスト

		// ファイルがScriptableObjectかどうかをチェック
		bool isScriptableObject(const std::string& filepath) const;
	};
}