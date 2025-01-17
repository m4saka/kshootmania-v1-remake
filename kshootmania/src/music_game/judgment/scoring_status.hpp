﻿#pragma once
#include "music_game/game_defines.hpp"
#include "judgment_defines.hpp"
#include "combo_status.hpp"

namespace MusicGame::Judgment
{
	class ScoringStatus
	{
	private:
		const int32 m_scoreValueMax = 0;
		const int32 m_gaugeValueMax = 0;

		int32 m_scoreValue = 0;
		int32 m_gaugeValue = 0;

		ComboStatus m_comboStatus;

		void addGaugeValue(int32 add);

		void subtractGaugeValue(int32 sub);

	public:
		ScoringStatus(int32 scoreValueMax, int32 gaugeValueMax);

		void onChipOrLaserSlamJudgment(Judgment::JudgmentResult result);

		void onLongOrLaserLineJudgment(Judgment::JudgmentResult result);

		int32 score() const;

		/// @brief NORMAL/EASYゲージのパーセンテージを返す
		/// @return パーセンテージ(0.0～100.0)
		double gaugePercentage() const;

		// TODO: HARDゲージのパーセンテージを返す

		int32 combo() const;

		int32 maxCombo() const;

		const ComboStats& comboStats() const;

		bool isNoError() const;

		int32 totalJudgedCombo() const;
	};
}
