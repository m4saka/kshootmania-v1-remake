#include "ksmaudio/audio_effect/dsp/peaking_filter_dsp.hpp"
#include <algorithm>

namespace ksmaudio::AudioEffect
{
	namespace
	{
		constexpr float kFreqSpeed = 5.0f;
	}

	PeakingFilterDSP::PeakingFilterDSP(const DSPCommonInfo& info)
		: m_info(info)
		, m_freqEasing(kFreqSpeed)
	{
	}

	void PeakingFilterDSP::process(float* pData, std::size_t dataSize, bool bypass, const PeakingFilterDSPParams& params)
	{
		if (m_info.isUnsupported)
		{
			return;
		}


		const float fSampleRate = static_cast<float>(m_info.sampleRate);

		const bool isBypassed = bypass || params.mix == 0.0f; // 切り替え時のノイズ回避のためにbypass状態でも処理自体はする
		const bool isSkipped = std::abs(params.v - m_prevV) > 0.2f || params.freq < 60.0f; // ノイズ回避のため、直角等で値が飛んだ瞬間や、低周波数に対しては適用しない
		// TODO: isSkippedは直角から一定時間かどうかで判定するようにする

		assert(dataSize % m_info.numChannels == 0);
		const std::size_t frameSize = dataSize / m_info.numChannels;
		for (std::size_t i = 0U; i < frameSize; ++i)
		{
			const bool freqUpdated = m_freqEasing.update(params.freq);

			for (std::size_t ch = 0U; ch < m_info.numChannels; ++ch)
			{
				if (freqUpdated)
				{
					m_peakingFilters[ch].setPeakingFilter(m_freqEasing.value(), params.bandwidth, params.gain, fSampleRate);
				}

				const float wet = m_peakingFilters[ch].process(*pData);
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
