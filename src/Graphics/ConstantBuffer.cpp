// src/Graphics/ConstantBuffer.cpp
#include "ConstantBuffer.hpp"

namespace Engine::Graphics
{
    Utils::VoidResult ConstantBufferManager::initialize(Device* device, UINT frameCount)
    {
        auto cameraResult = m_cameraConstants.initialize(device, frameCount);
        if (!cameraResult)
        {
            return cameraResult;
        }

        auto objectResult = m_objectConstants.initialize(device, frameCount);
        if (!objectResult)
        {
            return objectResult;
        }

        return {};
    }

    void ConstantBufferManager::updateCameraConstants(UINT frameIndex, const CameraConstants& constants)
    {
        m_cameraConstants.updateData(frameIndex, constants);
    }

    void ConstantBufferManager::updateObjectConstants(UINT frameIndex, const ObjectConstants& constants)
    {
        m_objectConstants.updateData(frameIndex, constants);
    }

    D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferManager::getCameraConstantsGPUAddress(UINT frameIndex) const
    {
        return m_cameraConstants.getGPUAddress(frameIndex);
    }

    D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferManager::getObjectConstantsGPUAddress(UINT frameIndex) const
    {
        return m_objectConstants.getGPUAddress(frameIndex);
    }

    bool ConstantBufferManager::isValid() const
    {
        return m_cameraConstants.isValid() && m_objectConstants.isValid();
    }
}
