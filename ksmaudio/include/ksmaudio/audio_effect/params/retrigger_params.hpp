#pragma once
#include "ksmaudio/audio_effect/audio_effect_param.hpp"

namespace ksmaudio::AudioEffect
{
	struct RetriggerDSPParams
	{
		float secUntilTrigger = -1.0f; // Note: Negative value will be just ignored
		bool updateTrigger = false;
		float waveLength = 0.0f;
		float rate = 0.7f;
		float mix = 1.0f;
	};

	struct RetriggerParams
	{
		Param updatePeriod = DefineParam(Type::kLength, "1/2");
		Param waveLength = DefineParam(Type::kLength, "0");
		Param rate = DefineParam(Type::kRate, "70%");
		Param updateTrigger = DefineParam(Type::kSwitch, "off");
		Param mix = DefineParam(Type::kRate, "0%>100%");

		const std::unordered_map<ParamID, Param*> dict = {
			{ ParamID::kUpdatePeriod, &updatePeriod },
			{ ParamID::kWaveLength, &waveLength },
			{ ParamID::kRate, &rate },
			{ ParamID::kUpdateTrigger, &updateTrigger },
			{ ParamID::kMix, &mix },
		};

	private:
		// DSPパラメータ上のupdateTriggerはoff→onに変わった瞬間だけtrueにするので、前回の値を持っておく
		// TODO: 持つ場所は本当にここが適切なのか要検討
		bool m_prevRawUpdateTrigger = false;

	public:
		RetriggerDSPParams render(const Status& status, bool isOn)
		{
			// updateTriggerの"Off>OnMin-OnMax"のOffのトリガタイミングは事前に計算済みで別途secUntilTrigger側で処理されるため無視する
			// (ただし、"on>off-on"や"on-off"の場合はプレイ中にoff→onになりうるので無視せず、3つすべて"on"の場合のみ無視する。secUntilTriggerの方と多重に更新される場合もあることになるが実用上大した問題はない)
			const bool ignoreUpdateTrigger = updateTrigger.valueSet.off && updateTrigger.valueSet.onMin && updateTrigger.valueSet.onMax;

			// DSPパラメータ上のupdateTriggerはoff→onに変わった瞬間だけtrueにする
			const bool rawUpdateTrigger = ValueAsBool(GetValue(updateTrigger, status, isOn)) && !ignoreUpdateTrigger;
			const bool updateTrigger = !m_prevRawUpdateTrigger && rawUpdateTrigger;
			m_prevRawUpdateTrigger = rawUpdateTrigger;

			return {
				.updateTrigger = updateTrigger,
				.waveLength = GetValue(waveLength, status, isOn),
				.rate = GetValue(rate, status, isOn),
				.mix = GetValue(mix, status, isOn),
				// secUntilTriggerは利用側(BasicAudioEffectWithTrigger::updateStatus())で設定されるのでここでは指定しない
			};
		}
	};
}
