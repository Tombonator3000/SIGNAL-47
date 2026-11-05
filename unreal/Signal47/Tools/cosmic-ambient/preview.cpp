// Compile/run with preview.py. Tests the exact kernel used by the Unreal node.
#include "S47CosmicDSP.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

static void Check(bool Ok, const char* Name)
{
    if (!Ok) throw std::runtime_error(Name);
    std::cout << "PASS " << Name << '\n';
}
static void Tests()
{
    for (double Rate : {8000., 44100., 48000.})
    {
        S47Cosmic::Synth S(Rate), Same(Rate);
        S.Set({1, .8, .6}); Same.Set({1, .8, .6});
        double Energy = 0, Width = 0, Peak = 0, Jump = 0; float Previous = 0;
        bool Finite = true, Deterministic = true;
        for (int I = 0; I < int(Rate * 12); ++I)
        {
            float L, R, L2, R2; S.Next(L, R); Same.Next(L2, R2);
            Deterministic &= L == L2 && R == R2;
            Finite &= std::isfinite(L) && std::isfinite(R);
            Energy += L * L + R * R; Width += (L - R) * (L - R);
            Peak = std::max({Peak, double(std::abs(L)), double(std::abs(R))});
            Jump = std::max(Jump, double(std::abs(L - Previous))); Previous = L;
        }
        Check(Finite && Energy > .001 && Width > .001 && Peak < .18, "finite, audible, bounded stereo at 8/44.1/48 kHz");
        Check(Deterministic, "same seed renders sample-identical audio");
        std::cout << "rate=" << Rate << " peak=" << Peak << " jump=" << Jump << "\n";
        Check(Jump < .08, "sample steps remain below eight percent full scale");
        const double At = S.Seconds(), Gain = S.Gain();
        S.Pause(true);
        for (int I = 0; I < 5000; ++I) { float L, R; S.Next(L, R); if (L || R) throw std::runtime_error("pause silence"); }
        Check(S.Seconds() == At && S.Gain() == Gain, "pause emits silence and freezes synthesis transport");
        S.Pause(false); float L, R; S.Next(L, R);
        Check(S.Seconds() > At, "resume advances the retained transport");
        S.Set({0, 0, 0});
        for (int I = 0; I < int(Rate * .5); ++I) S.Next(L, R, 1, -1);
        Check(L == 0 && R == 0 && S.Gain() == 0, "listening hold silences texture and reference within half a second");
        S.Set({std::numeric_limits<double>::quiet_NaN(), 100, -100});
        S.Next(L, R, std::numeric_limits<double>::infinity(), 0);
        Check(L == 0 && R == 0, "non-finite controls and reference samples are contained");
    }
}
static void U16(std::ofstream& O, uint16_t X) { O.put(char(X)); O.put(char(X >> 8)); }
static void U32(std::ofstream& O, uint32_t X) { U16(O, uint16_t(X)); U16(O, uint16_t(X >> 16)); }
int main(int Argc, char** Argv)
{
    try
    {
        Tests();
        if (Argc < 2) return 0;
        constexpr int Rate = 48000, Seconds = 72;
        std::ofstream O(Argv[1], std::ios::binary);
        O.write("RIFF", 4); U32(O, 36 + Rate * Seconds * 4); O.write("WAVEfmt ", 8);
        U32(O, 16); U16(O, 1); U16(O, 2); U32(O, Rate); U32(O, Rate * 4); U16(O, 4); U16(O, 16);
        O.write("data", 4); U32(O, Rate * Seconds * 4);
        S47Cosmic::Synth S(Rate);
        double Energy = 0, Peak = 0;
        for (int I = 0; I < Rate * Seconds; ++I)
        {
            const double T = double(I) / Rate;
            if (I % 800 == 0)
                S.Set(T < 20 ? S47Cosmic::State{.55, .55, .2} : T < 40 ? S47Cosmic::State{.65, .3, .65}
                    : T < 60 ? S47Cosmic::State{.45, .9, .1} : S47Cosmic::State{});
            float L, R; S.Next(L, R);
            Peak = std::max({Peak, double(std::abs(L)), double(std::abs(R))}); Energy += L * L + R * R;
            U16(O, uint16_t(int16_t(L * 32767))); U16(O, uint16_t(int16_t(R * 32767)));
        }
        Check(O.good(), "72-second original stereo listening file written");
        std::cout << "peak=" << Peak << " rms=" << std::sqrt(Energy / (Rate * Seconds * 2)) << '\n';
    }
    catch (const std::exception& E) { std::cerr << "FAIL " << E.what() << '\n'; return 1; }
}
