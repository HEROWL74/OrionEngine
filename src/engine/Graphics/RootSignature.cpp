#include "RootSignature.hpp"

namespace Engine::Graphics
{
	Utils::VoidResult RootSignature::initialize(Device* device, ShaderManager* shaderManager)
	{
		m_device = device;
		m_shaderManager = shaderManager;

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::DeviceCreation, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::DeviceCreation, "ShaderManager is null");

		return{};
	}
}

