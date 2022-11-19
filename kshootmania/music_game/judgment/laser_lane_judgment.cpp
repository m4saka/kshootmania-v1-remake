﻿#include "laser_lane_judgment.hpp"
#include "kson/util/timing_utils.hpp"
#include "kson/util/graph_utils.hpp"

namespace MusicGame::Judgment
{
	namespace
	{
		kson::ByPulse<int32> CreateLaserLineDirectionMap(const kson::ByPulse<kson::LaserSection>& lane)
		{
			kson::ByPulse<int32> directionMap;
			directionMap.emplace(kson::Pulse{ 0 }, 0);

			for (const auto& [y, sec] : lane)
			{
				kson::Pulse prevPulse = y;
				Optional<double> prevValue = none;
				for (const auto& [ry, v] : sec.v)
				{
					if (prevValue.has_value())
					{
						const int32 direction = Sign(v.v - prevValue.value());
						directionMap.insert_or_assign(prevPulse, direction);
					}
					prevPulse = y + ry;
					prevValue = v.v;
				}
				directionMap.insert_or_assign(prevPulse, 0);
			}

			return directionMap;
		}

		kson::ByPulse<JudgmentResult> CreateLineJudgmentResultArray(const kson::ByPulse<kson::LaserSection>& lane, const kson::BeatInfo& beatInfo)
		{
			kson::ByPulse<JudgmentResult> judgmentArray;

			for (const auto& [y, sec] : lane)
			{
				if (sec.v.empty())
				{
					continue;
				}
				
				const kson::RelPulse lastRy = sec.v.rbegin()->first;
				if (lastRy > 0)
				{
					// BPMをもとにLASERノーツのコンボ数を半減させるかを決める
					// (セクション途中でのBPM変更は特に加味しない)
					const bool halvesCombo = kson::TempoAt(y, beatInfo) >= kHalveComboBPMThreshold;
					const kson::RelPulse minPulseInterval = halvesCombo ? (kson::kResolution4 * 3 / 8) : (kson::kResolution4 * 3 / 16);
					const kson::RelPulse pulseInterval = halvesCombo ? (kson::kResolution4 / 8) : (kson::kResolution4 / 16);

					if (lastRy <= minPulseInterval)
					{
						judgmentArray.emplace(y, JudgmentResult::kUnspecified);
					}
					else
					{
						const kson::Pulse start = ((y + pulseInterval - 1) / pulseInterval + 1) * pulseInterval;
						const kson::Pulse end = y + lastRy - pulseInterval;

						for (kson::Pulse pulse = start; pulse < end; pulse += pulseInterval)
						{
							judgmentArray.emplace(pulse, JudgmentResult::kUnspecified);
						}
					}
				}
			}

			return judgmentArray;
		}

		kson::ByPulse<LaserSlamJudgment> CreateSlamJudgmentArray(const kson::ByPulse<kson::LaserSection>& lane, const kson::BeatInfo& beatInfo, const kson::TimingCache& timingCache)
		{
			kson::ByPulse<LaserSlamJudgment> judgmentArray;

			for (const auto& [y, sec] : lane)
			{
				for (const auto& [ry, point] : sec.v)
				{
					const bool isSlam = !kson::AlmostEquals(point.v, point.vf);
					if (isSlam)
					{
						const double slamTimeSec = kson::PulseToSec(y + ry, beatInfo, timingCache);
						const int32 slamDirection = Sign(point.vf - point.v);
						judgmentArray.emplace(y + ry, LaserSlamJudgment(slamTimeSec, slamDirection));
					}
				}
			}

			return judgmentArray;
		}

		Optional<double> GetPregeneratedCursorValue(const kson::ByPulse<kson::LaserSection>& lane, kson::Pulse currentPulse)
		{
			// 直近1小節以内にレーザーセクションの始点が存在すればカーソルが出ている
			const auto itr = kson::FirstInRange(lane, currentPulse, currentPulse + kson::kResolution4);
			if (itr != lane.end())
			{
				const auto& [_, sec] = *itr;
				if (!sec.v.empty())
				{
					const auto& [_, v] = *sec.v.begin();
					return v.v;
				}
			}

			return none;
		}
	}

	LaserSlamJudgment::LaserSlamJudgment(double sec, int32 direction)
		: m_sec(sec)
		, m_direction(direction)
	{
	}

	double LaserSlamJudgment::sec() const
	{
		return m_sec;
	}

	void LaserSlamJudgment::addDeltaCursorX(double deltaCursorX, double currentTimeSec)
	{
		if (Abs(currentTimeSec - m_sec) > TimingWindow::LaserNote::kWindowSecSlam)
		{
			// 判定時間外の場合は何もしない
			return;
		}

		if (isCriticalSatisfied())
		{
			// 既にCRITICAL判定に必要なカーソル移動量を満たしている場合は何もしない
			return;
		}

		if (Sign(deltaCursorX) == m_direction)
		{
			// 直角LASERと同方向の入力であれば、累計移動量に加算
			m_totalAbsDeltaCursorX += Abs(deltaCursorX);
		}
		else
		{
			// 直角LASERと逆方向の入力であれば、累計移動量から減算
			// (ただし、既に加算されたものを打ち消すだけで、0未満にはしない)
			m_totalAbsDeltaCursorX = Max(m_totalAbsDeltaCursorX - Abs(deltaCursorX), 0.0);
		}
	}

	bool LaserSlamJudgment::isCriticalSatisfied() const
	{
		return m_totalAbsDeltaCursorX >= kLaserSlamCriticalDeltaCursorXThreshold;
	}

	JudgmentResult LaserSlamJudgment::judgmentResult(double currentTimeSec) const
	{
		if (isCriticalSatisfied())
		{
			// CRITICAL判定の移動量を満たしていればCRITICAL判定
			return JudgmentResult::kCritical;
		}
		else if (currentTimeSec - m_sec <= TimingWindow::LaserNote::kWindowSecSlam)
		{
			// CRITICAL判定の移動量を満たしておらず、まだ判定時間中の場合は未判定
			return JudgmentResult::kUnspecified;
		}
		else
		{
			// CRITICAL判定の移動量を満たしておらず、既に判定時間を過ぎている場合はERROR判定
			return JudgmentResult::kError;
		}
	}

	void LaserLaneJudgment::processLineJudgment(double deltaCursorX, kson::Pulse currentPulse, LaserLaneStatus& laneStatusRef)
	{
		if (!laneStatusRef.cursorX.has_value())
		{
			// カーソルが出ていない場合は何もしない
			return;
		}

		if (!laneStatusRef.noteCursorX.has_value())
		{
			// 事前生成されたカーソルは動かさない(いわゆる始点ロック)
			return;
		}

		const int32 direction = Sign(deltaCursorX);
		if (direction == 0)
		{
			// 移動方向がない場合は何もしない
			return;
		}
		const int32 noteDirection = kson::ValueItrAt(m_laserLineDirectionMap, currentPulse)->second;
		const double noteCursorX = laneStatusRef.noteCursorX.value();
		const double cursorX = laneStatusRef.cursorX.value();
		double nextCursorX;
		if (direction == noteDirection || noteDirection == 0)
		{
			// LASERノーツと同方向にカーソル移動している、または、LASERノーツが横移動なしの場合
			const double overshootCursorX = cursorX + deltaCursorX * kLaserCursorInputOvershootScale; // 増幅移動量で計算したカーソル移動先
			if (Min(cursorX, overshootCursorX) <= noteCursorX && noteCursorX <= Max(cursorX, overshootCursorX))
			{
				// 増幅移動量で計算したカーソル移動先より手前に理想位置があれば、カーソルを理想位置へ吸い付かせる
				nextCursorX = noteCursorX;
			}
			else
			{
				// 増幅移動量で理想位置に届かなければ、カーソルを単純に動かす
				nextCursorX = cursorX + deltaCursorX;
			}
		}
		else
		{
			// LASERノーツと逆方向にカーソル移動している場合、カーソルを単純に動かす
			nextCursorX = cursorX + deltaCursorX;
		}
		laneStatusRef.cursorX = Clamp(nextCursorX, 0.0, 1.0);
	}

	void LaserLaneJudgment::processSlamJudgment(double deltaCursorX, double currentTimeSec, LaserLaneStatus& laneStatusRef)
	{
		// 直角LASERはまだカーソルが出ていなくても先行判定するので、カーソルの存在チェックはしない

		if (m_slamJudgmentArrayCursor == m_slamJudgmentArray.end())
		{
			// 残りの直角LASER判定がない場合は何もしない
			return;
		}

		// Note:
		// 1フレーム内では1つの直角LASERまでしか判定しない仕組みとしている
		// (1フレーム内で1つの直角LASER判定に必要なカーソル移動量を超える入力があったとしても、超えた分の移動量は特に加味せず破棄する)

		// 移動量を現在判定対象になっている直角LASERへ反映
		auto& [laserSlamPulse, laserSlamJudgmentRef] = *m_slamJudgmentArrayCursor;
		laserSlamJudgmentRef.addDeltaCursorX(deltaCursorX, currentTimeSec);

		// 判定が決まった場合
		const JudgmentResult judgmentResult = laserSlamJudgmentRef.judgmentResult(currentTimeSec);
		if (judgmentResult != JudgmentResult::kUnspecified)
		{
			// CRITICAL判定の場合はスコア加算
			// TODO: コンボ数加算
			if (judgmentResult == JudgmentResult::kCritical)
			{
				m_scoreValue += kScoreValueCritical;
			}

			// 判定した時間を記録(補正に使用)
			laneStatusRef.lastLaserSlamJudgedTimeSec = Max(currentTimeSec, laserSlamJudgmentRef.sec());
			laneStatusRef.lastJudgedLaserSlamPulse = laserSlamPulse;

			// 判定対象を次の直角LASERへ進める
			// (1フレームで複数の判定が過ぎ去っている可能性があるのでwhileで回す)
			while (true)
			{
				++m_slamJudgmentArrayCursor;

				// 末尾まで到達したら抜ける
				if (m_slamJudgmentArrayCursor == m_slamJudgmentArray.end())
				{
					break;
				}

				// 未判定である(まだERROR判定になっていない)直角LASERが見つかったら抜ける
				const auto& [_, nextLaserSlamJudgmentRef] = *m_slamJudgmentArrayCursor;
				if (nextLaserSlamJudgmentRef.judgmentResult(currentTimeSec) == JudgmentResult::kUnspecified)
				{
					break;
				}
			}
		}
	}

	void LaserLaneJudgment::processAutoCursorMovementBySlamJudgment(double currentTimeSec, LaserLaneStatus& laneStatusRef)
	{
		if (!laneStatusRef.cursorX.has_value())
		{
			// カーソルが出ていない場合は何もしない
			return;
		}

		if (!laneStatusRef.noteCursorX.has_value())
		{
			// 事前生成されたカーソルは動かさない(いわゆる始点ロック)
			return;
		}

		if (!laneStatusRef.currentLaserSectionPulse.has_value())
		{
			// noteCursorXが存在すればcurrentLaserSectionPulseも存在するはず
			assert(false && "currentLaserSectionPulse is none although noteCursorX is not none");
			return;
		}

		if (!laneStatusRef.lastLaserSlamJudgedTimeSec.has_value())
		{
			// 直角LASERをまだ一度も判定していない場合は何もしない
			return;
		}

		if (laneStatusRef.currentLaserSectionPulse.value() > laneStatusRef.lastJudgedLaserSlamPulse.value())
		{
			// 判定した直角LASERより後ろのLASERセクションを判定中の場合は補正しない
			return;
		}

		const double finishAutoTimeSec = laneStatusRef.lastLaserSlamJudgedTimeSec.value() + kLaserAutoSecAfterSlamJudgment;
		if (currentTimeSec < finishAutoTimeSec)
		{
			// 直角判定後の補正時間内であれば、カーソルを理想位置へ吸い付かせる
			laneStatusRef.cursorX = laneStatusRef.noteCursorX;
		}
	}

	void LaserLaneJudgment::processKeyPressed(KeyConfig::Button button, kson::Pulse currentPulse, double currentTimeSec, LaserLaneStatus& laneStatusRef)
	{
		assert(button == m_keyConfigButtonL || button == m_keyConfigButtonR);

		const int32 direction = button == m_keyConfigButtonL ? -1 : 1;
		const double deltaCursorX = kLaserKeyboardCursorXPerSec * Scene::DeltaTime() * direction;
		processLineJudgment(deltaCursorX, currentPulse, laneStatusRef);
		processSlamJudgment(deltaCursorX, currentTimeSec, laneStatusRef);
	}

	LaserLaneJudgment::LaserLaneJudgment(KeyConfig::Button keyConfigButtonL, KeyConfig::Button keyConfigButtonR, const kson::ByPulse<kson::LaserSection>& lane, const kson::BeatInfo& beatInfo, const kson::TimingCache& timingCache)
		: m_keyConfigButtonL(keyConfigButtonL)
		, m_keyConfigButtonR(keyConfigButtonR)
		, m_laserLineDirectionMap(CreateLaserLineDirectionMap(lane))
		, m_lineJudgmentArray(CreateLineJudgmentResultArray(lane, beatInfo))
		, m_slamJudgmentArray(CreateSlamJudgmentArray(lane, beatInfo, timingCache))
		, m_slamJudgmentArrayCursor(m_slamJudgmentArray.begin())
		, m_scoreValueMax(static_cast<int32>(m_lineJudgmentArray.size() + m_slamJudgmentArray.size()) * kScoreValueCritical)
	{
	}

	void LaserLaneJudgment::update(const kson::ByPulse<kson::LaserSection>& lane, kson::Pulse currentPulse, double currentTimeSec, LaserLaneStatus& laneStatusRef)
	{
		laneStatusRef.noteCursorX = kson::GraphSectionValueAt(lane, currentPulse);
		laneStatusRef.noteVisualCursorX = laneStatusRef.noteCursorX; // TODO: タイミング調整に合わせてずらして取得

		const auto pregeneratedCursorValue = GetPregeneratedCursorValue(lane, currentPulse);
		if (!laneStatusRef.cursorX.has_value() || laneStatusRef.currentLaserSectionPulse != m_prevCurrentLaserSectionPulse) // カーソルが出ていない、または前回とは異なるLASERセクションに突入した場合
		{
			if (laneStatusRef.currentLaserSectionPulse.has_value())
			{
				// 既にLASERセクションに突入している場合はカーソルを出現させる
				// そのLASERセクションの始点の値に合わせる(※現在の理想位置に合わせるのではない理由は、始点が直角の場合に直角の移動先にカーソルが合ってしまうため)
				laneStatusRef.cursorX = lane.at(laneStatusRef.currentLaserSectionPulse.value()).v.begin()->second.v;
			}
			else if (pregeneratedCursorValue.has_value())
			{
				// カーソルがLASERノーツの手前で事前生成された場合は、カーソルを出現させる
				// 次のLASERセクションの始点の値に合わせる
				laneStatusRef.cursorX = pregeneratedCursorValue;
			}
			else
			{
				laneStatusRef.cursorX = none;
			}
		}
		else // カーソルが出ている場合
		{
			// カーソルが消滅
			if (!laneStatusRef.noteCursorX.has_value() && !pregeneratedCursorValue.has_value())
			{
				laneStatusRef.cursorX = none;
			}
		}
		m_prevCurrentLaserSectionPulse = laneStatusRef.currentLaserSectionPulse;

		// 現在判定対象になっているLASERセクションの始点Pulse値を取得
		if (laneStatusRef.noteCursorX.has_value())
		{
			laneStatusRef.currentLaserSectionPulse = kson::ValueItrAt(lane, currentPulse)->first;
		}
		else
		{
			laneStatusRef.currentLaserSectionPulse = none;
		}

		// キー押下中の判定処理
		// (左向きキーと右向きキーを同時に押している場合、最後に押した方を優先する)
		const Optional<KeyConfig::Button> lastPressedButton = KeyConfig::LastPressed(m_keyConfigButtonL, m_keyConfigButtonR);
		if (lastPressedButton.has_value())
		{
			processKeyPressed(lastPressedButton.value(), currentPulse, currentTimeSec, laneStatusRef);
		}

		// 直角LASER判定直後のカーソル自動移動
		processAutoCursorMovementBySlamJudgment(currentTimeSec, laneStatusRef);
	}

	int32 LaserLaneJudgment::scoreValue() const
	{
		return m_scoreValue;
	}

	int32 LaserLaneJudgment::scoreValueMax() const
	{
		return m_scoreValueMax;
	}
}
