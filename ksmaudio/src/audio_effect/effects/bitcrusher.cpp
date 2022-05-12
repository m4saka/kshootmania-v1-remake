#include "ksmaudio/audio_effect/bitcrusher.hpp"
#include <array>
#include <cmath>

namespace ksmaudio
{
	class BitcrusherDSP
	{
	private:
		const float m_sampleRateScale;

		const std::size_t m_numChannels;

		const bool m_isUnsupported;

		std::array<float, 2> m_holdSample = { 0.0f, 0.0f };

		float m_holdSampleCount = 0.0f;

	public:
		BitcrusherDSP(std::size_t sampleRate, std::size_t numChannels);

		void process(float* pData, std::size_t dataSize, bool bypass, const BitcrusherParams& params);
	};

	BitcrusherDSP::BitcrusherDSP(std::size_t sampleRate, std::size_t numChannels)
		: m_sampleRateScale(static_cast<float>(sampleRate) / 44100.0f)
		, m_numChannels(numChannels)
		, m_isUnsupported(m_numChannels == 0U || m_numChannels >= 3U) // Supports stereo and mono only
	{
	}

	void BitcrusherDSP::process(float* pData, std::size_t dataSize, bool bypass, const BitcrusherParams& params)
	{
		if (bypass || m_isUnsupported)
		{
			return;
		}

		const std::size_t numFrames = dataSize / m_numChannels;
		const float reduction = params.reduction * m_sampleRateScale;
		float* pCursor = pData;
		for (std::size_t i = 0; i < numFrames; ++i)
		{
			const bool updateHoldSample = (m_holdSampleCount > reduction);
			if (updateHoldSample)
			{
				m_holdSampleCount = std::fmod(m_holdSampleCount, reduction);
			}
			for (std::size_t channel = 0; channel < m_numChannels; ++channel)
			{
				if (updateHoldSample)
				{
					m_holdSample[channel] = *pCursor;
				}
				else
				{
					*pCursor = m_holdSample[channel];
				}
				++pCursor;
			}
			m_holdSampleCount += 1.0f;
		}
	}

	Bitcrusher::Bitcrusher(std::size_t sampleRate, std::size_t numChannels)
		: m_dsp(std::make_unique<BitcrusherDSP>(sampleRate, numChannels))
	{
	}

	void Bitcrusher::process(float* pData, std::size_t dataSize)
	{
		m_dsp->process(pData, dataSize, m_bypass, m_params);
	}

}