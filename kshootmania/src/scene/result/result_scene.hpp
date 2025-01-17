﻿#pragma once
#include <CoTaskLib.hpp>
#include "result_assets.hpp"
#include "result_panel.hpp"
#include "result_scene_args.hpp"
#include "music_game/play_result.hpp"
#include "ksmaudio/ksmaudio.hpp"

class ResultScene : public Co::SceneBase
{
private:
	ksmaudio::Stream m_bgmStream{ "se/result_bgm.ogg", 1.0, false, false, true };

	const Texture m_bgTexture{ TextureAsset(ResultTexture::kBG) };

	const kson::ChartData m_chartData;

	const MusicGame::PlayResult m_playResult;

	ResultPanel m_resultPanel;

public:
	explicit ResultScene(const ResultSceneArgs& args);

	virtual ~ResultScene() = default;

	virtual Co::Task<void> start() override;

	virtual void draw() const override;

	virtual Co::Task<void> fadeIn() override;

	virtual Co::Task<void> postFadeOut() override;
};
