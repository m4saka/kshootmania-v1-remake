﻿#include "ksmaudio/audio_effect/audio_effect_param.hpp"
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include "ksmaudio/audio_effect/detail/wave_length_utils.hpp"

namespace ksmaudio::AudioEffect
{
	// Implementation in HSP: https://github.com/m4saka/kshootmania-v1-hsp/blob/19bfb6acbec8abd304b2e7dae6009df8e8e1f66f/src/scene/play/play_utils.hsp#L405
	// TODO: 正常な文字列かどうか検証するための関数を別途設ける
	float StrToValue(Type type, const std::string& str)
	{
		try
		{
			switch (type)
			{
			case Type::kLength:
			case Type::kWaveLength:
				// value
				//   > 0: measure
				//   < 0: sec
				if (str.ends_with("ms"))
				{
					return -std::max(std::stof(str), 0.0f) / 1000;
				}
				else if (str.ends_with('s') && !str.ends_with("es")) // Do not allow "XXXsamples"
				{
					return -std::max(std::stof(str), 0.0f);
				}
				else if (str.starts_with("1/"))
				{
					const int d = std::stoi(str.substr(2U)); // 2 = strlen("1/")
					if (d > 0)
					{
						return 1.0f / d;
					}
					else
					{
						return 0.0f;
					}
				}
				return std::max(std::stof(str), 0.0f);

			case Type::kSample:
				if (str.ends_with("samples"))
				{
					return std::clamp(std::stof(str), 0.0f, 44100.0f);
				}
				break;

			case Type::kSwitch:
				if (str == "on")
				{
					return 1.0f;
				}
				return 0.0f;

			case Type::kRate:
				if (str.ends_with('%'))
				{
					return std::clamp(std::stof(str) / 100, 0.0f, 1.0f);
				}
				return std::clamp(std::stof(str), 0.0f, 1.0f);

			case Type::kFreq:
				if (str.ends_with("kHz"))
				{
					return std::max(std::stof(str), 0.0f) * 1000;
				}
				else if (str.ends_with("Hz"))
				{
					return std::max(std::stof(str), 0.0f);
				}
				return 0.0f;

			case Type::kPitch:
				// param
				//   > 0: not quantized (real_value + 48)
				//   < 0: quantized (-real_value - 48)
				{
					const float value = std::stof(str);
					if ((value >= -48.0f) && (value <= 48.0f))
					{
						if (str.find('.') != std::string::npos)
						{
							return value + 48.0f;
						}
						else
						{
							return -(value + 48.0f);
						}
					}
				}
				return 0.0f;

			case Type::kInt:
				return static_cast<float>(std::stoi(str));

			case Type::kFloat:
				return std::stof(str);

			case Type::kFilename:
				return 0.0f;
			}
		}
		catch ([[maybe_unused]] const std::invalid_argument& e)
		{
			// Just ignore errors here
		}
		catch ([[maybe_unused]] const std::out_of_range& e)
		{
			// Just ignore errors here
		}
		return 0.0f;
	}

	ValueSet StrToValueSet(Type type, const std::string& str, bool* pSuccess)
	{
		const std::size_t pos1 = str.find('>');
		const std::size_t pos2FindStart = ((pos1 == std::string::npos) ? 0U : pos1 + 1U/*'>'*/) + 1U/*negative sign '-'*/;
		const std::size_t pos2 = str.find('-', pos2FindStart);

		const std::string offStr = str.substr(0U, pos1);
		const std::string onMinStr = (pos1 == std::string::npos) ? offStr : str.substr(pos1 + 1U/*'>'*/, pos2);
		const std::string onMaxStr = (pos2 == std::string::npos) ? onMinStr : str.substr(pos2 + 1U/*'-'*/);

		ValueSet valueSet = {
			.off = StrToValue(type, offStr),
			.onMin = StrToValue(type, onMinStr),
			.onMax = StrToValue(type, onMaxStr),
		};

		// For length parameters, the min and max values must have the same sign.
		// Otherwise, a value set of 0 is returned.
		if (type == Type::kLength && ((valueSet.onMin < 0.0f) != (valueSet.onMax < 0.0f)))
		{
			if (pSuccess != nullptr)
			{
				*pSuccess = false;
			}
			return {};
		}

		if (pSuccess != nullptr)
		{
			*pSuccess = true;
		}

		// For pitch parameters, the min and max values must have the same sign.
		// Otherwise, quantization is disabled (i.e., "0.0-12" is replaced by "0.0-12.0").
		if (type == Type::kPitch && ((valueSet.onMin < 0.0f) != (valueSet.onMax < 0.0f)))
		{
			return {
				.off = valueSet.off,
				.onMin = std::abs(valueSet.onMin),
				.onMax = std::abs(valueSet.onMax),
			};
		}

		return valueSet;
	}

	bool ValueAsBool(float value)
	{
		constexpr float kBoolThreshold = 0.999f;
		return value > kBoolThreshold;
	}

	float GetValue(const Param& param, const Status& status, bool isOn)
	{
		const float lerped = isOn ? std::lerp(param.valueSet.onMin, param.valueSet.onMax, status.v) : param.valueSet.off;

		if (param.type == Type::kLength)
		{
			if (lerped > 0.0f)
			{
				// Tempo-synced
				return lerped * 4 * 60 / status.bpm;
			}
			else
			{
				// Not tempo-synced
				return -lerped;
			}
		}

		if (param.type == Type::kWaveLength)
		{
			if (lerped > 0.0f)
			{
				// Tempo-synced
				float waveLength;
				if (isOn)
				{
					waveLength = detail::WaveLengthUtils::Interpolate(param.valueSet.onMin, param.valueSet.onMax, status.v);
				}
				else
				{
					waveLength = param.valueSet.off;
				}
				return waveLength * 4 * 60 / status.bpm;
			}
			else
			{
				// Not tempo-synced
				return -lerped;
			}
		}
		
		if (param.type == Type::kPitch)
		{
			if (lerped > 0.0f)
			{
				// Not quantized
				return lerped - 48.0f;
			}
			else
			{
				// Quantized
				return std::floor(-lerped - 48.0f);
			}
		}

		return lerped;
	}

	bool GetValueAsBool(const Param& param, const Status& status, bool isOn)
	{
		return ValueAsBool(GetValue(param, status, isOn));
	}

	int GetValueAsInt(const Param& param, const Status& status, bool isOn)
	{
		return static_cast<int>(GetValue(param, status, isOn));
	}

	Param DefineParam(Type type, const std::string& valueSetStr)
	{
		return {
			.type = type,
			.valueSet = StrToValueSet(type, valueSetStr),
		};
	}

	TapestopTriggerParam DefineTapestopTriggerParam(const std::string& valueSetStr)
	{
		return TapestopTriggerParam(StrToValueSet(Type::kSwitch, valueSetStr));
	}

	UpdateTriggerParam DefineUpdateTriggerParam(const std::string& valueSetStr)
	{
		return UpdateTriggerParam(StrToValueSet(Type::kSwitch, valueSetStr));
	}
}
