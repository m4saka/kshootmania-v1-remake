#pragma once
#include "kson/common/common.hpp"
#include "kson/beat/beat_info.hpp"

namespace kson
{
	struct TimingCache
	{
		std::map<Pulse, Ms> bpmChangeMs;
		std::map<Ms, Pulse> bpmChangePulse;
		std::map<std::int64_t, Pulse> timeSigChangePulse;
		std::map<Pulse, std::int64_t> timeSigChangeMeasureIdx;
	};

	namespace TimingUtils
	{
		Pulse TimeSigMeasurePulse(const TimeSig& timeSig);

		TimingCache CreateTimingCache(const BeatInfo& beatInfo);

		Ms PulseToMs(Pulse pulse, const BeatInfo& beatInfo, const TimingCache& cache);
		double PulseToSec(Pulse pulse, const BeatInfo& beatInfo, const TimingCache& cache);

		Pulse MsToPulse(Ms ms, const BeatInfo& beatInfo, const TimingCache& cache);
		Pulse SecToPulse(double sec, const BeatInfo& beatInfo, const TimingCache& cache);

		std::int64_t PulseToMeasureIdx(Pulse pulse, const BeatInfo& beatInfo, const TimingCache& cache);

		std::int64_t MsToMeasureIdx(Ms ms, const BeatInfo& beatInfo, const TimingCache& cache);
		std::int64_t SecToMeasureIdx(double sec, const BeatInfo& beatInfo, const TimingCache& cache);

		Pulse MeasureIdxToPulse(std::int64_t measureIdx, const BeatInfo& beatInfo, const TimingCache& cache);

		Pulse MeasureValueToPulse(double measureValue, const BeatInfo& beatInfo, const TimingCache& cache);

		Ms MeasureIdxToMs(std::int64_t measureIdx, const BeatInfo& beatInfo, const TimingCache& cache);
		double MeasureIdxToSec(std::int64_t measureIdx, const BeatInfo& beatInfo, const TimingCache& cache);

		Ms MeasureValueToMs(double measureValue, const BeatInfo& beatInfo, const TimingCache& cache);
		double MeasureValueToSec(double measureValue, const BeatInfo& beatInfo, const TimingCache& cache);

		bool IsPulseBarLine(Pulse pulse, const BeatInfo& beatInfo, const TimingCache& cache);

		double PulseTempo(Pulse pulse, const BeatInfo& beatInfo);

		const TimeSig& PulseTimeSig(Pulse pulse, const BeatInfo& beatInfo, const TimingCache& cache);
	};
}
