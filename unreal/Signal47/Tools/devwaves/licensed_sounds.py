#!/usr/bin/env python3
"""DEV-WAVES: klipper lyder fra Toms kjøpte lydbibliotek til den lisensierte lydpakka.

Fila inneholder bare stier og ffmpeg-parametre, ingen lyd. Resultatet havner i
Content/Licensed/DevWaves/, som er git-ignorert; kjøpt lyd skal aldri inn i Git.
Navn som mangler her finnes bare i den syntetiske pakka (make_sounds.py), og
spillet bruker da den.

Bruk:  python3 licensed_sounds.py [utmappe]
Biblioteket: miljøvariabelen S47_ULTIMATE_SFX, ellers standardstien under.
Krever bare ffmpeg og Pythons standardbibliotek.
"""
import array
import math
import os
import shutil
import subprocess
import sys
import wave

DEFAULT_LIB = os.path.expanduser("~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle")
DEFAULT_OUT = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "Content", "Licensed", "DevWaves"))

SR = 44100
PEAK_DBFS = -1.0
HEADROOM = 0.5

MIX = "pan=mono|c0=0.5*c0+0.5*c1"   # vanlig nedmiks
LEFT = "pan=mono|c0=c0"             # bare venstre: kanalene er i motfase i kilden

G = "Guns/FL_GFX_Gun_Shot_"
UI = "Ui Clicks/FL_GFX_UI_Click_"

# Skuddet: ett nært enkeltskudd (14: skarpest anslag målt). Den åpne halen er fra 13,
# som har lengst naturlig utklang (40 dB ned etter 1,1 s). Hylsene er de små
# metallklirrene som ligger i halen på skudd 16, 15 og 07; høypass fjerner skuddet.
# Filteret kjøres på hele fila FØR klippet, så det ikke lager et klikk i starten.
#
# navn: (fil, pan, start s, lengde s, fade inn ms, fade ut ms, filter før klipp)
RECIPES = {
    "shot_transient":   (G + "14.wav", MIX, 0.000, 0.060, 1, 20,
                         "highpass=f=350,highpass=f=350"),
    # Kilden er hardt begrenset og har mest energi rundt 34 Hz (55 % under 80 Hz). Høypasset
    # flytter tyngden til 80-250 Hz (målt topp 211 Hz); sett filteret til None for full bunn.
    "shot_body":        (G + "14.wav", MIX, 0.000, 0.300, 1, 80,
                         "highpass=f=60,highpass=f=60"),
    "shot_tail_yard":   (G + "13.wav", MIX, 0.120, 1.380, 30, 400, "highpass=f=25"),   # ingen DC
    # tre tidlige ekko (45/95/160 ms) av samme skudd; volume holder aecho under klipping
    "shot_tail_walls":  (G + "14.wav", MIX, 0.000, 0.850, 1, 250,
                         "volume=0.4,highpass=f=180,"
                         "aecho=0.35:1:45|95|160:0.62|0.45|0.30"),
    "dry_click":        (G + "Safety_01.wav", LEFT, 0.1440, 0.080, 2, 30, None),
    "mag_out":          (G + "Reloaded_06.wav", MIX, 0.0256, 0.360, 3, 30, None),
    "mag_in":           (G + "Reloaded_05.wav", MIX, 0.0224, 0.375, 3, 40, None),
    "bolt_pull":        (G + "Reloaded_04.wav", MIX, 0.0070, 0.215, 3, 40, None),
    "bolt_release":     (G + "Reloaded_04.wav", MIX, 0.2970, 0.160, 3, 35, None),
    "casing_1":         (G + "16.wav", MIX, 0.7570, 0.345, 3, 60,
                         "highpass=f=3000,highpass=f=3000"),
    "casing_2":         (G + "15.wav", MIX, 0.4520, 0.300, 3, 60,
                         "highpass=f=2500,highpass=f=2500"),
    "casing_3":         (G + "07.wav", MIX, 0.9480, 0.240, 3, 80,
                         "highpass=f=2500,highpass=f=2500"),
    "player_hurt":      ("Medieval Fighting/FL_GFX_Medieval_Fighting_War_Club_04.wav",
                         MIX, 0.0860, 0.320, 3, 60, None),
    "hit_marker":       (UI + "11.wav", MIX, 0.0080, 0.036, 2, 8, None),
    "kill_marker":      (UI + "19.wav", MIX, 0.0055, 0.095, 2, 20, None),
}


def render(src, pan, start, dur, fade_in_ms, fade_out_ms, pre):
    """Kjører ffmpeg og gir tilbake mono-samples som flyttall."""
    fi = fade_in_ms / 1000.0
    fo = fade_out_ms / 1000.0
    # Flyttall og 6 dB luft: ellers regner ffmpeg i heltall og filtrene klipper toppene.
    chain = ["aformat=sample_fmts=fltp", pan, "volume=%.3f" % HEADROOM]
    if pre:
        chain.append(pre)
    chain += [
        "atrim=start=%.4f:duration=%.4f" % (start, dur),
        "asetpts=PTS-STARTPTS",
        "afade=t=in:st=0:d=%.4f" % fi,
        "afade=t=out:st=%.4f:d=%.4f" % (dur - fo, fo),
    ]
    cmd = ["ffmpeg", "-v", "error", "-nostdin", "-i", src,
           "-af", ",".join(chain), "-ar", str(SR), "-ac", "1",
           "-f", "f32le", "-"]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if res.returncode != 0:
        raise RuntimeError(res.stderr.decode("utf-8", "replace").strip())
    samples = array.array("f")
    samples.frombytes(res.stdout[:len(res.stdout) // 4 * 4])
    if sys.byteorder != "little":
        samples.byteswap()
    return samples[:int(round(dur * SR))]


def write_wav(path, samples):
    """Normaliserer toppen til PEAK_DBFS og skriver 16-bit mono PCM."""
    peak = max(max(samples), -min(samples)) if samples else 0.0
    if peak <= 0.0:
        raise RuntimeError("stille klipp")
    gain = (10.0 ** (PEAK_DBFS / 20.0)) / peak * 32767.0
    pcm = array.array("h", (max(-32768, min(32767, int(round(v * gain))))
                            for v in samples))
    pcm[0] = 0      # start og slutt på null: ingen klikk
    pcm[-1] = 0
    if sys.byteorder != "little":
        pcm.byteswap()
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    return 20.0 * math.log10(peak / HEADROOM)


def main():
    lib = os.environ.get("S47_ULTIMATE_SFX", DEFAULT_LIB)
    out = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUT
    if not os.path.isdir(lib):
        print("licensed_sounds: finner ikke lydbiblioteket (%s); hopper over, "
              "spillet bruker de syntetiske lydene." % lib)
        return 0
    if shutil.which("ffmpeg") is None:
        print("licensed_sounds: ffmpeg mangler; hopper over.")
        return 0
    os.makedirs(out, exist_ok=True)
    failed = 0
    for name, (rel, pan, start, dur, fi, fo, pre) in RECIPES.items():
        src = os.path.join(lib, rel)
        if not os.path.isfile(src):
            print("  %-16s MANGLER kilde: %s" % (name, rel))
            failed += 1
            continue
        try:
            samples = render(src, pan, start, dur, fi, fo, pre)
            src_peak = write_wav(os.path.join(out, name + ".wav"), samples)
        except (RuntimeError, OSError) as err:
            print("  %-16s FEIL: %s" % (name, err))
            failed += 1
            continue
        print("  %-16s %4d ms  kildetopp %6.1f dBFS  <- %s @ %.4f s"
              % (name, round(len(samples) * 1000.0 / SR), src_peak, rel, start))
    print("licensed_sounds: %d av %d skrevet til %s"
          % (len(RECIPES) - failed, len(RECIPES), out))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
