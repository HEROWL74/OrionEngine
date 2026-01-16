// src/engine/UI/UIComponent.cpp
#include "UIComponent.hpp"

namespace Engine::EngineUI
{
	// ====================================
	// UIComponent Base Class
	// ====================================
	Utils::VoidResult UIComponent::initialize(Graphics::Device* device, Graphics::ShaderManager* shaderManager)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");

		m_device = device;
		m_shaderManager = shaderManager;

		return{};
	}

	// ====================================
	// UIText
	// ====================================
	UIText::UIText(const std::string& text)
		:m_text(text)
	{
	}

	Utils::VoidResult UIText::initialize(Graphics::Device* device, Graphics::ShaderManager* shaderManager)
	{
		// 親クラスの初期化を呼ぶ
		auto result = UIComponent::initialize(device, shaderManager);
		if (!result)
		{
			return result;
		}

		// TODO: ここでフォントテクスチャなどの読み込みを行う
		Utils::log_info(std::format("UI Text iniitlaized: '{}'", m_text));

		return {};
	}
}