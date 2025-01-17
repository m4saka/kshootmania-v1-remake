﻿#pragma once
#include <algorithm>
#include <vector>
#include <type_traits>
#include <cassert>
#include <cstring>
#include <cstddef>
#include "math_utils.hpp"

namespace ksmaudio::AudioEffect::detail
{
    constexpr std::size_t kLinearBufferDeclickFrames = 12U;

    // Useful for audio recording
    template <typename T>
    class LinearBuffer
    {
        static_assert(std::is_arithmetic_v<T>,
            "Value type of LinearBuffer is required to be arithmetic");

    private:
        std::vector<T> m_buffer;

        std::size_t m_readCursorFrame = 0U;

        std::size_t m_writeCursorFrame = 0U;

        float m_currentFadeOutScale = 1.0f;

        const std::size_t m_numFrames;

        const std::size_t m_numChannels;

    public:
        explicit LinearBuffer(std::size_t size, std::size_t numChannels)
            : m_buffer(size, T{ 0 })
            , m_numFrames(numChannels == 0U ? 0U : size / numChannels)
            , m_numChannels(numChannels)
        {
            assert(m_numChannels > 0U);
            assert(size % m_numChannels == 0U);
            m_buffer.shrink_to_fit();
        }

        void write(const T* pData, std::size_t size)
        {
            assert(size % m_numChannels == 0U);

            if (m_numFrames == 0U)
            {
                // 初期化に失敗した場合は何もしない
                return;
            }

            const std::size_t frameSize = size / m_numChannels;
            const std::size_t numWriteFrames = (std::min)(frameSize, m_numFrames - m_writeCursorFrame);
            if (numWriteFrames == 0U)
            {
                return;
            }
            std::memcpy(&m_buffer[m_writeCursorFrame * m_numChannels], pData, sizeof(T) * numWriteFrames * m_numChannels);
            m_writeCursorFrame += numWriteFrames;
        }

        void read(T* pData, std::size_t size, std::size_t numLoopFrames, std::size_t numNonZeroFrames, bool fadesOut = false, float fadeOutFeedbackLevel = 1.0f, float mix = 1.0f, bool bypass = false)
        {
            assert(size % m_numChannels == 0U);

            const std::size_t frameSize = size / m_numChannels;

            if (m_numFrames == 0U)
            {
                // 初期化に失敗した場合は何もしない
                return;
            }

            if (numLoopFrames == 0U || numLoopFrames > m_numFrames)
            {
                // numLoopFramesが処理できない値で来る場合がある(Retriggerのwave_lengthが未設定の時など)が、その場合も再生タイミングがずれないよう読み出しカーソルだけは進める必要がある
                m_readCursorFrame += frameSize; // ※m_readCursorFrameは使用時に剰余を使うので上限なしで問題ない
                return;
            }

            if (bypass || mix == 0.0f)
            {
                // bypass時でも再生タイミングがずれないよう読み出しカーソルだけは進める必要がある
                const std::size_t loopTimingCount = (m_readCursorFrame + frameSize) / numLoopFrames - m_readCursorFrame / numLoopFrames; // 通り越したループタイミングの個数(通常は0か1だが、frameSizeがnumLoopFramesより大きい場合も考慮して個数にしてある)
                m_readCursorFrame += frameSize; // ※m_readCursorFrameは使用時に剰余を使うので上限なしで問題ない
                for (std::size_t i = 0U; i < loopTimingCount; ++i)
                {
                    m_currentFadeOutScale *= fadeOutFeedbackLevel;
                }
                return;
            }

            if (numNonZeroFrames > numLoopFrames) [[unlikely]]
            {
                numNonZeroFrames = numLoopFrames;
            }

            const bool canDeclick = numNonZeroFrames > kLinearBufferDeclickFrames && numNonZeroFrames != numLoopFrames;
            for (std::size_t frameIdx = 0U; frameIdx < frameSize; ++frameIdx)
            {
                const std::size_t readCursorFrameWithLoop = m_readCursorFrame % numLoopFrames;

                for (std::size_t ch = 0U; ch < m_numChannels; ++ch)
                {
                    float wet;
                    if (canDeclick && readCursorFrameWithLoop < kLinearBufferDeclickFrames)
                    {
                        // Declick (start)
                        const float rate = static_cast<float>(readCursorFrameWithLoop + 1U) / (kLinearBufferDeclickFrames + 1U);
                        wet = m_buffer[readCursorFrameWithLoop * m_numChannels + ch] * rate;
                    }
                    else if (readCursorFrameWithLoop < numNonZeroFrames)
                    {
                        wet = m_buffer[readCursorFrameWithLoop * m_numChannels + ch];
                    }
                    else if (canDeclick && readCursorFrameWithLoop < numNonZeroFrames + kLinearBufferDeclickFrames)
                    {
                        // Declick (end)
                        const float rate = static_cast<float>(kLinearBufferDeclickFrames - (readCursorFrameWithLoop - numNonZeroFrames)) / (kLinearBufferDeclickFrames + 1U);
                        wet = m_buffer[readCursorFrameWithLoop * m_numChannels + ch] * rate;
                    }
                    else
                    {
                        wet = T{ 0 };
                    }

                    if (fadesOut)
                    {
                        const float fadeOutScale = static_cast<float>(numLoopFrames - readCursorFrameWithLoop) / numLoopFrames * m_currentFadeOutScale;
                        wet *= fadeOutScale;
                    }

                    *pData = std::lerp(*pData, wet, mix);
                    ++pData;
                }
                ++m_readCursorFrame;

                if (m_readCursorFrame % numLoopFrames == 0U)
                {
                    m_currentFadeOutScale *= fadeOutFeedbackLevel;
                }
            }
        }

        void read(T* pData, std::size_t size, std::size_t numLoopFrames)
        {
            read(pData, size, numLoopFrames, numLoopFrames);
        }

        void read(T* pData, std::size_t size)
        {
            assert(size % m_numChannels == 0U);

            if (m_numFrames == 0U)
            {
                // 初期化に失敗した場合は何もしない
                return;
            }

            const std::size_t frameSize = size / m_numChannels;
            if (m_writeCursorFrame <= m_readCursorFrame)
            {
                std::memset(pData, 0, sizeof(T) * frameSize * m_numChannels); // HACK: This assumes IEEE 754
            }
            else if (m_writeCursorFrame - m_readCursorFrame < frameSize)
            {
                const std::size_t numReadFrames = m_numFrames - m_readCursorFrame;
                std::memcpy(pData, &m_buffer[m_readCursorFrame * m_numChannels], sizeof(T) * numReadFrames * m_numChannels);
                std::memset(pData + numReadFrames * m_numChannels, 0, sizeof(T) * (frameSize - numReadFrames) * m_numChannels); // HACK: This assumes IEEE 754
                m_readCursorFrame += numReadFrames;
            }
            else
            {
                std::memcpy(pData, &m_buffer[m_readCursorFrame * m_numChannels], sizeof(T) * frameSize * m_numChannels);
                m_readCursorFrame += frameSize;
            }
        }

        void resetFadeOutScale()
        {
            m_currentFadeOutScale = 1.0f;
        }

        void resetReadCursor()
        {
            m_readCursorFrame = 0U;
            resetFadeOutScale();
        }

        void resetReadWriteCursors()
        {
            m_readCursorFrame = 0U;
            m_writeCursorFrame = 0U;
            resetFadeOutScale();
        }

        std::size_t size() const
        {
            return m_buffer.size();
        }

        std::size_t numFrames() const
        {
            return m_numFrames;
        }

        std::vector<T>& buffer()
        {
            return m_buffer;
        }

        const std::vector<T>& buffer() const
        {
            return m_buffer;
        }
    };
}
