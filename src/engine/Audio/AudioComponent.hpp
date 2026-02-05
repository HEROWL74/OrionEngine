// src/engine/Audio/AudioComponent.hpp
#pragma once
#include <miniaudio.h>
#include <string>
#include "../Core/GameObject.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Audio
{
	class AudioComponent : public Core::Component
	{
	public:
		AudioComponent() = default;
		~AudioComponent();

		// MiniAudio lib initialize
		[[nodiscard]]
		Utils::VoidResult initialize();

		// Load wav 
		[[nodiscard]] Utils::VoidResult loadAudio(const std::string& filepath);

		// 再生制御関数軍
		void play();
		void stop();
		void pause();
		void resume();

		// ループ設定
		void setLoop(bool loop) { m_loop = loop; }
		bool isLoop()const { return m_loop; }

		// 音量設定
		void setVolume(float volume);
		float getVolume() const noexcept { return m_volume; }

		// 状態取得
		bool isPlaying()const;
		bool isPaused()const { return m_paused; }

		// ファイルパス取得・設定
		const std::string& getFilePath()const { return m_filepath; }
		void setFilePath(const std::string& filepath) { m_filepath = filepath; }

		// Component Overrides
		void start() override;
		void update(float deltaTime) override;
	private:
		std::string m_filepath;
		ma_decoder m_decoder{};
		ma_device_config m_deviceConfig{};
		ma_device m_device{};

		bool m_initialized = false;
		bool m_audioLoaded = false;
		bool m_loop = false;
		bool m_shouldStop = false;
		bool m_paused = false;
		float m_volume = 1.0f;

		// MiniAudio Callback
		static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

		void cleanup();
	};
}