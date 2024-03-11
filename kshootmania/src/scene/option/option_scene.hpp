﻿#pragma once
#include "option_top_menu.hpp"
#include "option_menu.hpp"
#include "option_key_config_menu.hpp"
#include "ksmaudio/ksmaudio.hpp"

class OptionScene : public MyScene
{
public:
	// TODO: OptionTopMenu::Itemと統一
	enum OptionMenuType : int32
	{
		kDisplaySound = 0,
		kInputJudgment,
		kOther,
		kKeyConfig,

		kOptionMenuTypeEnumCount,
	};

private:
	const Texture m_bgTexture;

	TiledTexture m_headerTiledTexture;

	OptionTopMenu m_topMenu;

	std::array<OptionMenu, kOptionMenuTypeEnumCount> m_optionMenus;

	OptionKeyConfigMenu m_keyConfigMenu;

	const Font m_font = AssetManagement::SystemFont();

	Optional<OptionMenuType> m_currentOptionMenuIdx = none;

	ksmaudio::Stream m_bgmStream{ "se/option_bgm.ogg", 1.0, false, false, true };

public:
	explicit OptionScene(const InitData& initData);

	virtual void update() override;

	virtual void updateFadeIn([[maybe_unused]] double t) { update(); }

	virtual void draw() const override;

	void exitScene();
};
