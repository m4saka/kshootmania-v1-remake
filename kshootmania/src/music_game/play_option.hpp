﻿#pragma once

namespace MusicGame
{
	using IsAutoPlayYN = YesNo<struct IsAutoPlayYN_tag>;

	struct PlayOption
	{
		IsAutoPlayYN isAutoPlay = IsAutoPlayYN::No;

		GaugeType gaugeType = GaugeType::kNormalGauge;

		JudgmentPlayMode btJudgmentPlayMode() const
		{
			return isAutoPlay ? JudgmentPlayMode::kAuto : JudgmentPlayMode::kOn;
		}

		JudgmentPlayMode fxJudgmentPlayMode() const
		{
			return isAutoPlay ? JudgmentPlayMode::kAuto : JudgmentPlayMode::kOn;
		}

		JudgmentPlayMode laserJudgmentPlayMode() const
		{
			return isAutoPlay ? JudgmentPlayMode::kAuto : JudgmentPlayMode::kOn;
		}
	};
}