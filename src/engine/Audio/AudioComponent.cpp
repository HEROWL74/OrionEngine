#define MINIAUDIO_IMPLEMENTATION
#include "AudioComponent.hpp"
#include <format>
#include <thread>
#include <chrono>

namespace Engine::Audio
{
	AudioComponent::~AudioComponent()
	{
		cleanup();
	}

	Utils::VoidResult AudioComponent::initialize()
	{
		if (m_initialized)
		{
			return {};
		}

		m_initialized = true;
		Utils::log_info("AudioComponent initialized");
		return {};
	}

	Utils::VoidResult AudioComponent::loadAudio(const std::string& filePath)
	{
		// 既存のオーディオをクリーンアップ
		cleanup();

		// デコーダーを初期化
		ma_result result = ma_decoder_init_file(filePath.c_str(), nullptr, &m_decoder);
		if (result != MA_SUCCESS)
		{
			return std::unexpected(Utils::make_error(
				Utils::ErrorType::FileI0,
				std::format("Failed to load audio file: {}", filePath)
			));
		}

		// デバイス設定
		m_deviceConfig = ma_device_config_init(ma_device_type_playback);
		m_deviceConfig.playback.format = m_decoder.outputFormat;
		m_deviceConfig.playback.channels = m_decoder.outputChannels;
		m_deviceConfig.sampleRate = m_decoder.outputSampleRate;
		m_deviceConfig.dataCallback = dataCallback;
		m_deviceConfig.pUserData = this;

		// デバイスを初期化
		result = ma_device_init(nullptr, &m_deviceConfig, &m_device);
		if (result != MA_SUCCESS)
		{
			ma_decoder_uninit(&m_decoder);
			return std::unexpected(Utils::make_error(
				Utils::ErrorType::Unknown,
				"Failed to initialize audio device"
			));
		}

		m_filepath = filePath;
		m_audioLoaded = true;

		Utils::log_info(std::format("Audio loaded: {}", filePath));
		return {};
	}

	void AudioComponent::play()
	{
		if (!m_audioLoaded)
		{
			Utils::log_warning("No audio loaded");
			return;
		}

		if (isPlaying())
		{
			ma_decoder_seek_to_pcm_frame(&m_decoder, 0);
		}

		m_paused = false;
		ma_device_start(&m_device);
		Utils::log_info("Audio playing");
	}

	void AudioComponent::stop()
	{
		if (!m_audioLoaded)
		{
			return;
		}

		// デバイスが開始されているかチェックしてから停止
		if (ma_device_is_started(&m_device) == MA_TRUE)
		{
			ma_device_stop(&m_device);
		}
		ma_decoder_seek_to_pcm_frame(&m_decoder, 0);
		m_paused = false;
		Utils::log_info("Audio stopped");
	}

	void AudioComponent::pause()
	{
		if (!m_audioLoaded || !isPlaying())
		{
			return;
		}

		ma_device_stop(&m_device);
		m_paused = true;
		Utils::log_info("Audio paused");
	}

	void AudioComponent::resume()
	{
		if (!m_audioLoaded || !m_paused)
		{
			return;
		}

		ma_device_start(&m_device);
		m_paused = false;
		Utils::log_info("Audio resumed");
	}

	void AudioComponent::setVolume(float volume)
	{
		m_volume = Math::clamp(volume, 0.0f, 1.0f);

		if (m_audioLoaded)
		{
			ma_device_set_master_volume(&m_device, m_volume);
		}
	}

	bool AudioComponent::isPlaying()const
	{
		if (!m_audioLoaded)
		{
			return false;
		}

		return ma_device_is_started(&m_device) == MA_TRUE;
	}

	void AudioComponent::start()
	{
		if (!m_initialized)
		{
			initialize();
		}

		// ファイルパスが設定されている場合は自動的にロード
		if (!m_filepath.empty() && !m_audioLoaded)
		{
			auto result = loadAudio(m_filepath);
			if (!result.has_value())
			{
				Utils::log_warning(std::format("Failed to auto-load audio: {}", m_filepath));
			}
		}
	}

	void AudioComponent::update(float deltaTime)
	{
		// コールバックからの停止要求を処理
		if (m_shouldStop)
		{
			m_shouldStop = false;
			stop();
			return;
		}

		if (m_loop && m_audioLoaded && !isPlaying() && !m_paused)
		{
			play();
		}
	}
	void AudioComponent::dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		AudioComponent* audioComponent = static_cast<AudioComponent*>(pDevice->pUserData);
		if (!audioComponent)
		{
			return;
		}

		ma_decoder* pDecoder = &audioComponent->m_decoder;
		if (!pDecoder)
		{
			return;
		}

		ma_uint64 framesRead = 0;
		ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);

		// ループ処理
		if (framesRead < frameCount)
		{
			if (audioComponent->m_loop)
			{
				// 最初に戻る
				ma_decoder_seek_to_pcm_frame(pDecoder, 0);

				// 残りのフレームを読み込む
				ma_uint64 remainingFrames = frameCount - framesRead;
				void* pOutputOffset = (ma_uint8*)pOutput + (framesRead * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));

				ma_decoder_read_pcm_frames(pDecoder, pOutputOffset, remainingFrames, nullptr);
			}
			else
			{
				audioComponent->m_shouldStop = true;
			}
		}
		(void)pInput;
	}

	void AudioComponent::cleanup()
	{
		if (m_audioLoaded)
		{
			if (ma_device_is_started(&m_device) == MA_TRUE)
			{
				ma_device_stop(&m_device);

				// デバイスが完全に停止するまで少し待つ
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}

			ma_device_uninit(&m_device);
			ma_decoder_uninit(&m_decoder);

			m_audioLoaded = false;
			m_paused = false;
		}
	}
}