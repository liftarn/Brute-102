#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BruteLookAndFeel.h"

namespace brute102
{
    class BruteLogoComponent : public juce::Component
    {
    public:
        BruteLogoComponent()
        {
            setInterceptsMouseClicks(false, false);
        }

        void paint(juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            const float scale = juce::jmin(bounds.getWidth() / 168.0f, bounds.getHeight() / 30.0f);
            const float ox = bounds.getX();
            const float oy = bounds.getCentreY() - 15.0f * scale;

            g.saveState();
            g.addTransform(juce::AffineTransform::scale(scale).translated(ox, oy));

            // Chamfered machined chassis badge
            juce::Path badge;
            badge.startNewSubPath(5.0f, 1.5f);
            badge.lineTo(166.5f, 1.5f);
            badge.lineTo(166.5f, 24.5f);
            badge.lineTo(162.5f, 28.5f);
            badge.lineTo(1.5f, 28.5f);
            badge.lineTo(1.5f, 5.5f);
            badge.closeSubPath();

            g.setColour(Colors::recess);
            g.fillPath(badge);
            g.setColour(juce::Colour(0xff2c313b));
            g.strokePath(badge, juce::PathStrokeType(1.0f));

            g.setColour(juce::Colour(0x733f4654));
            g.drawLine(6.0f, 2.5f, 165.0f, 2.5f, 0.75f);

            // Left emblem frame (Hybrid Saw+FM Icon)
            juce::Path iconFrame;
            iconFrame.startNewSubPath(8.0f, 5.5f);
            iconFrame.lineTo(28.5f, 5.5f);
            iconFrame.lineTo(28.5f, 22.0f);
            iconFrame.lineTo(26.0f, 24.5f);
            iconFrame.lineTo(5.5f, 24.5f);
            iconFrame.lineTo(5.5f, 8.0f);
            iconFrame.closeSubPath();

            g.setColour(juce::Colour(0xff14171d));
            g.fillPath(iconFrame);
            g.setColour(Colors::amber);
            g.strokePath(iconFrame, juce::PathStrokeType(1.2f));

            // Saw + Square + FM Spike waveform
            juce::Path wave;
            wave.startNewSubPath(8.0f, 19.5f);
            wave.lineTo(14.0f, 9.5f);
            wave.lineTo(14.0f, 20.0f);
            wave.lineTo(18.0f, 20.0f);
            wave.lineTo(18.0f, 10.0f);
            wave.lineTo(21.0f, 10.0f);
            wave.lineTo(22.8f, 19.5f);
            wave.lineTo(24.8f, 12.5f);
            wave.lineTo(26.0f, 15.0f);
            g.setColour(Colors::textPrimary);
            g.strokePath(wave, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::square));

            // Vertical divider
            g.setColour(Colors::border);
            g.drawLine(33.5f, 6.5f, 33.5f, 23.5f, 1.0f);

            // Custom geometric "BRUTE" stencil wordmark
            juce::Path wordmark;
            wordmark.setUsingNonZeroWinding(false);

            // B
            {
                juce::Path bOuter;
                bOuter.startNewSubPath(39.0f, 8.5f);
                bOuter.lineTo(47.8f, 8.5f);
                bOuter.lineTo(49.5f, 10.2f);
                bOuter.lineTo(49.5f, 13.8f);
                bOuter.lineTo(48.2f, 14.9f);
                bOuter.lineTo(49.8f, 16.2f);
                bOuter.lineTo(49.8f, 19.8f);
                bOuter.lineTo(48.0f, 21.5f);
                bOuter.lineTo(39.0f, 21.5f);
                bOuter.closeSubPath();
                bOuter.addRectangle(41.5f, 10.8f, 5.3f, 3.0f);
                bOuter.addRectangle(41.5f, 16.0f, 5.5f, 3.2f);
                wordmark.addPath(bOuter);
            }

            // R
            {
                juce::Path rPath;
                rPath.startNewSubPath(52.8f, 8.5f);
                rPath.lineTo(61.5f, 8.5f);
                rPath.lineTo(63.3f, 10.3f);
                rPath.lineTo(63.3f, 14.3f);
                rPath.lineTo(61.5f, 16.0f);
                rPath.lineTo(60.4f, 16.0f);
                rPath.lineTo(63.5f, 21.5f);
                rPath.lineTo(60.6f, 21.5f);
                rPath.lineTo(57.7f, 16.0f);
                rPath.lineTo(55.3f, 16.0f);
                rPath.lineTo(55.3f, 21.5f);
                rPath.lineTo(52.8f, 21.5f);
                rPath.closeSubPath();
                rPath.addRectangle(55.3f, 10.8f, 5.3f, 3.0f);
                wordmark.addPath(rPath);
            }

            // U
            {
                juce::Path uPath;
                uPath.startNewSubPath(66.6f, 8.5f);
                uPath.lineTo(69.1f, 8.5f);
                uPath.lineTo(69.1f, 19.1f);
                uPath.lineTo(74.5f, 19.1f);
                uPath.lineTo(74.5f, 8.5f);
                uPath.lineTo(77.0f, 8.5f);
                uPath.lineTo(77.0f, 19.8f);
                uPath.lineTo(75.3f, 21.5f);
                uPath.lineTo(68.3f, 21.5f);
                uPath.lineTo(66.6f, 19.8f);
                uPath.closeSubPath();
                wordmark.addPath(uPath);
            }

            // T
            {
                juce::Path tPath;
                tPath.startNewSubPath(79.8f, 8.5f);
                tPath.lineTo(90.4f, 8.5f);
                tPath.lineTo(90.4f, 10.9f);
                tPath.lineTo(86.35f, 10.9f);
                tPath.lineTo(86.35f, 21.5f);
                tPath.lineTo(83.85f, 21.5f);
                tPath.lineTo(83.85f, 10.9f);
                tPath.lineTo(79.8f, 10.9f);
                tPath.closeSubPath();
                wordmark.addPath(tPath);
            }

            // E
            {
                juce::Path ePath;
                ePath.startNewSubPath(93.2f, 8.5f);
                ePath.lineTo(103.0f, 8.5f);
                ePath.lineTo(103.0f, 10.8f);
                ePath.lineTo(95.7f, 10.8f);
                ePath.lineTo(95.7f, 13.8f);
                ePath.lineTo(101.4f, 13.8f);
                ePath.lineTo(101.4f, 16.0f);
                ePath.lineTo(95.7f, 16.0f);
                ePath.lineTo(95.7f, 19.2f);
                ePath.lineTo(103.0f, 19.2f);
                ePath.lineTo(103.0f, 21.5f);
                ePath.lineTo(93.2f, 21.5f);
                ePath.closeSubPath();
                wordmark.addPath(ePath);
            }

            g.setColour(Colors::textPrimary);
            g.fillPath(wordmark);

            // Amber "102" hardware plate
            juce::Path plate;
            plate.startNewSubPath(111.5f, 5.5f);
            plate.lineTo(162.5f, 5.5f);
            plate.lineTo(162.5f, 21.5f);
            plate.lineTo(159.5f, 24.5f);
            plate.lineTo(108.5f, 24.5f);
            plate.lineTo(108.5f, 8.5f);
            plate.closeSubPath();

            g.setColour(Colors::amber);
            g.fillPath(plate);

            // Stencil "102"
            juce::Path numPath;
            numPath.setUsingNonZeroWinding(false);

            // 1
            {
                juce::Path one;
                one.startNewSubPath(116.5f, 11.2f);
                one.lineTo(119.3f, 9.0f);
                one.lineTo(121.7f, 9.0f);
                one.lineTo(121.7f, 21.0f);
                one.lineTo(119.2f, 21.0f);
                one.lineTo(119.2f, 11.7f);
                one.lineTo(116.5f, 13.3f);
                one.closeSubPath();
                numPath.addPath(one);
            }

            // 0
            {
                juce::Path zero;
                zero.startNewSubPath(127.6f, 9.0f);
                zero.lineTo(134.8f, 9.0f);
                zero.lineTo(136.5f, 10.7f);
                zero.lineTo(136.5f, 19.3f);
                zero.lineTo(134.8f, 21.0f);
                zero.lineTo(127.6f, 21.0f);
                zero.lineTo(125.9f, 19.3f);
                zero.lineTo(125.9f, 10.7f);
                zero.closeSubPath();
                zero.addRectangle(128.3f, 11.2f, 5.8f, 7.6f);
                numPath.addPath(zero);
            }

            // 2
            {
                juce::Path two;
                two.startNewSubPath(140.5f, 9.0f);
                two.lineTo(149.2f, 9.0f);
                two.lineTo(150.8f, 10.6f);
                two.lineTo(150.8f, 14.5f);
                two.lineTo(149.2f, 16.0f);
                two.lineTo(143.0f, 16.0f);
                two.lineTo(143.0f, 18.8f);
                two.lineTo(150.8f, 18.8f);
                two.lineTo(150.8f, 21.0f);
                two.lineTo(140.5f, 21.0f);
                two.lineTo(140.5f, 14.5f);
                two.lineTo(142.1f, 13.8f);
                two.lineTo(148.3f, 13.8f);
                two.lineTo(148.3f, 11.2f);
                two.lineTo(140.5f, 11.2f);
                two.closeSubPath();
                numPath.addPath(two);
            }

            g.setColour(Colors::background);
            g.fillPath(numPath);
            g.drawLine(128.8f, 18.2f, 133.6f, 11.8f, 1.6f);

            g.restoreState();
        }
    };
}
