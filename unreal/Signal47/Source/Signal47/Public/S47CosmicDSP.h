#pragma once
// Original, engine-independent synthesis kernel. No models, samples or network access.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace S47Cosmic
{
struct State
{
    double Level = 0, Wonder = 0, Mystery = 0;
};

class Synth
{
public:
    explicit Synth(double SampleRate = 48000, uint32_t Seed = 4701986)
        : Rate(std::isfinite(SampleRate) ? std::clamp(SampleRate, 8000., 192000.) : 48000.), Rng(Seed ? Seed : 1),
          DelayL(size_t(Rate * .719), 0), DelayR(size_t(Rate * .937), 0)
    { Attack = 1 - std::exp(-1 / (Rate * 7.)); Release = 1 - std::exp(-1 / (Rate * .18));
      HoldRelease = 1 - std::exp(-1 / (Rate * .025)); Colour = 1 - std::exp(-1 / (Rate * 4.)); }

    void Set(State S)
    {
        auto Safe = [](double X) { return std::isfinite(X) ? std::clamp(X, 0., 1.) : 0.; };
        Target = { Safe(S.Level), Safe(S.Wonder), Safe(S.Mystery) };
    }
    void Pause(bool On) { Frozen = On; }
    bool Paused() const { return Frozen; }
    double Seconds() const { return double(Frames) / Rate; }
    double Gain() const { return Envelope; }
    // Detail is an optional pre-cleared reference recording. It shares every hold/fade.
    void Next(float& Left, float& Right, double DetailL = 0, double DetailR = 0)
    {
        Left = Right = 0;
        if (Frozen) return; // No timeline, random source, delay or reference transport advances.
        const double Fade = Target.Level == 0 ? HoldRelease : Target.Level < Envelope ? Release : Attack;
        Envelope += (Target.Level - Envelope) * Fade;
        Wonder += (Target.Wonder - Wonder) * Colour;
        Mystery += (Target.Mystery - Mystery) * Colour;
        if (Target.Level == 0 && Envelope < 1e-7) Envelope = 0;
        // Slowly moving broad bands, not tuned notes or the gameplay's four/seven pulses.
        if ((Frames & 127) == 0)
        {
            const double T = Seconds();
            Breath = .64 + .16 * std::sin(T * .103) + .12 * std::sin(T * .071 + 1.7);
            const double Cutoff = 210 + 390 * Wonder + 140 * std::sin(T * .047);
            LowAlpha = 1 - std::exp(-6.283185307179586 * 65 / Rate);
            MidAlpha = 1 - std::exp(-6.283185307179586 * Cutoff / Rate);
            HighAlpha = 1 - std::exp(-6.283185307179586 * 1900 / Rate);
            Pan = .36 * std::sin(T * .037);
            Cloud = std::pow(std::max(0., std::sin(T * .151 + .6) * std::sin(T * .089)), 2.);
        }
        const double N = Noise();
        Low += LowAlpha * (N - Low);
        Mid += MidAlpha * (N - Mid);
        High += HighAlpha * (N - High);
        // Gains compensate filtered-noise energy. Long unequal echoes create width.
        const double Horizon = Low * 3.1 * (1 - .25 * Mystery);
        const double Plasma = (Mid - Low) * (1.4 + Mystery) * Breath;
        const double Dust = (High - Mid) * (.12 + .25 * Wonder) * Cloud;
        const double Dry = .12 * (Horizon + Plasma + Dust);
        const double EchoL = DelayL[IndexL], EchoR = DelayR[IndexR];
        DelayL[IndexL] = float(Dry + .48 * EchoR);
        DelayR[IndexR] = float(Dry + .44 * EchoL);
        IndexL = (IndexL + 1) % DelayL.size(); IndexR = (IndexR + 1) % DelayR.size();
        const double L = (Dry * (.8 - Pan) + .55 * EchoL) * .32 + SafeDetail(DetailL) * .012;
        const double R = (Dry * (.8 + Pan) + .55 * EchoR) * .32 + SafeDetail(DetailR) * .012;
        Left = float(Envelope * .18 * std::tanh(L / .18));
        Right = float(Envelope * .18 * std::tanh(R / .18));
        ++Frames;
    }
private:
    static double SafeDetail(double X) { return std::isfinite(X) ? std::clamp(X, -1., 1.) : 0.; }
    double Noise() { Rng ^= Rng << 13; Rng ^= Rng >> 17; Rng ^= Rng << 5; return double(Rng) / 2147483648. - 1.; }
    double Rate;
    double Attack = 0, Release = 0, HoldRelease = 0, Colour = 0;
    uint32_t Rng;
    State Target;
    bool Frozen = false;
    uint64_t Frames = 0;
    double Envelope = 0, Wonder = 0, Mystery = 0, Low = 0, Mid = 0, High = 0;
    double LowAlpha = 0, MidAlpha = 0, HighAlpha = 0, Breath = 0, Pan = 0, Cloud = 0;
    std::vector<float> DelayL, DelayR;
    size_t IndexL = 0, IndexR = 0;
};
}
