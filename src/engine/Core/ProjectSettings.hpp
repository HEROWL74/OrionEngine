// src/engine/Core/ProjectSettings.hpp
#pragma once

#include <string>
#include <filesystem>

namespace Engine::Core
{
	// ===============================
	// Window Settings
	// ===============================
	struct WindowConfig
	{
		int width = 1280;
		int height = 720;
		bool fullscreen = false;
		bool vsync = true;
	};

	class ProjectSettings
	{
	public:
		// Singleton
		static ProjectSettings& get();

		// Load
		bool load(const std::filesystem::path& jsonPath);

		/// エディタ用: project-templates/3d/ProjectSettings.json を探して読む。
		/// 見つからなければデフォルト値を使う。
		void loadForEditor();

		/// ランタイム用: assets/ProjectSettings.json など実行ファイル隣を探して読む。
		void loadForRuntime();

		// -----------------------
	    // アクセサ — JSON フィールド値
	    // ------------------------
		const std::string& getProjectName()   const { return m_projectName; }
		const std::string& getEngineVersion() const { return m_engineVersion; }
		const std::string& getDefaultScene()  const { return m_defaultScene; }
		const std::string& getAssetRoot()     const { return m_assetRoot; }
		const std::string& getProjectType()   const { return m_projectType; }
		const WindowConfig& getWindowConfig()  const { return m_window; }

		// -----------------------
		// アクセサ — 解決済みパス
		// -----------------------

		/// JSON ファイルが置かれているディレクトリ
		const std::filesystem::path& getProjectRootDir() const { return m_projectRootDir; }

		/// AssetRoot を projectRootDir 基準で解決したパス
		std::filesystem::path getAssetRootPath() const
		{
			return m_projectRootDir / m_assetRoot;
		}

		/// DefaultScene を projectRootDir 基準で解決したパス
		std::filesystem::path getDefaultScenePath() const
		{
			return m_projectRootDir / m_defaultScene;
		}

		/// ウィンドウタイトル用ワイド文字列（ProjectName をそのまま変換）
		std::wstring getProjectNameW() const;


	private:
		ProjectSettings() = default;
		~ProjectSettings() = default;
		ProjectSettings(const ProjectSettings&) = delete;
		ProjectSettings& operator=(const ProjectSettings&) = delete;

		// JSONファイルパスと、そのディレクトリ
		std::filesystem::path m_jsonPath;
		std::filesystem::path m_projectRootDir;

		// デフォルト値
		std::string m_projectName = "__PROJECT_NAME__";
		std::string m_engineVersion = "0.0.0";
		std::string m_defaultScene = "Assets/scenes/default.scene";
		std::string  m_assetRoot = "Assets";
		std::string  m_projectType = "3D";
		WindowConfig m_window{};
	};
}