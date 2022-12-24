﻿#pragma once
#include "music_game/game_status.hpp"
#include "highway_render_texture.hpp"
#include "key_beam_graphics.hpp"
#include "note/button_note_graphics.hpp"
#include "note/laser_note_graphics.hpp"

namespace MusicGame::Graphics
{
	class Highway3DGraphics
	{
	private:
		const Texture m_shineEffectTexture;

		const HighwayRenderTexture m_renderTexture;

		ButtonNoteGraphics m_buttonNoteGraphics;
		LaserNoteGraphics m_laserNoteGraphics;

		KeyBeamGraphics m_keyBeamGraphics;

		MeshData m_meshData;
		DynamicMesh m_mesh;

	public:
		Highway3DGraphics();

		void draw2D(const kson::ChartData& chartData, const GameStatus& gameStatus) const;

		void draw3D(double tiltRadians) const;
	};
}