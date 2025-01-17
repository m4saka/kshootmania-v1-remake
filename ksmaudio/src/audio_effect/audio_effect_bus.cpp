﻿#include "ksmaudio/audio_effect/audio_effect_bus.hpp"

namespace ksmaudio::AudioEffect
{
	AudioEffectBus::AudioEffectBus(bool isLaser, Stream* pStream)
		: m_isLaser(isLaser)
		, m_pStream(pStream)
	{
	}

	AudioEffectBus::~AudioEffectBus()
    {
		for (const auto& hDSP : m_hDSPs)
		{
			m_pStream->removeAudioEffect(hDSP);
		}
    }

	void AudioEffectBus::updateByFX(const AudioEffect::Status& status, const ActiveAudioEffectDict& activeAudioEffectDict)
	{
		assert(!m_isLaser);

		// Update override params
		{
			// "Active -> Inactive"
			for (const std::size_t& idx : m_activeAudioEffectIdxs)
			{
				if (!activeAudioEffectDict.contains(idx))
				{
					assert(m_paramControllers.size() > idx);
					m_paramControllers[idx].clearOverrideParams();
				}
			}

			// "Inactive -> Active" or "Active -> Active"
			m_activeAudioEffectIdxs.clear();
			for (const auto& [audioEffectIdx, activeAudioEffectInvocation] : activeAudioEffectDict)
			{
				assert(m_paramControllers.size() > audioEffectIdx);
				m_paramControllers[audioEffectIdx].setOverrideParams(*activeAudioEffectInvocation.pOverrideParams);

				m_activeAudioEffectIdxs.insert(audioEffectIdx);
			}
		}

		// Update all audio effects
		for (std::size_t i = 0U; i < m_audioEffects.size(); ++i)
		{
			if (m_paramControllers[i].update(status.sec))
			{
				// When the parameter value set is updated, pass it to the audio effect
				for (const auto& [paramID, valueSet] : m_paramControllers[i].currentParams())
				{
					m_audioEffects[i]->setParamValueSet(paramID, valueSet);
				}
			}

			const bool isOn = activeAudioEffectDict.contains(i);
			m_audioEffects[i]->updateStatusByFX(status, isOn ? std::make_optional(activeAudioEffectDict.at(i).laneIdx) : std::nullopt);
		}
	}

	void AudioEffectBus::updateByLaser(const AudioEffect::Status& status, std::optional<std::size_t> activeAudioEffectIdx)
	{
		assert(m_isLaser);

		// Update override params
		{
			// "Active -> Inactive"
			for (const std::size_t& idx : m_activeAudioEffectIdxs)
			{
				if (idx != activeAudioEffectIdx)
				{
					assert(m_paramControllers.size() > idx);
					m_paramControllers[idx].clearOverrideParams();
				}
			}

			// "Inactive -> Active" or "Active -> Active"
			m_activeAudioEffectIdxs.clear();
			if (activeAudioEffectIdx.has_value())
			{
				const std::size_t activeAudioEffectIdxValue = activeAudioEffectIdx.value();
				m_activeAudioEffectIdxs.insert(activeAudioEffectIdxValue);
				assert(m_paramControllers.size() > activeAudioEffectIdxValue);
				m_paramControllers[activeAudioEffectIdxValue].clearOverrideParams();
			}
		}

		// Update all audio effects
		for (std::size_t i = 0U; i < m_audioEffects.size(); ++i)
		{
			if (m_paramControllers[i].update(status.sec))
			{
				// When the parameter value set is updated, pass it to the audio effect
				for (const auto& [paramID, valueSet] : m_paramControllers[i].currentParams())
				{
					m_audioEffects[i]->setParamValueSet(paramID, valueSet);
				}
			}

			const bool isOn = activeAudioEffectIdx == i;
			m_audioEffects[i]->updateStatusByLaser(status, isOn);
		}
	}

	void AudioEffectBus::setBypass(bool bypass)
	{
		for (const auto& audioEffect : m_audioEffects)
		{
			audioEffect->setBypass(bypass);
		}
	}

	bool AudioEffectBus::audioEffectContainsName(const std::string& name) const
	{
		return m_nameIdxDict.contains(name);
	}

	std::size_t AudioEffectBus::audioEffectNameToIdx(const std::string& name) const
	{
		if (!audioEffectContainsName(name))
		{
			assert(false && "Audio effect name not found");
			return static_cast<std::size_t>(-1);
		}
		return m_nameIdxDict.at(name);
	}
}
