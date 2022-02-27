﻿#include "option_menu.hpp"
#include "option_scene.hpp"
#include "option_assets.hpp"

namespace
{
	constexpr int32 kMenuItemOffsetY = 40;
	constexpr int32 kMenuItemDiffY = 100;

	double MenuCursorAlphaValue(double sec, bool isSelected)
	{
		if (!isSelected)
		{
			return 100.0 / 256;
		}

		constexpr double baseValue = 184.0 / 256;
		constexpr double maxValue = 280.0 / 256;
		constexpr double periodSec = Math::TwoPi * 0.15;
		constexpr double secOffset = 1.0 / 0.15;
		return Min(baseValue + (maxValue - baseValue) * Periodic::Sine0_1(periodSec, sec + secOffset), 1.0);
	}
}

OptionMenu::OptionMenu(OptionScene* pOptionScene)
	: m_pOptionScene(pOptionScene)
	, m_menu(MenuHelper::MakeVerticalMenu(
		kItemEnumCount,
		MenuHelper::ButtonFlags::kArrow | MenuHelper::ButtonFlags::kBT | MenuHelper::ButtonFlags::kBTOpposite))
	, m_menuItemTextureAtlas(OptionTexture::kTopMenuItem, kItemEnumCount)
	, m_stopwatch(StartImmediately::Yes)
{
}

void OptionMenu::update()
{
	m_menu.update();
}

void OptionMenu::draw() const
{
	using namespace ScreenUtils;

	const int32 x = Scene::Center().x;

	// Draw menu items
	for (int32 i = 0; i < kItemEnumCount; ++i)
	{
		const int32 y = Scaled(kMenuItemOffsetY) + Scaled(kMenuItemDiffY) * i;
		const TextureRegion textureRegion = Scaled2x(m_menuItemTextureAtlas(i));
		const double alpha = MenuCursorAlphaValue(m_stopwatch.sF(), i == m_menu.cursor());
		textureRegion.draw(x - textureRegion.size.x / 2, y, ColorF{ 1.0, alpha });
	}
}
