//src/UI/ProjectWindow.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include "ImGuiManager.hpp"
#include "engine/Graphics/Texture.hpp"
#include "engine/Graphics/Material.hpp"
#include "engine/Scripting/LuaScriptUtility.hpp"

namespace Editor::UI
{
    //=========================================================================
    // アセット情報構造体
    //=========================================================================
    struct AssetInfo
    {
        std::filesystem::path path;
        std::string name;
        std::string extension;
        enum class Type
        {
            Folder,
            Texture,
            Material,
            Shader,
            Script,
            Unknown
        } type{};

        //リネーム情報格納
        bool renaming = false;
        char renameBuffer[256]{};

        std::shared_ptr<Engine::Graphics::Texture> texture; // テクスチャプレビュー用
        std::shared_ptr<Engine::Graphics::Material> material; // マテリアル用
    };

    struct AssetPayload
    {
        char path[256];
        int type;     // AssetInfo::Type を int にキャストして保存
    };

    //=========================================================================
    // プロジェクトウィンドウ
    //=========================================================================
    class ProjectWindow : public ImGuiWindow
    {
    public:
        ProjectWindow();
        ~ProjectWindow() = default;

        void draw() override;

        // 依存関係設定
        void setTextureManager(Engine::Graphics::TextureManager* textureManager);
        void setMaterialManager(Engine::Graphics::MaterialManager* materialManager) { m_materialManager = materialManager; }

        // プロジェクトパス設定
        void setProjectPath(const std::string& path);
        const std::string& getProjectPath() const { return m_projectPath; }

        std::string generateUniqueScriptPath();

        // 選択されたアセット取得
        const AssetInfo* getSelectedAsset() const { return m_selectedAsset; }

        // アセット操作コールバック
        void setAssetDropCallback(std::function<void(const AssetInfo&)> callback) { m_assetDropCallback = callback; }

        void setImGuiManager(ImGuiManager* manager) { m_imguiManager = manager; }

    private:
        Engine::Graphics::TextureManager* m_textureManager = nullptr;
        Engine::Graphics::MaterialManager* m_materialManager = nullptr;

        ImGuiManager* m_imguiManager = nullptr;

        std::string m_projectPath = "assets";
        std::vector<AssetInfo> m_assets;
        AssetInfo* m_selectedAsset = nullptr;
        std::shared_ptr<Engine::Graphics::Texture> m_folderIcon;
        std::shared_ptr<Engine::Graphics::Texture> m_luaIcon;

        ImTextureID m_folderIconID = 0;
        ImTextureID m_luaIconID = 0;

        // UI状態
        float m_iconSize = 64.0f;
        bool m_showGrid = true;
        std::string m_searchFilter;
        std::string m_pendingPathChange;

        // コールバック
        std::function<void(const AssetInfo&)> m_assetDropCallback;

        // プライベートメソッド
        void refreshAssets();
        void drawToolbar();
        void drawAssetGrid();
        void drawAssetList();
        void drawAssetPreview();
        void drawContextMenu();

        AssetInfo::Type getAssetType(const std::filesystem::path& path);
        void loadAssetPreview(AssetInfo& asset);
        bool matchesFilter(const AssetInfo& asset) const;

        // ドラッグ&ドロップ
        void handleDragDrop(const AssetInfo& asset);
    };
}
