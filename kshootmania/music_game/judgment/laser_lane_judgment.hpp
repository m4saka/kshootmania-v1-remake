﻿#pragma once
#include "music_game/game_defines.hpp"
#include "music_game/game_status.hpp"
#include "kson/chart_data.hpp"
#include "kson/util/timing_utils.hpp"

namespace MusicGame::Judgment
{
	class LaserSlamJudgment
	{
	private:
		// 時間(秒)
		const double m_sec;

		// 方向(-1:左, +1:右)
		const int32 m_direction;

		// カーソルの累計移動量
		double m_totalAbsDeltaCursorX = 0.0;

	public:
		LaserSlamJudgment(double sec, int32 direction);

		double sec() const;

		void addDeltaCursorX(double deltaCursorX, double currentTimeSec);

		bool isCriticalSatisfied() const;

		JudgmentResult judgmentResult(double currentTimeSec) const;
	};

	class LaserLaneJudgment
	{
	private:
		const KeyConfig::Button m_keyConfigButtonL;
		const KeyConfig::Button m_keyConfigButtonR;
		const std::map<kson::Pulse, double> m_pulseToSec;
		const kson::ByPulse<int32> m_laserLineDirectionMap;

		kson::ByPulse<JudgmentResult> m_lineJudgmentArray;

		kson::ByPulse<LaserSlamJudgment> m_slamJudgmentArray;
		kson::ByPulse<LaserSlamJudgment>::iterator m_slamJudgmentArrayCursor;

		Optional<kson::Pulse> m_prevCurrentLaserSectionPulse = none;

		int32 m_scoreValue = 0;

		const int32 m_scoreValueMax;

		void processLineJudgment(double deltaCursorX, kson::Pulse currentPulse, LaserLaneStatus& laneStatusRef);

		void processSlamJudgment(double deltaCursorX, double currentTimeSec, LaserLaneStatus& laneStatusRef);

		void processAutoCursorMovementBySlamJudgment(double currentTimeSec, LaserLaneStatus& laneStatusRef);

		void processKeyPressed(KeyConfig::Button button, kson::Pulse currentPulse, double currentSec, LaserLaneStatus& laneStatusRef);

	public:
		LaserLaneJudgment(KeyConfig::Button keyConfigButtonL, KeyConfig::Button keyConfigButtonR, const kson::ByPulse<kson::LaserSection>& lane, const kson::BeatInfo& beatInfo, const kson::TimingCache& timingCache);

		void update(const kson::ByPulse<kson::LaserSection>& lane, kson::Pulse currentPulse, double currentSec, LaserLaneStatus& laneStatusRef);

		int32 scoreValue() const;

		int32 scoreValueMax() const;
	};
}
