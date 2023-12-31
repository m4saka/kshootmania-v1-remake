﻿#include "bgm.hpp"

namespace MusicGame::Audio
{
	namespace
	{
		constexpr double kBlendTimeSec = 5.0;
		constexpr double kManualUpdateIntervalSec = 0.005;
	}

	void BGM::emplaceAudioEffectImpl(bool isFX, const std::string& name, const kson::AudioEffectDef& def, const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges, const std::set<float>& updateTriggerTiming)
	{
		if (m_stream.numChannels() == 0)
		{
			// ロード失敗時は音声エフェクトを追加しない
			return;
		}

		const auto pAudioEffectBus = isFX ? m_pAudioEffectBusFX : m_pAudioEffectBusLaser;
		switch (def.type)
		{
		case kson::AudioEffectType::Retrigger:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Retrigger>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Gate:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Gate>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Flanger:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Flanger>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Bitcrusher:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Bitcrusher>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Wobble:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Wobble>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Tapestop:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Tapestop>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Echo:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Echo>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::Sidechain:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::Sidechain>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::PeakingFilter:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::PeakingFilter>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::HighPassFilter:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::HighPassFilter>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		case kson::AudioEffectType::LowPassFilter:
			pAudioEffectBus->emplaceAudioEffect<ksmaudio::LowPassFilter>(name, def.v, paramChanges, updateTriggerTiming);
			break;

		default:
			assert(false && "Unknown audio effect type");
			break;
		}
	}

	BGM::BGM(FilePathView filePath, double volume, double offsetSec)
		: m_stream(filePath.narrow(), volume, true, true)
		, m_durationSec(m_stream.durationSec())
		, m_offsetSec(offsetSec)
		, m_pAudioEffectBusFX(m_stream.emplaceAudioEffectBusFX())
		, m_pAudioEffectBusLaser(m_stream.emplaceAudioEffectBusLaser())
		, m_stopwatch(StartImmediately::No)
		, m_manualUpdateStopwatch(StartImmediately::Yes)
	{
	}

	void BGM::update()
	{
		if (m_isPaused)
		{
			return;
		}

		if (m_isStreamStarted)
		{
			if (m_manualUpdateStopwatch.sF() >= kManualUpdateIntervalSec)
			{
				m_stream.updateManually();
				m_manualUpdateStopwatch.restart();
			}
			m_timeSec = m_stream.posSec() - m_offsetSec;

			if (m_timeSec + m_offsetSec < m_durationSec - kBlendTimeSec)
			{
				// ストップウォッチの時間を同期
				m_stopwatch.set(SecondsF{ m_timeSec });
			}
		}
		else
		{
			m_timeSec = m_stopwatch.sF();

			if (m_timeSec + m_offsetSec >= 0.0)
			{
				m_stream.seekPosSec(m_timeSec + m_offsetSec);
				m_stream.play();
				m_isStreamStarted = true;
			}
		}
	}

	void BGM::updateAudioEffectFX(bool bypass, const ksmaudio::AudioEffect::Status& status, const ksmaudio::AudioEffect::ActiveAudioEffectDict& activeAudioEffects)
	{
		m_pAudioEffectBusFX->setBypass(bypass);
		m_pAudioEffectBusFX->updateByFX(
			status,
			activeAudioEffects);
	}

	void BGM::updateAudioEffectLaser(bool bypass, const ksmaudio::AudioEffect::Status& status, const std::optional<std::size_t>& activeAudioEffectIdx)
	{
		m_pAudioEffectBusLaser->setBypass(bypass);
		m_pAudioEffectBusLaser->updateByLaser(
			status,
			activeAudioEffectIdx);
	}

	void BGM::play()
	{
		m_stopwatch.start();
		m_isStreamStarted = false;
		m_isPaused = false;
	}

	void BGM::pause()
	{
		if (m_isStreamStarted)
		{
			m_stream.pause();
		}
		m_stopwatch.pause();
		m_isPaused = true;
	}

	void BGM::seekPosSec(double posSec)
	{
		if (posSec < 0.0)
		{
			m_stream.stop();
		}
		else
		{
			m_stream.seekPosSec(posSec);
		}
		m_timeSec = posSec;
		m_stopwatch.set(SecondsF{ posSec });
	}

	double BGM::posSec() const
	{
		// 開始・終了地点でノーツの動きが一瞬止まるのを防ぐため、最初と最後は再生位置に対してストップウォッチの時間を混ぜる
		// TODO: うまく効いていないようなので見直す
		if (m_isStreamStarted)
		{
			const double timeSecWithOffset = m_timeSec + m_offsetSec;
			if (0.0 <= timeSecWithOffset && timeSecWithOffset < kBlendTimeSec)
			{
				const double lerpRate = timeSecWithOffset / kBlendTimeSec;
				return Math::Lerp(m_stopwatch.sF(), m_timeSec, lerpRate);
			}
			else if (m_durationSec - kBlendTimeSec <= timeSecWithOffset && timeSecWithOffset < m_durationSec)
			{
				const double lerpRate = (timeSecWithOffset - (m_durationSec - kBlendTimeSec)) / kBlendTimeSec;
				return Math::Lerp(m_timeSec, m_stopwatch.sF(), lerpRate);
			}
			else if (m_durationSec - m_offsetSec <= m_stopwatch.sF())
			{
				return m_stopwatch.sF();
			}
		}

		return m_timeSec;
	}

	double BGM::durationSec() const
	{
		return m_durationSec;
	}

	double BGM::latencySec() const
	{
		return m_stream.latencySec();
	}

	void BGM::emplaceAudioEffectFX(const std::string& name, const kson::AudioEffectDef& def, const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges, const std::set<float>& updateTriggerTiming)
	{
		emplaceAudioEffectImpl(true, name, def, paramChanges, updateTriggerTiming);
	}

	void BGM::emplaceAudioEffectLaser(const std::string& name, const kson::AudioEffectDef& def, const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges, const std::set<float>& updateTriggerTiming)
	{
		emplaceAudioEffectImpl(false, name, def, paramChanges, updateTriggerTiming);
	}

	const ksmaudio::AudioEffect::AudioEffectBus& BGM::audioEffectBusFX() const
	{
		return *m_pAudioEffectBusFX;
	}

	const ksmaudio::AudioEffect::AudioEffectBus& BGM::audioEffectBusLaser() const
	{
		return *m_pAudioEffectBusLaser;
	}

	void BGM::setFadeOut(double durationSec)
	{
		m_stream.setFadeOut(durationSec);
	}
}
