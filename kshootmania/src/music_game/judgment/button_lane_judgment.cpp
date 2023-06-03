﻿#include "button_lane_judgment.hpp"
#include "music_game/graphics/graphics_defines.hpp"

namespace MusicGame::Judgment
{
	namespace
	{
		std::map<kson::Pulse, double> CreatePulseToSec(const kson::ByPulse<kson::Interval>& lane, const kson::BeatInfo& beatInfo, const kson::TimingCache& timingCache)
		{
			std::map<kson::Pulse, double> pulseToSec;

			for (const auto& [y, note] : lane)
			{
				if (!pulseToSec.contains(y))
				{
					const double sec = kson::PulseToSec(y, beatInfo, timingCache);
					pulseToSec.emplace(y, sec);
				}
				if (!pulseToSec.contains(y + note.length))
				{
					const double sec = kson::PulseToSec(y + note.length, beatInfo, timingCache);
					pulseToSec.emplace(y + note.length, sec);
				}
			}

			return pulseToSec;
		}

		kson::ByPulse<JudgmentResult> CreateChipNoteJudgmentArray(const kson::ByPulse<kson::Interval>& lane)
		{
			kson::ByPulse<JudgmentResult> judgmentArray;

			for (const auto& [y, note] : lane)
			{
				if (note.length == 0)
				{
					judgmentArray.emplace(y, JudgmentResult::kUnspecified);
				}
			}

			return judgmentArray;
		}

		using LongNoteJudgment = ButtonLaneJudgment::LongNoteJudgment;

		kson::ByPulse<ButtonLaneJudgment::LongNoteJudgment> CreateLongNoteJudgmentArray(const kson::ByPulse<kson::Interval>& lane, const kson::BeatInfo& beatInfo)
		{
			// HSP版: https://github.com/kshootmania/ksm-v1/blob/8deaf1fd147f6e13ac6665731e1ff1e00c5b4176/src/scene/play/play_chart_load.hsp#L1707-L1761

			kson::ByPulse<LongNoteJudgment> judgmentArray;

			for (const auto& [y, note] : lane)
			{
				if (note.length > 0)
				{
					// BPMをもとにロングノーツのコンボ数を半減させるかを決める
					// (ノーツ途中でのBPM変更は特に加味しない)
					const bool halvesCombo = kson::TempoAt(y, beatInfo) >= kHalveComboBPMThreshold;
					const kson::RelPulse minPulseInterval = halvesCombo ? (kson::kResolution4 * 3 / 8) : (kson::kResolution4 * 3 / 16);
					const kson::RelPulse pulseInterval = halvesCombo ? (kson::kResolution4 / 8) : (kson::kResolution4 / 16);

					if (note.length < pulseInterval * 2)
					{
						judgmentArray.emplace(y, LongNoteJudgment{ .length = note.length });
					}
					else if (note.length <= minPulseInterval)
					{
						judgmentArray.emplace(y + pulseInterval, LongNoteJudgment{ .length = pulseInterval });
					}
					else
					{
						const kson::Pulse start = ((y + pulseInterval - 1) / pulseInterval + 1) * pulseInterval;
						const kson::Pulse end = y + note.length - pulseInterval;

						for (kson::Pulse pulse = start; pulse < end; pulse += pulseInterval)
						{
							const kson::RelPulse length = (pulse <= end - pulseInterval) ? pulseInterval : (pulseInterval * 2); // 末尾の判定のみ2倍の長さ
							judgmentArray.emplace(pulse, LongNoteJudgment{ .length = length });
						}
					}
				}
			}

			return judgmentArray;
		}

		bool IsDuringLongNote(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse)
		{
			const auto& currentNoteItr = kson::ValueItrAt(lane, currentPulse);
			if (currentNoteItr != lane.end())
			{
				const auto& [y, note] = *currentNoteItr;
				if (y <= currentPulse && currentPulse < y + note.length)
				{
					return true;
				}
			}
			return false;
		}
	}

	void ButtonLaneJudgment::processKeyDown(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, double currentTimeSec, ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef)
	{
		using namespace TimingWindow;

		// レーン上で最も現在時間に近いノーツを調べる
		bool found = false;
		double minDistance = 0.0;
		kson::Pulse nearestNotePulse;
		for (auto itr = m_passedNoteCursor; itr != lane.end(); ++itr)
		{
			const auto& [y, note] = *itr;
			const double sec = m_pulseToSec.at(y);
			const double endSec = (note.length == 0) ? sec : m_pulseToSec.at(y + note.length);
			if (currentTimeSec - endSec >= ChipNote::kWindowSecError)
			{
				continue;
			}

			const double diffSec = sec - currentTimeSec;
			if (note.length == 0) // Chip note
			{
				if (m_chipJudgmentArray.at(y) != JudgmentResult::kUnspecified)
				{
					continue;
				}

				if (!found || Abs(diffSec) < minDistance)
				{
					nearestNotePulse = y;
					minDistance = Abs(diffSec);
					found = true;
				}
				else if (found && Abs(diffSec) >= minDistance && y > currentPulse)
				{
					break;
				}
			}
			else // Long note
			{
				if ((!found || Abs(diffSec) < minDistance) && diffSec <= LongNote::kWindowSecPreHold && (y + note.length > currentPulse))
				{
					laneStatusRef.currentLongNotePulse = y;
					laneStatusRef.currentLongNoteAnimOffsetTimeSec = currentTimeSec;
					return;
				}
				else if (found && diffSec > LongNote::kWindowSecPreHold && y > currentPulse)
				{
					break;
				}
			}
		}

		laneStatusRef.keyBeamTimeSec = currentTimeSec;
		laneStatusRef.keyBeamType = KeyBeamType::kDefault;

		if (found)
		{
			// チップノーツの判定
			Optional<JudgmentResult> chipAnimType = none;
			if (minDistance < ChipNote::kWindowSecCritical)
			{
				// CRITICAL判定
				m_chipJudgmentArray.at(nearestNotePulse) = JudgmentResult::kCritical;
				judgmentHandlerRef.onChipJudged(JudgmentResult::kCritical);
				laneStatusRef.keyBeamType = KeyBeamType::kCritical;
				chipAnimType = JudgmentResult::kCritical;
			}
			else if (minDistance < ChipNote::kWindowSecNear)
			{
				// NEAR判定
				m_chipJudgmentArray.at(nearestNotePulse) = JudgmentResult::kNear;
				judgmentHandlerRef.onChipJudged(JudgmentResult::kNear);
				laneStatusRef.keyBeamType = KeyBeamType::kNear; // TODO: fast/slow
				chipAnimType = JudgmentResult::kNear; // TODO: fast/slow
			}
			else if (minDistance < ChipNote::kWindowSecError) // TODO: easy gauge, fast/slow
			{
				// ERROR判定
				m_chipJudgmentArray.at(nearestNotePulse) = JudgmentResult::kError;
				judgmentHandlerRef.onChipJudged(JudgmentResult::kError);
				laneStatusRef.keyBeamType = KeyBeamType::kDefault;
				chipAnimType = JudgmentResult::kError;
			}

			if (chipAnimType.has_value())
			{
				laneStatusRef.chipAnim.push({
					.startTimeSec = currentTimeSec,
					.type = *chipAnimType,
				});
			}
		}
	}

	void ButtonLaneJudgment::processKeyPressed(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, const ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef)
	{
		if (laneStatusRef.currentLongNotePulse.has_value())
		{
			const kson::Pulse noteStartPulse = laneStatusRef.currentLongNotePulse.value();
			const kson::Pulse noteEndPulse = noteStartPulse + lane.at(noteStartPulse).length;
			const kson::Pulse limitPulse = Min(currentPulse + kson::RelPulse{ 1 }, noteEndPulse);

			// 処理落ちした場合でも判定が漏れないように前回フレームからの判定を全て拾う
			for (auto itr = m_longJudgmentArray.upper_bound(noteStartPulse - 1); itr != m_longJudgmentArray.end(); ++itr)
			{
				auto& [y, judgment] = *itr;
				if (y + judgment.length <= m_prevPulse)
				{
					continue;
				}

				if (y >= limitPulse)
				{
					break;
				}

				if (judgment.result != JudgmentResult::kUnspecified)
				{
					// 判定が既に決まっている場合はスキップ
					continue;
				}

				// ロングノーツのCRITICAL判定
				judgment.result = JudgmentResult::kCritical;
				judgmentHandlerRef.onLongJudged(JudgmentResult::kCritical);
			}
		}
	}

	void ButtonLaneJudgment::processPassedNoteJudgment(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, double currentTimeSec, ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef)
	{
		using namespace TimingWindow;

		for (auto itr = m_passedNoteCursor; itr != lane.end(); ++itr)
		{
			const auto& [y, note] = *itr;
			const double passSec = (note.length == 0) ? m_pulseToSec.at(y) + ChipNote::kWindowSecError : m_pulseToSec.at(y + note.length);
			if (currentTimeSec >= passSec)
			{
				// 通過済みチップノーツのERROR判定
				if (note.length == 0 && m_chipJudgmentArray.at(y) == JudgmentResult::kUnspecified)
				{
					m_chipJudgmentArray.at(y) = JudgmentResult::kError;
					judgmentHandlerRef.onChipJudged(JudgmentResult::kError);

					laneStatusRef.chipAnim.push({
						.startTimeSec = currentTimeSec,
						.type = JudgmentResult::kError,
					});
				}

				m_passedNoteCursor = std::next(itr);
			}
		}

		for (auto itr = m_passedLongJudgmentCursor; itr != m_longJudgmentArray.end(); ++itr)
		{
			auto& [y, judgment] = *itr;
			if (y + judgment.length < currentPulse)
			{
				// 通過済みロングノーツのERROR判定
				if (judgment.result == JudgmentResult::kUnspecified)
				{
					judgment.result = JudgmentResult::kError;
					judgmentHandlerRef.onLongJudged(JudgmentResult::kError);
				}

				m_passedLongJudgmentCursor = std::next(itr);
			}
		}
	}

	ButtonLaneJudgment::ButtonLaneJudgment(KeyConfig::Button keyConfigButton, const kson::ByPulse<kson::Interval>& lane, const kson::BeatInfo& beatInfo, const kson::TimingCache& timingCache)
		: m_keyConfigButton(keyConfigButton)
		, m_pulseToSec(CreatePulseToSec(lane, beatInfo, timingCache))
		, m_chipJudgmentArray(CreateChipNoteJudgmentArray(lane))
		, m_longJudgmentArray(CreateLongNoteJudgmentArray(lane, beatInfo))
		, m_passedNoteCursor(lane.begin())
		, m_passedLongJudgmentCursor(m_longJudgmentArray.begin())
	{
	}

	void ButtonLaneJudgment::update(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, double currentTimeSec, ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef)
	{
		// チップノーツとロングノーツの始点の判定処理
		if (KeyConfig::Down(m_keyConfigButton))
		{
			processKeyDown(lane, currentPulse, currentTimeSec, laneStatusRef, judgmentHandlerRef);
		}

		// ロングノーツ押下中の判定処理
		if (KeyConfig::Pressed(m_keyConfigButton))
		{
			processKeyPressed(lane, currentPulse, laneStatusRef, judgmentHandlerRef);
		}

		// ロングノーツを離したときの判定処理
		if (laneStatusRef.currentLongNotePulse.has_value() &&
			(KeyConfig::Up(m_keyConfigButton) || (*laneStatusRef.currentLongNotePulse + lane.at(*laneStatusRef.currentLongNotePulse).length < currentPulse)))
		{
			laneStatusRef.currentLongNotePulse = none;
			laneStatusRef.currentLongNoteAnimOffsetTimeSec = currentTimeSec;
		}

		// 通り過ぎたノーツをERROR判定にする
		processPassedNoteJudgment(lane, currentPulse, currentTimeSec, laneStatusRef, judgmentHandlerRef);

		if (IsDuringLongNote(lane, currentPulse))
		{
			laneStatusRef.longNotePressed = laneStatusRef.currentLongNotePulse.has_value();
		}
		else
		{
			laneStatusRef.longNotePressed = none;
		}

		m_prevPulse = currentPulse;
	}

	std::size_t ButtonLaneJudgment::chipJudgmentCount() const
	{
		return m_chipJudgmentArray.size();
	}

	std::size_t ButtonLaneJudgment::longJudgmentCount() const
	{
		return m_longJudgmentArray.size();
	}
}
