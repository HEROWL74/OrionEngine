//src/UI/ProjectWindow.cpp
#include "ProjectWindow.hpp"
#include <format>
#include <algorithm>
#include <fstream>

namespace Engine::UI
{
    ProjectWindow::ProjectWindow()
        : ImGuiWindow("Project", true)
    {
        refreshAssets();
    }

    void ProjectWindow::draw()
    {
        if (!m_visible) return;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 wp = vp->WorkPos;
        ImVec2 ws = vp->WorkSize;

        // レイアウト比率（好みで微調整OK）
        const float LEFT = 0.22f; // 左カラム (Hierarchy)
        const float RIGHT = 0.26f; // 右カラム (Inspector)
        const float BOTTOM = 0.28f; // 下カラム (Project)

        // 画面サイズ変更を検知して、その時だけ強制反映
        static ImVec2 prevDisplay(0, 0);
        ImGuiIO& io = ImGui::GetIO();
        bool resized = fabsf(prevDisplay.x - io.DisplaySize.x) > 1.0f ||
            fabsf(prevDisplay.y - io.DisplaySize.y) > 1.0f;
        prevDisplay = io.DisplaySize;
        ImGuiCond cond = resized ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

        // Project は最下段フル幅
        ImVec2 projPos = ImVec2(wp.x, wp.y + ws.y * (1.0f - BOTTOM));
        ImVec2 projSize = ImVec2(ws.x, ws.y * BOTTOM);
        ImGui::SetNextWindowPos(projPos, cond);
        ImGui::SetNextWindowSize(projSize, cond);

        if (ImGui::Begin(m_title.c_str(), &m_visible))
        {
            drawToolbar();
            ImGui::Separator();

         
            if (ImGui::BeginChild("AssetArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
            {
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                    m_selectedAsset = nullptr;
                }
                if (m_showGrid) drawAssetGrid();
                else            drawAssetList();
                // drawContextMenu(); ← ここは削除
            }
            ImGui::EndChild();

            drawContextMenu();

            // ここで一括処理
            if (!m_pendingPathChange.empty()) {
                setProjectPath(m_pendingPathChange);
                m_pendingPathChange.clear();
            }
 
            drawAssetPreview();
        }
        ImGui::End();
    }

    void ProjectWindow::setProjectPath(const std::string& path)
    {
        m_projectPath = path;
        refreshAssets();
    }

    void ProjectWindow::refreshAssets()
    {
        m_assets.clear();
        m_selectedAsset = nullptr;

        if (!std::filesystem::exists(m_projectPath))
        {
            return;
        }

        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(m_projectPath))
            {
                AssetInfo asset;
                asset.path = entry.path();
                asset.name = entry.path().filename().string();
                asset.extension = entry.path().extension().string();
                asset.type = getAssetType(entry.path());

                m_assets.push_back(asset);
            }

            // 蜷榊燕縺ｧ繧ｽ繝ｼ繝・
            std::sort(m_assets.begin(), m_assets.end(),
                [](const AssetInfo& a, const AssetInfo& b) {
                    if (a.type != b.type)
                    {
                        return a.type < b.type; // 繝輔か繝ｫ繝繧貞・縺ｫ
                    }
                    return a.name < b.name;
                });
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::format("Failed to refresh assets: {}", e.what()));
        }
    }

    void ProjectWindow::drawToolbar()
    {

        char searchBuffer[256];
        strncpy_s(searchBuffer, m_searchFilter.c_str(), sizeof(searchBuffer) - 1);
        if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer)))
        {
            m_searchFilter = searchBuffer;
        }

        ImGui::SameLine();

        if (ImGui::Button(m_showGrid ? "List" : "Grid"))
        {
            m_showGrid = !m_showGrid;
        }

        ImGui::SameLine();


        if (ImGui::Button("Up"))
        {
            std::filesystem::path parent = std::filesystem::path(m_projectPath).parent_path();
            if (!parent.empty() && std::filesystem::exists(parent))
            {
                setProjectPath(parent.string());
            }
        }
        ImGui::SameLine();


        if (ImGui::Button("Refresh"))
        {
            refreshAssets();
        }

        ImGui::SameLine();


        if (m_showGrid)
        {
            ImGui::SetNextItemWidth(100);
            ImGui::SliderFloat("Size", &m_iconSize, 32.0f, 128.0f);
        }
    }

    void ProjectWindow::drawAssetGrid()
    {
        const float panelWidth = ImGui::GetContentRegionAvail().x;
        const float cellSize = m_iconSize + 16.0f;
        const int columnCount = max(1, (int)(panelWidth / cellSize));

        int index = 0;
        for (auto& asset : m_assets)
        {
            if (!matchesFilter(asset)) continue;

            ImGui::PushID(index);

         
            if (index > 0 && (index % columnCount) != 0)
            {
                ImGui::SameLine();
            }

          
            ImGui::BeginGroup();

            
            bool isSelected = (m_selectedAsset == &asset);
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            if (asset.type == AssetInfo::Type::Folder) {
                Utils::log_info("Drawing folder icon, ID=" + std::to_string((uint64_t)m_folderIconID));
                if (m_folderIconID)
                    ImGui::Image(m_folderIconID, ImVec2(m_iconSize, m_iconSize));
            }
            else if (asset.extension == ".lua") {
                Utils::log_info("Drawing lua icon, ID=" + std::to_string((uint64_t)m_luaIconID));
                if (m_luaIconID)
                    ImGui::Image(m_luaIconID, ImVec2(m_iconSize, m_iconSize));
            }
            else
            {
                ImGui::Button("##icon", ImVec2(m_iconSize, m_iconSize));
            }


            if (isSelected)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                std::string clickedPath = asset.path.string(); // ← コピーしておく

                if (asset.type == AssetInfo::Type::Folder) {
                    m_pendingPathChange = clickedPath; // 次の処理にまわす
                }
                else if (asset.type == AssetInfo::Type::Script && !asset.renaming) {
                    if (asset.extension == ".lua")
                        Engine::Scripting::LuaScriptUtility::openInVSCode(clickedPath);
                    else if (asset.extension == ".cpp")
                        Engine::Scripting::CppScriptUtility::openInVSCode(clickedPath);
                }
            }

        
            handleDragDrop(asset);
            // 名前部分の描画
            if (asset.renaming)
            {
                ImGui::SetNextItemWidth(m_iconSize + 20);

                if (ImGui::InputText("##rename", asset.renameBuffer, sizeof(asset.renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    std::filesystem::path newPath = asset.path.parent_path() / asset.renameBuffer;

                    // 拡張子補正
                    if (asset.extension == ".lua") {
                        newPath.replace_extension(".lua");
                    }
                    else if (asset.extension == ".cpp") {
                        newPath.replace_extension(".cpp");
                    }

                    try {
                        std::filesystem::rename(asset.path, newPath);

                        // 🔧 C++ の場合、中の class 名も置換
                        if (asset.extension == ".cpp") {
                            std::ifstream in(newPath);
                            std::stringstream buffer;
                            buffer << in.rdbuf();
                            in.close();

                            std::string content = buffer.str();
                            std::string oldClassName = asset.path.stem().string(); // リネーム前の名前
                            std::string newClassName = newPath.stem().string();

                            // 簡易置換
                            size_t pos = content.find("class " + oldClassName);
                            if (pos != std::string::npos) {
                                content.replace(pos + 6, oldClassName.size(), newClassName);

                                std::ofstream out(newPath, std::ios::trunc);
                                out << content;
                                out.close();
                            }
                        }

                        asset.path = newPath;
                        asset.name = newPath.filename().string();
                        asset.extension = newPath.extension().string();
                        asset.renaming = false;
                        refreshAssets();
                    }
                    catch (...) {
                        Utils::log_warning("Rename failed");
                    }
                }


                // Escでキャンセルできるようにする（任意）
                if (ImGui::IsItemDeactivatedAfterEdit() && !ImGui::IsItemActive())
                {
                    asset.renaming = false;
                }
            }
            else
            {
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + m_iconSize);
                ImGui::TextWrapped("%s", asset.name.c_str());
                ImGui::PopTextWrapPos();
            }


            ImGui::EndGroup();

            ImGui::PopID();
            index++;
        }
    }

    void ProjectWindow::drawAssetList()
    {
        if (ImGui::BeginTable("AssetTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Size");
            ImGui::TableHeadersRow();

            int index = 0;
            for (auto& asset : m_assets)
            {
                if (!matchesFilter(asset)) continue;

                ImGui::PushID(index);
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                bool isSelected = (m_selectedAsset == &asset);

                if (asset.renaming)
                {
                    ImGui::SetNextItemWidth(200);

                    if (ImGui::InputText("##rename", asset.renameBuffer, sizeof(asset.renameBuffer),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        std::filesystem::path newPath = asset.path.parent_path() / asset.renameBuffer;
                        try {
                            std::filesystem::rename(asset.path, newPath);
                            asset.path = newPath;
                            asset.name = newPath.filename().string();
                            asset.renaming = false;
                            refreshAssets();
                        }
                        catch (...) {
                            Utils::log_warning("Rename failed");
                        }
                    }

                    if (ImGui::IsItemDeactivatedAfterEdit() && !ImGui::IsItemActive())
                    {
                        asset.renaming = false;
                    }
                }
                else
                {
                    if (ImGui::Selectable(asset.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        m_selectedAsset = &asset;
                        loadAssetPreview(asset);
                    }
                }




                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    std::string clickedPath = asset.path.string();

                    if (asset.type == AssetInfo::Type::Folder) {
                        m_pendingPathChange = clickedPath;
                    }
                    else if (asset.type == AssetInfo::Type::Script && !asset.renaming) {
                        if (asset.extension == ".lua")
                            Engine::Scripting::LuaScriptUtility::openInVSCode(clickedPath);
                        else if (asset.extension == ".cpp")
                            Engine::Scripting::CppScriptUtility::openInVSCode(clickedPath);
                    }
                }

             
                handleDragDrop(asset);

      
                ImGui::TableNextColumn();
                const char* typeStr = "Unknown";
                switch (asset.type)
                {
                case AssetInfo::Type::Folder: typeStr = "Folder"; break;
                case AssetInfo::Type::Texture: typeStr = "Texture"; break;
                case AssetInfo::Type::Material: typeStr = "Material"; break;
                case AssetInfo::Type::Shader: typeStr = "Shader"; break;
                }
                ImGui::Text("%s", typeStr);

          
                ImGui::TableNextColumn();
                if (asset.type != AssetInfo::Type::Folder)
                {
                    try
                    {
                        auto size = std::filesystem::file_size(asset.path);
                        if (size < 1024)
                            ImGui::Text("%llu B", size);
                        else if (size < static_cast<unsigned long long>(1024) * 1024)
                            ImGui::Text("%.1f KB", size / 1024.0f);
                        else
                            ImGui::Text("%.1f MB", size / (1024.0f * 1024.0f));
                    }
                    catch (...)
                    {
                        ImGui::Text("-");
                    }
                }

                ImGui::PopID();
                index++;
            }

            ImGui::EndTable();
        }
    }

    void ProjectWindow::drawAssetPreview()
    {
        if (!m_selectedAsset) return;

        if (ImGui::BeginChild("Preview", ImVec2(0, 100), true))
        {
            ImGui::Text("Selected: %s", m_selectedAsset->name.c_str());
            ImGui::Text("Path: %s", m_selectedAsset->path.string().c_str());

            switch (m_selectedAsset->type)
            {
            case AssetInfo::Type::Texture:
                if (m_selectedAsset->texture)
                {
                    ImGui::Text("Texture: %dx%d",
                        m_selectedAsset->texture->getWidth(),
                        m_selectedAsset->texture->getHeight());
                }
                break;

            case AssetInfo::Type::Material:
                if (m_selectedAsset->material)
                {
                    const auto& props = m_selectedAsset->material->getProperties();
                    ImGui::Text("Albedo: %.2f, %.2f, %.2f", props.albedo.x, props.albedo.y, props.albedo.z);
                    ImGui::Text("Metallic: %.2f, Roughness: %.2f", props.metallic, props.roughness);
                }
                break;
            }
        }
        ImGui::EndChild();
    }

    void ProjectWindow::drawContextMenu()
    {
        if (ImGui::BeginPopupContextWindow("ProjectWindowContext",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {

            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Lua Script"))
                {
                    std::string newScriptPath = generateUniqueScriptPath();
                    newScriptPath = Engine::Scripting::LuaScriptUtility::normalizePath(newScriptPath);

                    if (Engine::Scripting::LuaScriptUtility::createNewScript(newScriptPath))
                    {
                        Utils::log_info(std::format("Lua script created: {}", newScriptPath));
                        refreshAssets();

                        for (auto& asset : m_assets)
                        {
                            if (asset.path == newScriptPath)
                            {
                                m_selectedAsset = &asset;
                                asset.renaming = true;
                                strncpy_s(asset.renameBuffer, asset.name.c_str(), sizeof(asset.renameBuffer));
                                break;
                            }
                        }
                    }
                }

                if (ImGui::MenuItem("C++ Script")) {
                    std::string path = "assets/scripts/NewCppScript";
                    path = Engine::Scripting::CppScriptUtility::normalizePath(path);

                    if (Engine::Scripting::CppScriptUtility::createNewScript(path)) {
                        Utils::log_info(std::format("Created C++ script: {}", path));
                        refreshAssets();

                        for (auto& asset : m_assets) {
                            if (asset.path == path) {
                                m_selectedAsset = &asset;
                                asset.renaming = true;
                                strncpy_s(asset.renameBuffer, asset.name.c_str(), sizeof(asset.renameBuffer));
                                break;
                            }
                        }
                    }
                }


                ImGui::EndMenu();
            }


            if (m_selectedAsset && ImGui::MenuItem("Delete"))
            {
                try {
                    std::filesystem::remove(m_selectedAsset->path);
                    Utils::log_info(std::format("Deleted asset: {}", m_selectedAsset->path.string()));
                    refreshAssets();
                    m_selectedAsset = nullptr;
                }
                catch (...) {
                    Utils::log_warning("Failed to delete asset");
                }
            }
            ImGui::EndPopup();
        }
        
    }


    AssetInfo::Type ProjectWindow::getAssetType(const std::filesystem::path& path)
    {
        if (std::filesystem::is_directory(path))
        {
            return AssetInfo::Type::Folder;
        }

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds")
        {
            return AssetInfo::Type::Texture;
        }
        else if (ext == ".json" && path.stem().string().find("_material") != std::string::npos)
        {
            return AssetInfo::Type::Material;
        }
        else if (ext == ".hlsl")
        {
            return AssetInfo::Type::Shader;
        }
        else if (ext == ".lua" || ext == ".cpp")
        {
            return AssetInfo::Type::Script;
        }

        return AssetInfo::Type::Unknown;
    }

    void ProjectWindow::loadAssetPreview(AssetInfo& asset)
    {
        switch (asset.type)
        {
        case AssetInfo::Type::Texture:
            if (m_textureManager && !asset.texture)
            {
                asset.texture = m_textureManager->loadTexture(asset.path.string());
            }
            break;

        case AssetInfo::Type::Material:
            if (m_materialManager && !asset.material)
            {
                asset.material = m_materialManager->createMaterial(asset.name);
            }
            break;
        }
    }

    bool ProjectWindow::matchesFilter(const AssetInfo& asset) const
    {
        if (m_searchFilter.empty()) return true;

        std::string lowerName = asset.name;
        std::string lowerFilter = m_searchFilter;

        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

        return lowerName.find(lowerFilter) != std::string::npos;
    }

    void ProjectWindow::handleDragDrop(const AssetInfo& asset)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            AssetPayload payload{};
            auto s = asset.path.string();
            strncpy_s(payload.path, s.c_str(), sizeof(payload.path) - 1);
            payload.type = static_cast<int>(asset.type);

            //POD を渡す（ImGui 内部でコピーされるのでローカルでOK）
            ImGui::SetDragDropPayload("ASSET", &payload, sizeof(payload));

            ImGui::Text("Dragging: %s", asset.name.c_str());
            ImGui::EndDragDropSource();
        }
    }

    std::string ProjectWindow::generateUniqueScriptPath()
    {
        int counter = 0;
        std::string baseName = "NewLuaScript";
        std::string fileName;
        std::filesystem::path scriptPath;

        do
        {
            fileName = (counter == 0)
                ? baseName
                : baseName + " " + std::to_string(counter);

            scriptPath = std::filesystem::path(m_projectPath) / fileName;
            counter++;
        } while (std::filesystem::exists(scriptPath.string() + ".lua"));

        return scriptPath.string(); // 拡張子なしで返す
    }

    void ProjectWindow::setTextureManager(Graphics::TextureManager* textureManager)
    {
        m_textureManager = textureManager;

        if (m_textureManager && m_imguiManager)
        {
            m_folderIcon = m_textureManager->loadTexture("engine-assets/images/ProjectWindowFolder.png");
            m_luaIcon = m_textureManager->loadTexture("engine-assets/images/LuaFileImage.png");

            if (m_folderIcon) {
                m_folderIconID = m_imguiManager->registerTexture(m_folderIcon.get());
                Utils::log_info("FolderIcon registered, ID=" + std::to_string((uint64_t)m_folderIconID));
            }
            else {
                Utils::log_warning("Failed to load folder icon!");
            }

            if (m_luaIcon) {
                m_luaIconID = m_imguiManager->registerTexture(m_luaIcon.get());
                Utils::log_info("LuaIcon registered, ID=" + std::to_string((uint64_t)m_luaIconID));
            }
            else {
                Utils::log_warning("Failed to load lua icon!");
            }
        }
    }

}
