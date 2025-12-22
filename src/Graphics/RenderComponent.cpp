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


	Utils::VoidResult RenderComponent::initialize(Device* device, ShaderManager* shaderManager)
	{
		if (m_initialized)
		{
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null in RenderComponent::initialize");

		m_device = device;
		m_shaderManager = shaderManager;

		Utils::log_info("RenderComponent::initialize - Device and ShaderManager assigned successfully");

		// ShaderManagerのエラー返し
		if (!m_shaderManager) {
			Utils::log_warning("ShaderManager became null after assignment");
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null after assignment"));
		}

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

	void RenderComponent::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!m_visible || !m_initialized || !getGameObject())
		{
			return;
		}

		// Transform初期化
		auto* transform = getGameObject()->getTransform();
		if (!transform)
		{
			return;
		}

		// マテリアルがセットされていない場合の処理
		if (!m_material && m_materialManager)
		{
			m_material = m_materialManager->getDefaultMaterial();
		}
		/*
		//一時的に無効化
		if (m_material)
		{
			auto props = m_material->getProperties();


			props.albedo = m_color;
			m_material->setProperties(props);

	
			auto updateResult = m_material->updateConstantBuffer();
			if (!updateResult)
			{
				Utils::log_warning(std::format("Failed to update material constant buffer: {}",
					updateResult.error().message));
			}
		}
		*/

		//レンダータイプに分かれての処理
		switch (m_renderableType)
		{
		case RenderableType::Triangle:
			if (m_triangleRenderer && m_triangleRenderer->isValid())
			{
				m_triangleRenderer->setPosition(transform->getPosition());
				m_triangleRenderer->setRotation(transform->getRotation());
				m_triangleRenderer->setScale(transform->getScale());
				m_triangleRenderer->setMaterial(m_material);
				m_triangleRenderer->render(commandList, camera, frameIndex);
			}
			break;

		case RenderableType::Cube:
			if (m_cubeRenderer && m_cubeRenderer->isValid())
			{
				m_cubeRenderer->setPosition(transform->getPosition());
				m_cubeRenderer->setRotation(transform->getRotation());
				m_cubeRenderer->setScale(transform->getScale());
				m_cubeRenderer->setMaterial(m_material);
				m_cubeRenderer->render(commandList, camera, frameIndex);
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
		case RenderableType::Triangle:
			return m_triangleRenderer && m_triangleRenderer->isValid();
		case RenderableType::Cube:
			return m_cubeRenderer && m_cubeRenderer->isValid();
		default:
			return false;
		}
	}

	Utils::VoidResult RenderComponent::initializeRenderer()
	{
		m_triangleRenderer.reset();
		m_cubeRenderer.reset();

		// ShaderManagerエラー返し
		if (!m_shaderManager) {
			Utils::log_warning("ShaderManager is null in RenderComponent::initializeRenderer");
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null in RenderComponent"));
		}

		switch (m_renderableType)
		{
		case RenderableType::Triangle:
		{
			m_triangleRenderer.reset(new TriangleRenderer());
			auto result = m_triangleRenderer->initialize(m_device, m_shaderManager);
			if (!result) {
				m_triangleRenderer.reset();
				return result;
			}
			if (m_materialManager) {
				m_triangleRenderer->setMaterialManager(m_materialManager);
			}
		}
		break;

		case RenderableType::Cube:
		{
			m_cubeRenderer.reset(new CubeRenderer());
			auto result = m_cubeRenderer->initialize(m_device, m_shaderManager);
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
