//src/UI/ContextMenu.cpp
#include "ContextMenu.hpp"
#include "imgui.h"
#include "engine/Utils/Common.hpp"
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

    void ContextMenu::drawModals()
    {
        // Deleteボタン描画
        if (m_showDeleteConfirm && m_deleteTarget)
        {
            ImGui::OpenPopup("Delete Confirmation");

            if (ImGui::BeginPopupModal("Delete Confirmation", &m_showDeleteConfirm,
                ImGuiWindowFlags_AlwaysAutoResize))
            {
                std::string targetName = m_deleteTarget->getName();

                ImGui::Text("Are you sure you want to delete:");
                ImGui::Text("\"%s\"?", targetName.c_str());
                ImGui::Separator();

                if (ImGui::Button("Delete", ImVec2(100, 0)))
                {
                    if (m_deleteObjectCallback)
                    {
                        Engine::Core::GameObject* toDelete = m_deleteTarget;
                        m_deleteTarget = nullptr;
                        m_showDeleteConfirm = false;
                        ImGui::CloseCurrentPopup();
                        m_deleteObjectCallback(toDelete);
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

        // Renameボタン描画
        if (m_showRenameDialog && m_renameTarget)
        {
            ImGui::OpenPopup("Rename Object");

            if (ImGui::BeginPopupModal("Rename Object", &m_showRenameDialog,
                ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter new name:");

                if (ImGui::InputText("##RenameInput", m_renameBuffer, sizeof(m_renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    if (m_renameObjectCallback && strlen(m_renameBuffer) > 0)
                    {
                        Engine::Core::GameObject* toRename = m_renameTarget;
                        std::string newName = m_renameBuffer;
                        m_renameTarget = nullptr;
                        m_showRenameDialog = false;
                        ImGui::CloseCurrentPopup();
                        m_renameObjectCallback(toRename, newName);
                    }
                }

                if (ImGui::Button("OK", ImVec2(100, 0)))
                {
                    if (m_renameObjectCallback && strlen(m_renameBuffer) > 0)
                    {
                        Engine::Core::GameObject* toRename = m_renameTarget;
                        std::string newName = m_renameBuffer;
                        m_renameTarget = nullptr;
                        m_showRenameDialog = false;
                        ImGui::CloseCurrentPopup();
                        m_renameObjectCallback(toRename, newName);
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
    }

    void ContextMenu::drawCreateMenu()
    {
        if (ImGui::BeginMenu("Create"))
        {
            draw3DObjectMenu();
            
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

            ImGui::EndMenu();
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
    }

    std::string ContextMenu::generateUniqueName(const std::string& baseName)
    {
        if (baseName.empty())
        {
            return "GameObject";
        }

        //名前生成カウンター
        static int globalCounter = 0;
        globalCounter++;

        return baseName + "_" + std::to_string(globalCounter);
    }
}
