﻿#include "laser_note_graphics.hpp"
#include "note_graphics_utils.hpp"
#include "music_game/graphics/graphics_defines.hpp"

namespace
{
	constexpr StringView kLaserNoteTextureFilename = U"laser.gif";
	constexpr StringView kLaserNoteMaskTextureFilename = U"laser_mask.gif";
	constexpr StringView kLaserNoteLeftStartTextureFilename = U"laserl_0.gif";
	constexpr StringView kLaserNoteRightStartTextureFilename = U"laserr_0.gif";

	constexpr Size kLaserTextureSize = { 48, 48 };
	constexpr double kLaserLineWidth = static_cast<double>(kLaserTextureSize.x);
	constexpr Size kLaserStartTextureSize = { 44, 200 };
	constexpr double kLaserShiftY = -10;

	Quad LaserLineQuad(const Vec2& positionStart, const Vec2& positionEnd)
	{
		return {
			positionStart + Vec2{ kLaserLineWidth / 2, 0.0 },
			positionStart + Vec2{ -kLaserLineWidth / 2, 0.0 },
			positionEnd + Vec2{ -kLaserLineWidth / 2, 0.0 },
			positionEnd + Vec2{ kLaserLineWidth / 2, 0.0 }
		};
	}

	Quad LaserSlamLineQuad(const Vec2& positionStart, const Vec2& positionEnd)
	{
		if (Abs(positionEnd.x - positionStart.x) <= kLaserLineWidth)
		{
			// Too short to draw line
			return { Vec2::Zero(), Vec2::Zero(), Vec2::Zero(), Vec2::Zero() };
		}

		const int diffXSign = Sign(positionEnd.x - positionStart.x);
		return {
			positionStart + Vec2{ diffXSign * kLaserLineWidth / 2, -kLaserLineWidth },
			positionEnd + Vec2{ -diffXSign * kLaserLineWidth / 2, -kLaserLineWidth },
			positionEnd + Vec2{ -diffXSign * kLaserLineWidth / 2, 0.0 },
			positionStart + Vec2{ diffXSign * kLaserLineWidth / 2, 0.0 }
		};
	}
}

MusicGame::Graphics::LaserNoteGraphics::LaserNoteGraphics()
	: m_laserNoteTexture(TextureAsset(kLaserNoteTextureFilename))
	, m_laserNoteMaskTexture(TextureAsset(kLaserNoteMaskTextureFilename))
	, m_laserNoteLeftStartTexture(kLaserNoteLeftStartTextureFilename,
		{
			.column = 2,
			.sourceSize = kLaserStartTextureSize,
		})
	, m_laserNoteRightStartTexture(kLaserNoteRightStartTextureFilename,
		{
			.column = 2,
			.sourceSize = kLaserStartTextureSize,
		})
{
}

void MusicGame::Graphics::LaserNoteGraphics::draw(const kson::ChartData& chartData, const GameStatus& gameStatus, const RenderTexture& additiveTarget, const RenderTexture& invMultiplyTarget) const
{
	const ScopedRenderStates2D defaultSamplerState(SamplerState::ClampNearest);
	const ScopedRenderStates2D defaultRenderState(BlendState::Additive);
	const ScopedRenderTarget2D defaultRenderTarget(additiveTarget);

	const std::array<std::pair<std::reference_wrapper<const RenderTexture>, std::reference_wrapper<const Texture>>, 2> drawTexturePairs = {
		std::make_pair(std::ref(additiveTarget), std::ref(m_laserNoteTexture)),
		std::make_pair(std::ref(invMultiplyTarget), std::ref(m_laserNoteMaskTexture)),
	};

	// Draw laser notes
	for (std::size_t laneIdx = 0; laneIdx < kson::kNumLaserLanesSZ; ++laneIdx)
	{
		const auto& lane = chartData.note.laser[laneIdx];
		for (const auto& [y, laserSection] : lane)
		{
			const double positionSectionStartY = static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY;
			if (positionSectionStartY + kLaserStartTextureSize.y < 0)
			{
				// Laser section is above the drawing area
				break;
			}

			for (auto itr = laserSection.v.begin(); itr != laserSection.v.end(); ++itr)
			{
				const auto& [ry, point] = *itr;

				// Draw laser start texture
				if (itr == laserSection.v.begin())
				{
					for (int32 i = 0; i < 2; ++i)
					{
						const ScopedRenderTarget2D renderTarget((i == 0) ? additiveTarget : invMultiplyTarget);
						const Vec2 positionStart = {
							point.v * (kHighwayTextureSize.x - kLaserLineWidth) + kLaserLineWidth / 2,
							static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + ry - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY
						};
						const TiledTexture& startTexture = (laneIdx == 0) ? m_laserNoteLeftStartTexture : m_laserNoteRightStartTexture;
						startTexture(0, i).draw(Arg::topCenter = positionStart);
					}
				}

				const double positionStartY = static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + ry - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY;
				if (positionStartY < 0)
				{
					// Laser point is above the drawing area
					break;
				}

				// Draw laser slam
				if (point.v != point.vf)
				{
					const Vec2 positionStart = {
						point.v * (kHighwayTextureSize.x - kLaserLineWidth) + kLaserLineWidth / 2,
						static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + ry - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY
					};

					const Vec2 positionEnd = {
						point.vf * (kHighwayTextureSize.x - kLaserLineWidth) + kLaserLineWidth / 2,
						static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + ry - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY
					};

					const bool isLeftToRight = (point.v < point.vf);
					const Quad quad = LaserSlamLineQuad(positionStart, positionEnd);

					for (const auto& [renderTargetTexture, laserNoteTexture] : drawTexturePairs)
					{
						const ScopedRenderTarget2D renderTarget(renderTargetTexture);
						laserNoteTexture(kLaserTextureSize.x * laneIdx, 0, kLaserTextureSize).mirrored(isLeftToRight).drawAt(positionStart + Vec2{ 0.0, -kLaserLineWidth / 2 });
						laserNoteTexture(kLaserTextureSize.x * laneIdx, 0, kLaserTextureSize).mirrored(!isLeftToRight).flipped().drawAt(positionEnd + Vec2{ 0.0, -kLaserLineWidth / 2 });
						quad(laserNoteTexture(kLaserTextureSize.x * laneIdx + kOnePixelTextureSourceOffset, 0, kOnePixelTextureSourceSize, kLaserTextureSize.y)).draw();
					}
				}

				// Last laser point does not create laser line
				const auto nextItr = std::next(itr);
				if (nextItr == laserSection.v.end())
				{
					// If the final point is a laser slam, draw its extension
					if (point.v != point.vf)
					{
						const Vec2 positionStart = {
							point.vf * (kHighwayTextureSize.x - kLaserLineWidth) + kLaserLineWidth / 2,
							static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + ry - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY
						};
						const Quad quad = LaserLineQuad(positionStart + Vec2{ 0.0, -kLaserTextureSize.y }, positionStart + Vec2{ 0.0, -kLaserTextureSize.y - 80.0 });

						for (const auto& [renderTargetTexture, laserNoteTexture] : drawTexturePairs)
						{
							const ScopedRenderTarget2D renderTarget(renderTargetTexture);
							quad(laserNoteTexture(kLaserTextureSize.x * laneIdx, kLaserTextureSize.y - 1 + kOnePixelTextureSourceOffset, kLaserTextureSize.x, kOnePixelTextureSourceSize)).draw();
						}
					}

					break;
				}

				// Draw laser line by two laser points
				{
					const auto& [nextRy, nextPoint] = *nextItr;
					if (y + nextRy < gameStatus.currentPulse - kson::kResolution)
					{
						continue;
					}

					const Vec2 positionStart = {
						point.vf * (kHighwayTextureSize.x - kLaserLineWidth) + kLaserLineWidth / 2,
						static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + ry - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY
					};

					const Vec2 positionEnd = {
						nextPoint.v * (kHighwayTextureSize.x - kLaserLineWidth) + kLaserLineWidth / 2,
						static_cast<double>(kHighwayTextureSize.y) - static_cast<double>(y + nextRy - gameStatus.currentPulse) * 480 / kson::kResolution + kLaserShiftY
					};

					const Quad quad = LaserLineQuad(positionStart + ((point.v != point.vf) ? Vec2{ 0.0, -kLaserTextureSize.y } : Vec2::Zero()), positionEnd);

					for (const auto& [renderTargetTexture, laserNoteTexture] : drawTexturePairs)
					{
						const ScopedRenderTarget2D renderTarget(renderTargetTexture);
						quad(laserNoteTexture(kLaserTextureSize.x * laneIdx, kLaserTextureSize.y - 1 + kOnePixelTextureSourceOffset, kLaserTextureSize.x, kOnePixelTextureSourceSize)).draw();
					}
				}
			}
		}
	}
}
