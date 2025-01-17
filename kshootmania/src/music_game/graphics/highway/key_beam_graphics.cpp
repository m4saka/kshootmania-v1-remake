﻿#include "key_beam_graphics.hpp"
#include "music_game/graphics/graphics_defines.hpp"
#include "music_game/camera/camera_math.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kKeyBeamTextureFilename = U"judge.gif";

		constexpr Size kBTKeyBeamTextureSize = { 40, 300 };
		constexpr Size kFXKeyBeamTextureSize = { 82, 300 };

		constexpr double kKeyBeamFullWidthSec = 0.075;
		constexpr double kKeyBeamEndSec = 0.155;
		constexpr Vec2 kKeyBeamPositionOffset = kLanePositionOffset + Vec2{ 0.0, kHighwayTextureSize.y - 300.0 };
	}

	KeyBeamGraphics::KeyBeamGraphics()
		: m_beamTexture(TextureAsset(kKeyBeamTextureFilename))
	{
	}

	void KeyBeamGraphics::draw(const GameStatus& gameStatus, const ViewStatus& viewStatus, const HighwayRenderTexture& target) const
	{
		const ScopedRenderTarget2D renderTarget(target.additiveTexture());
		const ScopedRenderStates2D renderState(BlendState::Additive);

		for (std::size_t i = 0; i < kson::kNumBTLanesSZ + kson::kNumFXLanesSZ; ++i)
		{
			const bool isBT = (i < kson::kNumBTLanesSZ);
			const std::size_t numLanes = isBT ? kson::kNumBTLanesSZ : kson::kNumFXLanesSZ;
			const std::size_t laneIdx = isBT ? i : (i - kson::kNumBTLanesSZ);
			const double centerSplitShiftX = Camera::CenterSplitShiftX(viewStatus.camStatus.centerSplit) * ((laneIdx >= numLanes / 2) ? 1 : -1);
			const ButtonLaneStatus& laneStatus = isBT ? gameStatus.btLaneStatus[laneIdx] : gameStatus.fxLaneStatus[laneIdx];

			const double sec = gameStatus.currentTimeSec - laneStatus.keyBeamTimeSec;
			if (sec < 0.0 || kKeyBeamEndSec < sec)
			{
				continue;
			}

			double widthRate = 1.0;
			double alpha = 1.0;
			if (sec < kKeyBeamFullWidthSec)
			{
				widthRate = sec / kKeyBeamFullWidthSec;
			}
			else
			{
				alpha = 1.0 - (sec - kKeyBeamFullWidthSec) / (kKeyBeamEndSec - kKeyBeamFullWidthSec);
			}

			const TextureRegion beamTextureRegion = m_beamTexture(
				kBTKeyBeamTextureSize.x * (static_cast<double>(static_cast<int32>(laneStatus.keyBeamType)) + 0.5 - widthRate / 2),
				0,
				kBTKeyBeamTextureSize.x * widthRate,
				kBTKeyBeamTextureSize.y);

			const double dLaneIdx = static_cast<double>(laneIdx);

			if (isBT)
			{
				beamTextureRegion
					.draw(kKeyBeamPositionOffset + Vec2::Right(centerSplitShiftX) + kBTLanePositionDiff * (dLaneIdx + 0.5 - widthRate / 2), ColorF{ 1.0, alpha });
			}
			else
			{
				beamTextureRegion
					.resized(kFXKeyBeamTextureSize.x * widthRate, kFXKeyBeamTextureSize.y)
					.draw(kKeyBeamPositionOffset + Vec2::Right(centerSplitShiftX) + kFXLanePositionDiff * dLaneIdx + kFXKeyBeamTextureSize * (0.5 - widthRate / 2), ColorF{ 1.0, alpha });
			}
		}
	}
}
