// src/Graphics/SplashScreen.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "../Utils/Common.hpp"
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "Texture.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // Splash screen constants structure
    struct SplashConstants
    {
        float fadeAlpha{};      // Current fade alpha (0.0 to 1.0)
        float logoScale{};      // Logo scale factor
        float screenAspect{};   // Screen width / height
        float padding;        // Padding for 16-byte alignment
    };

    class SplashScreen
    {
    public:
        SplashScreen() = default;
        ~SplashScreen() = default;

        // Disable copy/move
        SplashScreen(const SplashScreen&) = delete;
        SplashScreen& operator=(const SplashScreen&) = delete;

        // Initialize splash screen
        [[nodiscard]] Utils::VoidResult initialize(
            Device* device,
            ShaderManager* shaderManager,
            TextureManager* textureManager
        );

        // Update splash screen animation (returns true if still active)
        bool update(float deltaTime);

        // Render splash screen
        void render(ID3D12GraphicsCommandList* commandList);

        // Check if splash screen is finished
        bool isFinished() const { return m_currentTime >= m_totalDuration; }

        // Get current progress (0.0 to 1.0)
        float getProgress() const { return m_currentTime / m_totalDuration; }

        // Update screen size (call when window resizes)
        void setScreenSize(float width, float height)
        {
            m_screenWidth = width;
            m_screenHeight = height;
        }

    private:
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;
        std::shared_ptr<Texture> m_logoTexture;

        // Pipeline resources
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12Resource> m_vertexBuffer;
        ComPtr<ID3D12Resource> m_constantBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};

        // Timing
        float m_currentTime = 0.0f;
        float m_fadeInDuration = 0.5f;    // Fade in: 0.5 seconds
        float m_displayDuration = 2.0f;   // Display: 2.0 seconds
        float m_fadeOutDuration = 0.5f;   // Fade out: 0.5 seconds
        float m_totalDuration = 3.0f;     // Total: 3.0 seconds

        // Logo settings
        float m_logoScale = 0.25f;        // Logo size (25% of screen)
        float m_screenWidth = 1280.0f;
        float m_screenHeight = 720.0f;

        // Initialization helpers
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();
        [[nodiscard]] Utils::VoidResult createConstantBuffer();

        // Calculate current fade alpha based on time
        float calculateFadeAlpha() const;

        // Update constant buffer
        void updateConstants();
    };
}

