// src/engine/UI/UIComponent.hpp
#include "../Core/GameObject.hpp"
#include "../Graphics/Device.hpp"
#include "../Graphics/ShaderManager.hpp"

namespace Engine::EngineUI
{
	// ======================================
	// UIComponentベースクラス
	// ======================================
	class UIComponent : public Core::Component
	{
	public:
		explicit UIComponent() {}
		virtual ~UIComponent() = default;

		[[nodiscard]] virtual Utils::VoidResult initialize(Graphics::Device* device, Graphics::ShaderManager* shaderManager);

		// 全てのUIコンポネント共通のプロパティ
		bool isVisible()const noexcept { return m_isVisible; }
		void setVisible(bool visible) { m_isVisible = visible; }
	private:
		Graphics::Device* m_device = nullptr;
		Graphics::ShaderManager* m_shaderManager = nullptr;
		bool m_isVisible = true;
	};

	// ======================================
    // UITextベースクラス
    // ======================================

	class UIText : public UIComponent
	{
	public:
		explicit UIText(const std::string& text = "New Text");
		~UIText() = default;

		[[nodiscard]] 
		Utils::VoidResult initialize(Graphics::Device* device, Graphics::ShaderManager* shaderManager) override;

		// テキスト関連
		const std::string& getText() const noexcept { return m_text; }
		void setText(const std::string& text) { m_text = text; }

		// フォントサイズ
		float  getFontSize() const noexcept { return m_fontSize; }
		void setFontSize(float size) { m_fontSize = size; }

		// カラー(RGB 0-1)
		Math::Vector3 getColor() const noexcept { return m_color; }
		void setColor(const Math::Vector3& color) { m_color = color; }

		// 位置（スクリーン座標系: 0-1）
		Math::Vector2 getPosition()const noexcept { return m_position; }
		void setPosition(const Math::Vector2& position) { m_position = position; }
	private:
		std::string m_text = "New Text";
		float m_fontSize = 16.0f;
		Math::Vector3 m_color = Math::Vector3(1.0f, 1.0f, 1.0f);
		Math::Vector2 m_position = Math::Vector2(0.5f, 0.5f);
	};
}