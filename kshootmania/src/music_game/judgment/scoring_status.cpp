﻿#include "scoring_status.hpp"

namespace MusicGame::Judgment
{
	void ScoringStatus::addGaugeValue(int32 add)
	{
		m_gaugeValue = Min(m_gaugeValue + add, m_gaugeValueMax);
	}

	void ScoringStatus::subtractGaugeValue(int32 sub)
	{
		m_gaugeValue = Max(m_gaugeValue - sub, 0);
	}

	ScoringStatus::ScoringStatus(int32 scoreValueMax, int32 gaugeValueMax)
		: m_scoreValueMax(scoreValueMax)
		, m_gaugeValueMax(gaugeValueMax)
	{
	}

	void ScoringStatus::onChipOrLaserSlamJudgment(Judgment::JudgmentResult result)
	{
		m_comboStatus.processJudgmentResult(result);

		switch (result)
		{
		case Judgment::JudgmentResult::kCritical:
			m_scoreValue += Judgment::kScoreValueCritical;
			addGaugeValue(kGaugeValueChip);
			break;

		case Judgment::JudgmentResult::kNearFast:
		case Judgment::JudgmentResult::kNearSlow:
			m_scoreValue += Judgment::kScoreValueNear;
			addGaugeValue(kGaugeValueChipNear);
			break;

		case Judgment::JudgmentResult::kError:
			subtractGaugeValue(static_cast<int32>(m_gaugeValueMax * kGaugeDecreasePercentByChipError / 100));
			break;

		default:
			assert(false && "Invalid JudgmentResult in doChipJudgment");
			break;
		}
	}

	void ScoringStatus::onLongOrLaserLineJudgment(Judgment::JudgmentResult result)
	{
		m_comboStatus.processJudgmentResult(result);

		switch (result)
		{
		case Judgment::JudgmentResult::kCritical:
			m_scoreValue += Judgment::kScoreValueCritical;
			addGaugeValue(kGaugeValueLong);
			break;

		case Judgment::JudgmentResult::kError:
			subtractGaugeValue(static_cast<int32>(m_gaugeValueMax * kGaugeDecreasePercentByLongError / 100));
			break;

		default:
			assert(false && "Invalid JudgmentResult in doLongJudgment");
			break;
		}
	}

	int32 ScoringStatus::score() const
	{
		if (m_scoreValueMax == 0)
		{
			return 0;
		}
		return static_cast<int32>(static_cast<int64>(kScoreMax) * m_scoreValue / m_scoreValueMax);
	}

	double ScoringStatus::gaugePercentage() const
	{
		if (m_gaugeValueMax == 0)
		{
			return 0.0;
		}
		return 100.0 * m_gaugeValue / m_gaugeValueMax;
	}

	int32 ScoringStatus::combo() const
	{
		return m_comboStatus.combo();
	}

	int32 ScoringStatus::maxCombo() const
	{
		return m_comboStatus.maxCombo();
	}

	const ComboStats& ScoringStatus::comboStats() const
	{
		return m_comboStatus.stats();
	}

	bool ScoringStatus::isNoError() const
	{
		return m_comboStatus.isNoError();
	}

	int32 ScoringStatus::totalJudgedCombo() const
	{
		return m_comboStatus.totalJudgedCombo();
	}
}
