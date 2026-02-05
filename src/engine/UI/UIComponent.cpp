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
}