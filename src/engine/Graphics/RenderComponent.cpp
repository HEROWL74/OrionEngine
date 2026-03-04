// src/Graphics/RenderComponent.cpp
#include "RenderComponent.hpp"
#include <algorithm>

namespace Engine::Graphics
{
	//==========================================================================================
	//RenderComponent
	//==========================================================================================
	RenderComponent::RenderComponent(RenderableType type)
		:m_renderableType(type)
	{
	}

	RenderComponent::~RenderComponent()
	{
		// デストラクタでGPU同期を行う
		if (m_device && m_initialized)
		{
			m_device->waitForGpu();
		}

		// レンダラーを明示的に破棄
		m_cubeRenderer.reset();
		m_material.reset();
	}

	Utils::VoidResult RenderComponent::initialize(Device* device, ShaderManager* shaderManager)
	{
		if (m_initialized) return {};

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown,
			"ShaderManager is null in RenderComponent::initialize");

		m_device = device;
		m_shaderManager = shaderManager;

		Utils::log_info("RenderComponent::initialize - Device and ShaderManager assigned successfully");

		auto result = initializeRenderer();
		if (!result)
		{
			Utils::log_error(result.error());
			return result;
		}

		m_initialized = true;
		Utils::log_info("RenderComponent initialized successfully");
		return {};
	}

	void RenderComponent::render(const Utils::RenderContext& context)
	{
		if (!m_visible || !m_initialized || !getGameObject())
		{
			return;
		}

		auto* transform = getGameObject()->getTransform();
		if (!transform)
		{
			return;
		}

		if (!m_material && m_materialManager)
		{
			m_material = m_materialManager->getDefaultMaterial();
		}

		// レンダラーに RenderContext を渡す
		switch (m_renderableType)
		{
		case RenderableType::Cube:
			if (m_cubeRenderer && m_cubeRenderer->isValid())
			{
				m_cubeRenderer->setPosition(transform->getPosition());
				m_cubeRenderer->setRotation(transform->getRotation());
				m_cubeRenderer->setScale(transform->getScale());
				m_cubeRenderer->setMaterial(m_material);
				m_cubeRenderer->render(context);
			}
			break;
		}
	}

	void RenderComponent::setRenderableType(RenderableType type)
	{
		if (m_renderableType != type)
		{
			m_renderableType = type;
			if (m_initialized)
			{
				initializeRenderer();
			}
		}
	}

	bool RenderComponent::isValid() const
	{
		if (!m_initialized || !m_device)
		{
			return false;
		}

		switch (m_renderableType)
		{
		case RenderableType::Cube:
			return m_cubeRenderer && m_cubeRenderer->isValid();
		default:
			return false;
		}
	}

	Utils::VoidResult RenderComponent::initializeRenderer()
	{
		m_cubeRenderer.reset();

		// ShaderManagerエラー返し
		if (!m_shaderManager) {
			Utils::log_warning("ShaderManager is null in RenderComponent::initializeRenderer");
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null in RenderComponent"));
		}

		switch (m_renderableType)
		{
		case RenderableType::Cube:
		{
			m_cubeRenderer.reset(new CubeRenderer());
			auto result = m_cubeRenderer->initialize(m_device);
			if (!result) {
				m_cubeRenderer.reset();
				return result;
			}
			if (m_materialManager) {
				m_cubeRenderer->setMaterialManager(m_materialManager);
			}
		}
		break;

		default:
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unknown renderable type"));
		}

		return {};
	}
}