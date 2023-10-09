﻿#pragma once

enum DifficultyIdx : int32
{
	kDifficultyIdxLight = 0,
	kDifficultyIdxChallenge,
	kDifficultyIdxExtended,
	kDifficultyIdxInfinite,

	kNumDifficulties,
};

constexpr int32 kLevelMax = 20;

inline constexpr StringView kKSHExtension = U"ksh";
inline constexpr StringView kKSONExtension = U"kson";

constexpr double kPastTimeSec = -100000.0;

enum class Medal : int32
{
	kNoMedal = 0,
	kPlayed,
	kEasyClear,
	kEasyFullCombo,
	kEasyPerfect,
	kClear,
	kHardClear,
	kFullCombo,
	kPerfect,
};

enum class Achievement : int32
{
	kFailed,
	kCleared,
	kFullCombo,
	kPerfect,
};

enum class Grade : int32
{
	kNoGrade = 0,
	kD,
	kC,
	kB,
	kA,
	kAA,
	kAAA,
};
