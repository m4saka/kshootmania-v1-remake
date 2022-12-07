#pragma once
#include <optional>
#include "ksmaudio/audio_effect/audio_effect.hpp"
#include "ksmaudio/audio_effect/params/peaking_filter_params.hpp"
#include "ksmaudio/audio_effect/detail/biquad_filter.hpp"
#include "ksmaudio/audio_effect/detail/linear_easing.hpp"

namespace ksmaudio::AudioEffect
{
	class PeakingFilterDSP
	{
	private:
		const DSPCommonInfo m_info;
		std::array<detail::BiquadFilter<float>, 2> m_peakingFilters;
		float m_prevV = 0.0f;
		detail::LinearEasing<float> m_freqEasing; // TODO: freqではなくvに適用

	public:
		explicit PeakingFilterDSP(const DSPCommonInfo& info);

		void process(float* pData, std::size_t dataSize, bool bypass, const PeakingFilterDSPParams& params);
	};
}