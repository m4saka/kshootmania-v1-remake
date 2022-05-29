#pragma once
#include <array>
#include <memory>
#include "bass.h"
#include "audio_effect_param.hpp"

namespace ksmaudio::AudioEffect
{
	class IAudioEffect
	{
	public:
		virtual ~IAudioEffect() = default;

		virtual void process(float* pData, std::size_t dataSize) = 0;

		virtual void updateStatus(const Status& status, bool isOn) = 0;

		virtual bool setParamValueSet(const std::string& name, const std::string& valueSetStr) = 0;

		virtual bool setParamValueSet(const std::string& name, const ValueSet& valueSetStr) = 0;

		virtual Type paramTypeOf(const std::string& name) const = 0;
	};

	class IUpdateTrigger
	{
	public:
		virtual ~IUpdateTrigger() = default;

		virtual void setUpdateTriggerTiming(const std::set<float>& timing) = 0;
	};

	struct DSPCommonInfo
	{
		bool isUnsupported;

		std::size_t sampleRate;

		float sampleRateScale;

		std::size_t numChannels;

		constexpr DSPCommonInfo(std::size_t sampleRate, std::size_t numChannels)
			: isUnsupported(numChannels == 0U || numChannels >= 3U) // Supports stereo and mono only
			, sampleRate(sampleRate)
			, sampleRateScale(sampleRate / 44100.0f)
			, numChannels(numChannels)
		{
		}
	};

	template <typename Params, typename DSP, typename DSPParams>
	class BasicAudioEffect : public IAudioEffect
	{
	protected:
		bool m_bypass = false;
		Params m_params;
		DSPParams m_dspParams;
		DSP m_dsp;

	public:
		BasicAudioEffect(std::size_t sampleRate, std::size_t numChannels)
			: m_dsp(DSPCommonInfo{ sampleRate, numChannels })
			, m_dspParams(m_params.render(Status{}, false))
		{
		}

		virtual ~BasicAudioEffect() = default;

		virtual void process(float* pData, std::size_t dataSize) override
		{
			m_dsp.process(pData, dataSize, m_bypass, m_dspParams);
		}

		virtual void updateStatus(const Status& status, bool isOn) override
		{
			m_dspParams = m_params.render(status, isOn);
		}

		virtual bool setParamValueSet(const std::string& name, const std::string& valueSetStr) override
		{
			if (m_params.dict.contains(name))
			{
				Param& paramRef = *m_params.dict.at(name);
				bool success;
				paramRef.valueSet = StrToValueSet(paramRef.type, valueSetStr, &success);
				return success;
			}
			else
			{
				return false;
			}
		}

		virtual bool setParamValueSet(const std::string& name, const ValueSet& valueSet) override
		{
			if (m_params.dict.contains(name))
			{
				m_params.dict.at(name)->valueSet = valueSet;
				return true;
			}
			else
			{
				return false;
			}
		}

		virtual Type paramTypeOf(const std::string& name) const
		{
			if (m_params.dict.contains(name))
			{
				return m_params.dict.at(name)->type;
			}
			else
			{
				return Type::kUnspecified;
			}
		}
	};

	template <typename Params, typename DSP, typename DSPParams>
	class BasicAudioEffectWithTrigger : public BasicAudioEffect<Params, DSP, DSPParams>, public IUpdateTrigger
	{
	protected:
		using BasicAudioEffect<Params, DSP, DSPParams>::m_params;

	public:
		BasicAudioEffectWithTrigger(std::size_t sampleRate, std::size_t numChannels)
			: BasicAudioEffect<Params, DSP, DSPParams>(sampleRate, numChannels)
		{
		}

		virtual ~BasicAudioEffectWithTrigger() = default;

		virtual void setUpdateTriggerTiming(const std::set<float>& timing) override
		{
			m_params.setUpdateTriggerTiming(timing);
		}
	};
}
