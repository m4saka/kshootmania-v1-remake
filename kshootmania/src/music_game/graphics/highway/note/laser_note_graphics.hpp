﻿#pragma once
#include "music_game/game_status.hpp"
#include "music_game/scroll/highway_scroll.hpp"
#include "music_game/graphics/highway/highway_render_texture.hpp"

namespace MusicGame::Graphics
{
	class LaserNoteGraphics
	{
	private:
		const Texture m_laserNoteTexture;
		const Texture m_laserNoteMaskTexture;
		const std::array<TiledTexture, kson::kNumLaserLanesSZ> m_laserNoteStartTextures;

	public:
		LaserNoteGraphics();

		void draw(const kson::ChartData& chartData, const GameStatus& gameStatus, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;
	};
}
