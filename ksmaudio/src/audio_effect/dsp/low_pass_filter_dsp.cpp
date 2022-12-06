#include "ksmaudio/audio_effect/dsp/low_pass_filter_dsp.hpp"

namespace ksmaudio::AudioEffect
{
	LowPassFilterDSP::LowPassFilterDSP(const DSPCommonInfo& info)
		: m_info(info)
	{
	}

	void LowPassFilterDSP::process(float* pData, std::size_t dataSize, bool bypass, const LowPassFilterDSPParams& params)
	{
		if (m_info.isUnsupported)
		{
			return;
		}

		const float fSampleRate = static_cast<float>(m_info.sampleRate);
		for (std::size_t ch = 0U; ch < m_info.numChannels; ++ch)
		{
			m_lowPassFilters[ch].setLowPassFilter(params.freq, params.q, fSampleRate);
		}

		const bool isBypassed = bypass || params.mix == 0.0f; // 切り替え時のノイズ回避のためにbypass状態でも処理自体はする
		const bool isSkipped = std::abs(params.v - m_prevV) > 0.5f || params.freq > 14800.0f; // ノイズ回避のため、直角等で値が飛んだ瞬間や、高周波数に対しては適用しない

		assert(dataSize % m_info.numChannels == 0);
		const std::size_t frameSize = dataSize / m_info.numChannels;
		for (std::size_t i = 0U; i < frameSize; ++i)
		{
			for (std::size_t ch = 0U; ch < m_info.numChannels; ++ch)
			{
				const float wet = m_lowPassFilters[ch].process(*pData);
				if (!isBypassed && !isSkipped)
				{
					*pData = std::lerp(*pData, wet, params.mix);
				}
				++pData;
			}
		}

		m_prevV = params.v;
	}
}
