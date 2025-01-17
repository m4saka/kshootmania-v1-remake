﻿#pragma once
#include "ui/linear_menu.hpp"
#include "graphics/tiled_texture.hpp"

class OptionMenuField
{
private:
	const String m_configIniKey;

	bool m_isEnum;

	// 数値オプション用
	const String m_suffixStr;

	// 選択肢オプション用
	const Array<std::pair<String, String>> m_valueDisplayNamePairs;

	const std::function<void()> m_onChangeCallback;

	LinearMenu m_menu;

	TextureRegion m_keyTextureRegion;

public:
	struct CreateInfo
	{
		String configIniKey;

		// 数値オプション用
		int32 valueMin = 0;
		int32 valueMax = 0;
		int32 valueDefault = 0;
		int32 valueStep = 0;
		String suffixStr;

		// 選択肢オプション用
		Array<std::pair<String, String>> valueDisplayNamePairs;

		static constexpr int32 kKeyTextureIdxAutoSet = -1;
		int32 keyTextureIdx = kKeyTextureIdxAutoSet;

		std::function<void()> onChangeCallback = nullptr;

		static CreateInfo Enum(StringView configIniKey, const Array<String>& valueDisplayNames);

		static CreateInfo Enum(StringView configIniKey, const Array<StringView>& valueDisplayNames);

		static CreateInfo Enum(StringView configIniKey, const Array<std::pair<String, String>>& valueDisplayNamePairs);

		static CreateInfo Enum(StringView configIniKey, const Array<std::pair<int, String>>& valueDisplayNamePairs);

		static CreateInfo Enum(StringView configIniKey, const Array<std::pair<double, String>>& valueDisplayNamePairs);

		static CreateInfo Int(StringView configIniKey, int32 valueMin = std::numeric_limits<int32>::min(), int32 valueMax = std::numeric_limits<int32>::max(), int32 valueDefault = 0, StringView suffixStr = U"", int32 valueStep = 1);

		CreateInfo& setKeyTextureIdx(int32 idx)&;

		CreateInfo&& setKeyTextureIdx(int32 idx)&&;

		CreateInfo& setOnChangeCallback(std::function<void()> callback)&;

		CreateInfo&& setOnChangeCallback(std::function<void()> callback)&&;
	};

	OptionMenuField(const TextureRegion& keyTextureRegion, const CreateInfo& createInfo);

	void update();

	void draw(const Vec2& position, const TiledTexture& valueTiledTexture, const Font& font) const;

	// 注意: これらの列挙子の値はテクスチャの順番にも使用されるので順番を入れ替えないこと
	enum ArrowType
	{
		kArrowTypeLeft = 0,
		kArrowTypeRight,
		kArrowTypeLeftRight,
		kArrowTypeNone,

		kArrowTypeEnumCount,
	};
};
