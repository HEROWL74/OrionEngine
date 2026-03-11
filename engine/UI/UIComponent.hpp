// src/engine/UI/UIComponent.hpp
#pragma once
#include "../renderer/Device.hpp"
#include "../renderer/ShaderManager.hpp"
#include "../Math/Math.hpp"
#include "../Core/GameObject.hpp"

namespace Engine::EngineUI
{
    using namespace Renderer;

    enum class UIAnchor
    {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    enum class UIRenderMode
    {
        ScreenSpace,    // 2D
        WorldSpace      // 3D
    };

    // ======================================
    // UIComponent
    // ======================================
    class UIComponent : public Core::Component
    {
    public:
        explicit UIComponent() = default;
        virtual ~UIComponent() = default;

        UIComponent(const UIComponent&) = delete;
        UIComponent& operator=(const UIComponent&) = delete;
        UIComponent(UIComponent&&) = delete;
        UIComponent& operator=(UIComponent&&) = delete;

        [[nodiscard]] virtual Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);

        bool isVisible() const noexcept { return m_isVisible; }
        void setVisible(bool visible) { m_isVisible = visible; }

        UIAnchor getAnchor() const noexcept { return m_anchor; }
        void setAnchor(UIAnchor anchor) { m_anchor = anchor; }

        Math::Vector2 getScreenPosition() const noexcept { return m_screenPosition; }
        void setScreenPosition(const Math::Vector2& pos) { m_screenPosition = pos; }

        UIRenderMode getRenderMode() const noexcept { return m_renderMode; }
        void setRenderMode(UIRenderMode mode) { m_renderMode = mode; }

    protected:
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;
        bool m_isVisible = true;
        UIAnchor m_anchor = UIAnchor::TopLeft;
        Math::Vector2 m_screenPosition = Math::Vector2::zero();
        UIRenderMode m_renderMode = UIRenderMode::WorldSpace;
    };

    // ======================================
    // UIText - UIComponent 継承
    // ======================================
    class UIText : public UIComponent
    {
    public:
        UIText() : UIComponent()
        {
            m_isVisible = true;
        }

        ~UIText() override = default;

        void setName(const std::string& name) { m_name = name; }
        const std::string& getName() const { return m_name; }

        void setText(const std::string& text)
        {
            if (m_text != text)
            {
                m_text = text;
                m_dirty = true;
            }
        }
        const std::string& getText() const { return m_text; }

        void setPosition(const Math::Vector3& position)
        {
            if (m_position != position)
            {
                m_position = position;
                m_dirty = true;

                syncToGameObjectTransform();
            }
        }
        const Math::Vector3& getPosition() const { return m_position; }

        void setRotation(const Math::Vector3& rotation)
        {
            if (m_rotation != rotation)
            {
                m_rotation = rotation;
                m_dirty = true;

                syncToGameObjectTransform();
            }
        }
        const Math::Vector3& getRotation() const { return m_rotation; }

        void setScale(const Math::Vector3& scale)
        {
            if (m_scale != scale)
            {
                m_scale = scale;
                m_dirty = true;

                syncToGameObjectTransform();
            }
        }
        const Math::Vector3& getScale() const { return m_scale; }

        void setFontSize(float size)
        {
            if (m_fontSize != size)
            {
                m_fontSize = size;
                m_dirty = true;
            }
        }
        float getFontSize() const { return m_fontSize; }

        void setColor(const Math::Vector3& color) { m_color = color; }
        const Math::Vector3& getColor() const { return m_color; }

        void setAlpha(float alpha) { m_alpha = alpha; }
        float getAlpha() const { return m_alpha; }

        Math::Matrix4 getWorldMatrix() const
        {
            if (m_dirty)
            {
                updateWorldMatrix();
                m_dirty = false;
            }
            return m_cachedWorldMatrix;
        }

        bool isDirty() const { return m_dirty; }
        void markDirty() { m_dirty = true; }

        void setPositionXYZ(float x, float y, float z)
        {
            setPosition(Math::Vector3(x, y, z));
        }

        void setRotationXYZ(float x, float y, float z)
        {
            setRotation(Math::Vector3(x, y, z));
        }

        void setScaleXYZ(float x, float y, float z)
        {
            setScale(Math::Vector3(x, y, z));
        }

        void setColorRGB(float r, float g, float b)
        {
            setColor(Math::Vector3(r, g, b));
        }

        void syncFromGameObjectTransform()
        {
            if (m_syncInProgress) return;

            auto* gameObject = getGameObject();
            if (!gameObject) return;

            auto* transform = gameObject->getTransform();
            if (!transform) return;

            m_syncInProgress = true;
            m_position = transform->getPosition();
            m_rotation = transform->getRotation();
            m_scale = transform->getScale();
            m_dirty = true;
            m_syncInProgress = false;
        }

    private:
        std::string m_name = "UIText";
        std::string m_text = "New Text";

        // 3D Transform
        Math::Vector3 m_position = Math::Vector3(0.0f, 0.0f, 0.0f);
        Math::Vector3 m_rotation = Math::Vector3(0.0f, 0.0f, 0.0f);
        Math::Vector3 m_scale = Math::Vector3(1.0f, 1.0f, 1.0f);

        float m_fontSize = 32.0f;
        Math::Vector3 m_color = Math::Vector3(1.0f, 1.0f, 1.0f);
        float m_alpha = 1.0f;

        mutable Math::Matrix4 m_cachedWorldMatrix = Math::Matrix4::identity();
        mutable bool m_dirty = true;

        bool m_syncInProgress = false;

        void syncToGameObjectTransform()
        {
            if (m_syncInProgress) return;

            auto* gameObject = getGameObject();
            if (!gameObject) return;

            auto* transform = gameObject->getTransform();
            if (!transform) return;

            m_syncInProgress = true;
            transform->setPosition(m_position);
            transform->setRotation(m_rotation);
            transform->setScale(m_scale);
            m_syncInProgress = false;
        }


        void updateWorldMatrix() const
        {
            Math::Matrix4 translation = Math::Matrix4::translation(m_position);
            Math::Matrix4 rotationX = Math::Matrix4::rotationX(Math::radians(m_rotation.x));
            Math::Matrix4 rotationY = Math::Matrix4::rotationY(Math::radians(m_rotation.y));
            Math::Matrix4 rotationZ = Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
            Math::Matrix4 scale = Math::Matrix4::scaling(m_scale);

            m_cachedWorldMatrix = translation * rotationX * rotationY * rotationZ * scale;
        }
    };
}

