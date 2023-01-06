﻿#pragma once
#include "music_game/scroll/hispeed_setting.hpp"

namespace MusicGame
{
	/// @brief ハイスピード設定のメニュー
	class HispeedSettingMenu
	{
	private:
		ArrayWithLinearMenu<HispeedType> m_typeMenu;
		LinearMenu m_valueMenu;

		void refreshValueMenuConstraints();

		void setHispeedSetting(const HispeedSetting& hispeedSetting);

	public:
		/// @brief コンストラクタ
		/// @remarks 内部でConfigIniを参照するため、ConfigIniが初期化済みである必要がある
		HispeedSettingMenu();

		/// @brief 毎フレームの更新
		void update();

		/// @brief ConfigIniから読み込んでメニューに反映
		/// @remarks ハイスピード設定の表示/非表示設定に更新があっても反映されない点に注意
		void loadFromConfigIni();

		/// @brief ConfigIniへメニューの状態を保存
		void saveToConfigIni() const;

		/// @brief 現在の値をHispeedSetting構造体として取得
		/// @return HispeedSetting構造体
		HispeedSetting hispeedSetting() const;
	};
}