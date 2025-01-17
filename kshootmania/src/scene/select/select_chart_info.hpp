﻿#pragma once
#include "kson/chart_data.hpp"
#include "high_score/high_score_info.hpp"

class SelectChartInfo
{
private:
	FilePath m_chartFilePath;

	kson::MetaChartData m_chartData;

	HighScoreInfo m_highScoreInfo;

	FilePath toFullPath(const std::string& u8Filename) const;

public:
	explicit SelectChartInfo(FilePathView chartFilePath);

	String title() const;

	String artist() const;

	FilePath jacketFilePath() const;

	String jacketAuthor() const;

	FilePathView chartFilePath() const;

	String chartAuthor() const;

	int32 difficultyIdx() const;

	int32 level() const;

	FilePath previewBGMFilePath() const;

	SecondsF previewBGMOffset() const;

	Duration previewBGMDuration() const;

	double previewBGMVolume() const;

	FilePath iconFilePath() const;

	String information() const;

	const HighScoreInfo& highScoreInfo() const;

	bool hasError() const;

	String errorString() const;
};
