// src/editor/UI/ContextMenu.cpp - UIText作成対応版
#include "ContextMenu.hpp"
#include <imgui.h>
#include "../engine/Utils/Common.hpp"
#include <format>

namespace Editor::UI
{
    bool ContextMenu::drawHierarchyContextMenu()
    {
        bool actionPerformed = false;

        if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            drawCreateMenu();
            actionPerformed = true;
            ImGui::EndPopup();
        }

        return actionPerformed;
    }

    bool ContextMenu::drawGameObjectContextMenu(Engine::Core::GameObject* selectedObject)
    {
        if (!selectedObject) return false;
        if (selectedObject->isDestroyed()) return false;

        bool actionPerformed = false;

        if (ImGui::BeginPopupContextItem("GameObjectContextMenu"))
        {
            if (ImGui::MenuItem("Rename"))
            {
                m_showRenameDialog = true;
                m_renameTarget = selectedObject;
                std::string name = selectedObject->getName();
                strncpy_s(m_renameBuffer, sizeof(m_renameBuffer), name.c_str(), _TRUNCATE);
                actionPerformed = true;
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                if (m_duplicateObjectCallback)
                {
                    m_duplicateObjectCallback(selectedObject);
                }
                actionPerformed = true;
            }

            if (ImGui::MenuItem("Delete"))
            {
                m_deleteTarget = selectedObject;
                m_showDeleteConfirm = true;
                actionPerformed = true;
            }

            ImGui::Separator();
            drawCreateMenu();
            ImGui::EndPopup();
        }

        return actionPerformed;
    }

    bool ContextMenu::drawUITextContextMenu(Engine::EngineUI::UIText* selectedText)
    {
        bool actionTaken = false;

        if (ImGui::MenuItem("Rename"))
        {
            m_showUITextRenameDialog = true;
            m_uiTextRenameTarget = selectedText;
            strncpy_s(m_renameBuffer, selectedText->getName().c_str(), sizeof(m_renameBuffer) - 1);
            actionTaken = true;
        }

        if (ImGui::MenuItem("Edit Text"))
        {
            m_showUITextEditDialog = true;
            m_uiTextEditTarget = selectedText;
            strncpy_s(m_editBuffer, selectedText->getText().c_str(), sizeof(m_editBuffer) - 1);
            actionTaken = true;
        }

        if (ImGui::MenuItem("Delete"))
        {
            m_showUITextDeleteConfirm = true;
            m_uiTextDeleteTarget = selectedText;
            actionTaken = true;
        }

        return actionTaken;
    }

    void ContextMenu::drawModals()
    {
        // === GameObject削除確認ダイアログ ===
        if (m_deleteTarget && m_deleteTarget->isDestroyed())
        {
            m_deleteTarget = nullptr;
            m_showDeleteConfirm = false;
        }

        if (m_showDeleteConfirm && m_deleteTarget)
        {
            if (!ImGui::IsPopupOpen("Delete Confirmation"))
            {
                ImGui::OpenPopup("Delete Confirmation");
            }

            if (ImGui::BeginPopupModal("Delete Confirmation", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                std::string targetName = m_deleteTarget->getName();

                ImGui::Text("Are you sure you want to delete:");
                ImGui::Text("\"%s\"?", targetName.c_str());
                ImGui::Separator();

                if (ImGui::Button("Delete", ImVec2(100, 0)))
                {
                    if (m_deleteObjectCallback && m_deleteTarget)
                    {
                        Engine::Core::GameObject* toDelete = m_deleteTarget;
                        m_deleteTarget = nullptr;
                        m_showDeleteConfirm = false;
                        ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                        m_deleteObjectCallback(toDelete);
                        return;
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(100, 0)))
                {
                    m_deleteTarget = nullptr;
                    m_showDeleteConfirm = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        // === GameObjectリネームダイアログ ===
        if (m_renameTarget && m_renameTarget->isDestroyed())
        {
            m_renameTarget = nullptr;
            m_showRenameDialog = false;
        }

        if (m_showRenameDialog && m_renameTarget)
        {
            if (!ImGui::IsPopupOpen("Rename Object"))
            {
                ImGui::OpenPopup("Rename Object");
            }

            if (ImGui::BeginPopupModal("Rename Object", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::Text("Enter new name:");

                if (ImGui::InputText("##RenameInput", m_renameBuffer, sizeof(m_renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    if (m_renameObjectCallback && strlen(m_renameBuffer) > 0 && m_renameTarget)
                    {
                        Engine::Core::GameObject* toRename = m_renameTarget;
                        std::string newName = m_renameBuffer;
                        m_renameTarget = nullptr;
                        m_showRenameDialog = false;
                        ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                        m_renameObjectCallback(toRename, newName);
                        return;
                    }
                }

                if (ImGui::Button("OK", ImVec2(100, 0)))
                {
                    if (m_renameObjectCallback && strlen(m_renameBuffer) > 0 && m_renameTarget)
                    {
                        Engine::Core::GameObject* toRename = m_renameTarget;
                        std::string newName = m_renameBuffer;
                        m_renameTarget = nullptr;
                        m_showRenameDialog = false;
                        ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                        m_renameObjectCallback(toRename, newName);
                        return;
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(100, 0)))
                {
                    m_renameTarget = nullptr;
                    m_showRenameDialog = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        // UIText Rename Dialog
        if (m_showUITextRenameDialog)
        {
            ImGui::OpenPopup("Rename UIText");
            m_showUITextRenameDialog = false;
        }

        if (ImGui::BeginPopupModal("Rename UIText", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter new name:");
            ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer));

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                if (m_renameUITextCallback && m_uiTextRenameTarget)
                {
                    m_renameUITextCallback(m_uiTextRenameTarget, m_renameBuffer);
                }
                m_uiTextRenameTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_uiTextRenameTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // UIText Edit Dialog
        if (m_showUITextEditDialog)
        {
            ImGui::OpenPopup("Edit Text Content");
            m_showUITextEditDialog = false;
        }

        if (ImGui::BeginPopupModal("Edit Text Content", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter text content:");
            ImGui::InputTextMultiline("##edittext", m_editBuffer, sizeof(m_editBuffer), ImVec2(300, 100));

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                if (m_uiTextEditTarget)
                {
                    m_uiTextEditTarget->setText(m_editBuffer);
                }
                m_uiTextEditTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_uiTextEditTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // UIText Delete Confirmation
        if (m_showUITextDeleteConfirm)
        {
            ImGui::OpenPopup("Delete UIText?");
            m_showUITextDeleteConfirm = false;
        }

        if (ImGui::BeginPopupModal("Delete UIText?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete this UIText?");
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                if (m_deleteUITextCallback && m_uiTextDeleteTarget)
                {
                    m_deleteUITextCallback(m_uiTextDeleteTarget);
                }
                m_uiTextDeleteTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_uiTextDeleteTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void ContextMenu::drawCreateMenu()
    {
        if (ImGui::BeginMenu("Create"))
        {
            // 3D Objectサブメニュー
            if (ImGui::BeginMenu("3D Object"))
            {
                if (ImGui::MenuItem("Cube"))
                {
                    if (m_createObjectCallback)
                    {
                        std::string name = generateUniqueName("Cube");
                        auto* newObject = m_createObjectCallback(PrimitiveType::Cube, name);
                        if (newObject)
                        {
                            Engine::Utils::log_info(std::format("Created Cube: {}", name));
                        }
                    }
                }

                if (ImGui::MenuItem("Sphere"))
                {
                    if (m_createObjectCallback)
                    {
                        std::string name = generateUniqueName("Sphere");
                        auto* newObject = m_createObjectCallback(PrimitiveType::Sphere, name);
                        if (newObject)
                        {
                            Engine::Utils::log_info(std::format("Created Sphere: {}", name));
                        }
                    }
                }

                if (ImGui::MenuItem("Plane"))
                {
                    if (m_createObjectCallback)
                    {
                        std::string name = generateUniqueName("Plane");
                        auto* newObject = m_createObjectCallback(PrimitiveType::Plane, name);
                        if (newObject)
                        {
                            Engine::Utils::log_info(std::format("Created Plane: {}", name));
                        }
                    }
                }

                if (ImGui::MenuItem("Cylinder"))
                {
                    if (m_createObjectCallback)
                    {
                        std::string name = generateUniqueName("Cylinder");
                        auto* newObject = m_createObjectCallback(PrimitiveType::Cylinder, name);
                        if (newObject)
                        {
                            Engine::Utils::log_info(std::format("Created Cylinder: {}", name));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            // UIサブメニュー
            if (ImGui::MenuItem("Text"))
            {
               Engine::Utils::log_info("ContextMenu: 'Text' menu item clicked");

                if (m_createUIElementCallback)
                {
                    std::string name = generateUniqueName("UIText");
                    Engine::Utils::log_info(std::format("ContextMenu: Calling createUIElementCallback with name '{}'", name));

                    auto* result = m_createUIElementCallback(UIElementType::Text, name);

                    if (result)
                    {
                        Engine::Utils::log_info(std::format("ContextMenu: UIText created successfully: '{}'", result->getName()));
                    }
                    else
                    {
                        Engine::Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown, "Failed to create UIText"));
                    }
                }
                else
                {
                    Engine::Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown, "createUIElementCallback is NULL!"));
                }
                }
 
                if (ImGui::MenuItem("Image"))
                {
                    Engine::Utils::log_info("UI Image creation not yet implemented");
                }

                if (ImGui::MenuItem("Button"))
                {
                    Engine::Utils::log_info("UI Button creation not yet implemented");
                }

                ImGui::EndMenu();
            

            ImGui::Separator();

            // その他のメニュー
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Directional Light"))
                {
                }
                if (ImGui::MenuItem("Point Light"))
                {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Camera"))
            {
                if (ImGui::MenuItem("Camera"))
                {
                }
                ImGui::EndMenu();
            }
        }
    }

    void ContextMenu::draw3DObjectMenu()
    {
        if (ImGui::BeginMenu("3D Object"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                if (m_createObjectCallback)
                {
                    std::string name = generateUniqueName("Cube");
                    auto* newObject = m_createObjectCallback(PrimitiveType::Cube, name);
                    if (newObject)
                    {
                        Engine::Utils::log_info(std::format("Created Cube: {}", name));
                    }
                }
            }

            if (ImGui::MenuItem("Sphere"))
            {
                if (m_createObjectCallback)
                {
                    std::string name = generateUniqueName("Sphere");
                    auto* newObject = m_createObjectCallback(PrimitiveType::Sphere, name);
                    if (newObject)
                    {
                        Engine::Utils::log_info(std::format("Created Sphere: {}", name));
                    }
                }
            }

            if (ImGui::MenuItem("Plane"))
            {
                if (m_createObjectCallback)
                {
                    std::string name = generateUniqueName("Plane");
                    auto* newObject = m_createObjectCallback(PrimitiveType::Plane, name);
                    if (newObject)
                    {
                        Engine::Utils::log_info(std::format("Created Plane: {}", name));
                    }
                }
            }

            if (ImGui::MenuItem("Cylinder"))
            {
                if (m_createObjectCallback)
                {
                    std::string name = generateUniqueName("Cylinder");
                    auto* newObject = m_createObjectCallback(PrimitiveType::Cylinder, name);
                    if (newObject)
                    {
                        Engine::Utils::log_info(std::format("Created Cylinder: {}", name));
                    }
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("UI"))
        {
            if (ImGui::MenuItem("Text"))
            {
                Engine::Utils::log_info("ContextMenu: 'Text' menu item clicked");

                if (m_createUIElementCallback)
                {
                    std::string name = generateUniqueName("UIText");
                    Engine::Utils::log_info(std::format("ContextMenu: Calling createUIElementCallback with name '{}'", name));

                    auto* result = m_createUIElementCallback(UIElementType::Text, name);

                    if (result)
                    {
                        Engine::Utils::log_info(std::format("ContextMenu: UIText created successfully: '{}'", result->getName()));
                    }
                    else
                    {
                        Engine::Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown, "Failed to create UIText"));
                    }
                }
                else
                {
                    Engine::Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown, "createUIElementCallback is NULL!"));
                }
            }

            if (ImGui::MenuItem("Image"))
            {
                Engine::Utils::log_info("UI Image creation not yet implemented");
            }

            if (ImGui::MenuItem("Button"))
            {
                Engine::Utils::log_info("UI Button creation not yet implemented");
            }

            ImGui::EndMenu();
        }

    }

    std::string ContextMenu::generateUniqueName(const std::string& baseName)
    {
        if (baseName.empty())
        {
            return "GameObject";
        }

        static int globalCounter = 0;
        globalCounter++;

        return baseName + "_" + std::to_string(globalCounter);
    }
}

