﻿#pragma once
#include "music_game/game_defines.hpp"
#include "judgment/combo_status.hpp"

namespace MusicGame
{
	struct PlayResult
	{
		int32 score = 0;

		int32 maxCombo = 0;

		int32 totalCombo = 0;

		Judgment::ComboStats comboStats;

		GaugeType gaugeType = GaugeType::kNormalGauge;

		double gaugePercentage = 0.0;

		double gaugePercentageHard = 0.0;

		Achievement achievement() const;

		Grade grade() const;
	};
}