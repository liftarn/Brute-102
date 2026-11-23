#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>
#include "BruteLookAndFeel.h"

namespace brute102
{
    class LevelMeterComponent : public juce::Component
    {
    public:
        static constexpr int kSegmentCount = 14;

        void updatePeak(float linearPeak)
        {
            const float db = linearPeak > 1e-5f ? 20.0f * std::log10(linearPeak) : -72.0f;
            const float clampedDb = juce::jlimit(-60.0f, 0.0f, db);
            const float norm = (clampedDb + 60.0f) / 60.0f;

            peakNorm = norm;
            const auto nowMs = juce::Time::getMillisecondCounter();
            if (norm >= holdValue)
            {
                holdValue = norm;
                holdExpiryMs = nowMs + 700;
            }
            else if (nowMs > holdExpiryMs)
            {
                holdValue = std::max(0.0f, holdValue - 0.025f);
            }

            if (linearPeak > 1e-4f)
                dbText = db >= -0.05f ? "0.0" : juce::String(db, 1);
            else
                dbText = "-inf";

            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds();

            g.setColour(Colors::textMuted);
            g.setFont(juce::Font(juce::FontOptions(8.5f)));
            g.drawText("OUT", bounds.getX(), bounds.getY(), bounds.getWidth(), 11, juce::Justification::centred);

            const auto ledArea = bounds.withTrimmedTop(13).withTrimmedBottom(14).reduced(6, 0);
            g.setColour(Colors::recess);
            g.fillRoundedRectangle(ledArea.toFloat(), 2.0f);
            g.setColour(Colors::border);
            g.drawRoundedRectangle(ledArea.toFloat(), 2.0f, 1.0f);

            const int litCount = juce::roundToInt(peakNorm * static_cast<float>(kSegmentCount));
            const int holdIdx = holdValue > 0.02f
                ? std::min(kSegmentCount - 1, static_cast<int>(holdValue * static_cast<float>(kSegmentCount)))
                : -1;

            const auto inner = ledArea.reduced(3, 3);
            const float segGap = 2.0f;
            const float segHeight = (static_cast<float>(inner.getHeight()) - segGap * (kSegmentCount - 1))
                                  / static_cast<float>(kSegmentCount);

            for (int idx = 0; idx < kSegmentCount; ++idx)
            {
                const bool isLit = idx < litCount || idx == holdIdx;
                juce::Colour col(0xff232730);
                if (isLit)
                {
                    if (idx >= kSegmentCount - 2)      col = juce::Colour(0xffef4444);
                    else if (idx >= kSegmentCount - 5) col = Colors::amber;
                    else                               col = juce::Colour(0xff34d399);
                }

                const float y = static_cast<float>(inner.getBottom())
                              - static_cast<float>(idx + 1) * segHeight
                              - static_cast<float>(idx) * segGap;
                g.setColour(col);
                g.fillRoundedRectangle(static_cast<float>(inner.getX()), y,
                                       static_cast<float>(inner.getWidth()), std::max(2.0f, segHeight), 1.0f);
            }

            // Fixed-width centered dBFS readout prevents any layout size shifts
            g.setColour(Colors::textMuted.brighter(0.25f));
            g.setFont(juce::Font(juce::FontOptions(9.0f)));
            g.drawText(dbText, bounds.getX(), bounds.getBottom() - 12, bounds.getWidth(), 12,
                       juce::Justification::centred, false);
        }

    private:
        float peakNorm = 0.0f;
        float holdValue = 0.0f;
        juce::uint32 holdExpiryMs = 0;
        juce::String dbText { "-inf" };
    };
}
