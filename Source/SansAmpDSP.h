#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include "SansAmpFIR.h"
#include "BinaryData.h"

namespace SansAmpDSP
{

// Exact Field 15 waveshaper linear tail constants (0x50000CC40)
static constexpr float kPsa2SlopePos     =  2.360343933e-05f; // 0x37C60000
static constexpr float kPsa2InterceptPos =  0.2738398015f;    // 0x3E8C34BB
static constexpr float kPsa2SlopeNeg     = -1.788139343e-05f; // 0xB7960000
static constexpr float kPsa2InterceptNeg = -0.3116825521f;    // 0xBE9F94DB

static constexpr float kPsa1SlopePos     =  3.039836884e-06f; // 0x364C0000
static constexpr float kPsa1InterceptPos =  0.5248223543f;    // 0x3F065AC2
static constexpr float kPsa1SlopeNeg     =  1.370906830e-06f; // 0x35B80000
static constexpr float kPsa1InterceptNeg = -0.6525591612f;    // 0xBF270E1E

// Knob taper function (0x500008CA0): maps 0..100 -> 0..100
inline float applyTaper (float x) noexcept
{
    if (x >= 50.0f)
    {
        const float d = x - 50.0f;
        if (d <= 0.0f)
            return 50.0f;
        const float denom = static_cast<float> (std::exp (std::pow (static_cast<double> (d) / 80.0, 1.2)));
        return (d * 2.5f + 50.0f) / denom;
    }
    else
    {
        const float d = 50.0f - x;
        const float denom = static_cast<float> (std::exp (std::pow (static_cast<double> (d) / 80.0, 1.2)));
        const float val = 100.0f - (d * 2.5f + 50.0f) / denom;
        return val < 0.0f ? 0.0f : val;
    }
}

struct Coeffs
{
    // Field 5: Pre-Amp (0x50000AD80)
    float preamp[3] {};
    // Field 6: Buzz (0x50000AF60)
    float buzz[2] {};
    // Field 7: Punch (0x50000B090)
    float punch[3] {};
    // Field 8: Crunch (0x50000B200)
    float crunch[2] {};
    // Field 9: Drive (0x50000B330)
    float drive[2] {};
    // Field 10: Low (0x50000B460)
    float low[3] {};
    // Field 11: High (0x50000B5F0)
    float high[2] {};
    // Field 12: Level (0x50000B750)
    float level = 1.0f;

    // Field 13: Sample-rate fixed coefficients (0x50000B820)
    float sr[16] {};
    int srFlag = 0x30;
    int latencySamples = 38;
    const float* upFIR = kUpFIR_48k;
    const float* downFIR = kDownFIR_48k;

    void updateSampleRate (float sampleRate) noexcept
    {
        const double srD = static_cast<double> (sampleRate);

        if (sampleRate > 100000.0f)
        {
            srFlag = 0xC0;
            latencySamples = 83;
            upFIR = kUpFIR_192k;
            downFIR = kDownFIR_192k;
        }
        else if (sampleRate > 50000.0f)
        {
            srFlag = 0x60;
            latencySamples = 53;
            upFIR = kUpFIR_96k;
            downFIR = kDownFIR_96k;
        }
        else
        {
            srFlag = 0x30;
            latencySamples = 38;
            upFIR = kUpFIR_48k;
            downFIR = kDownFIR_48k;
        }

        // [0x00..0x3C] (16 floats)
        sr[0]  = static_cast<float> (1.0 - std::exp (-29.9708 * 0.5 / srD));
        sr[1]  = static_cast<float> (1.0 - std::exp (-108000.0 * 0.5 / srD));
        sr[2]  = static_cast<float> (1.0 - std::exp (-12566.370614359172 * 0.5 / srD));
        sr[3]  = static_cast<float> (1.0 - std::exp (-5026.548245743669 * 0.5 / srD));
        sr[4]  = static_cast<float> (1.0 - std::exp (-1.0 / (srD * 0.0001585)));
        sr[5]  = static_cast<float> (1.0 - std::exp (-12566.370614359172 * 0.5 / srD));
        sr[6]  = static_cast<float> (1.0 - std::exp (-263.89378290154264 * 0.5 / srD));
        sr[7]  = static_cast<float> (1.0 - std::exp (-23184.95378349267 * 0.5 / srD));
        sr[8]  = static_cast<float> (1.0 - std::exp (-18472.56480310798 * 0.5 / srD));
        sr[9]  = static_cast<float> (1.0 - std::exp (-36442.4747816416 * 0.5 / srD));

        double w10 = -57805.30482605219;
        if (sampleRate > 100000.0f)
            w10 = -80000.0;
        else if (sampleRate > 50000.0f)
            w10 = -71628.31250184759;
        sr[10] = static_cast<float> (1.0 - std::exp ((w10 / srD) * 0.5));

        sr[11] = static_cast<float> (1.0 - std::exp (-94247.77960769379 * 0.5 / srD));
        sr[12] = static_cast<float> (1.0 - std::exp (-534.0707511102648 / srD));
        sr[13] = static_cast<float> (1.0 - std::exp (-11309.733552923255 / srD));
        sr[14] = static_cast<float> (1.0 - std::exp (-7539.822368615503 / srD));
        sr[15] = static_cast<float> (1.0 - std::exp (-27646.01535159018 / srD));
    }

    void updateParams (float preampPct, float buzzPct, float punchPct, float crunchPct,
                       float drivePct, float lowPct, float highPct, float levelPct,
                       float sampleRate) noexcept
    {
        const double srD = static_cast<double> (sampleRate);

        // 1. Pre-Amp (0x50000AD80)
        {
            const float v = applyTaper (preampPct);
            const float v10 = v / 10.0f;
            const float t1 = (12.0f - v10) / 10.0f;
            const float t2 = (11.0f - v10) / (v10 + 1.0f);
            preamp[2] = 1.0f / (t1 + t2);
            const double d10 = 10.22 - static_cast<double> (v10);
            preamp[1] = static_cast<float> (1.0 - std::exp (-1.0 / (d10 * 0.001 * srD * 2.0)));
            preamp[0] = static_cast<float> (1.0 - std::exp (-1.0 / (d10 * 3.3e-6 * static_cast<double> (preamp[2]) * srD * 2.0)));
        }

        // 2. Buzz (0x50000AF60)
        {
            const float v = applyTaper (buzzPct);
            const float d = v - 50.0f;
            double w;
            if (d >= 0.0f)
            {
                buzz[1] = static_cast<float> (static_cast<double> (d) / 20.833333);
                w = -1256.6370614359173;
            }
            else
            {
                buzz[1] = static_cast<float> (static_cast<double> (d) / 71.43);
                w = -5026.548245743669;
            }
            buzz[0] = static_cast<float> (1.0 - std::exp ((w / srD) * 0.5));
        }

        // 3. Punch (0x50000B090)
        {
            const float v = applyTaper (punchPct);
            if (v >= 50.0f)
            {
                punch[2] = (v - 50.0f) * 0.01850000024f;
                punch[1] = (v - 50.0f) * 0.03154810145f;
            }
            else
            {
                punch[2] = 0.0f;
                punch[1] = (v - 50.0f) / 50.0f;
            }
            const double denom = (1.0 / (1.0 / (static_cast<double> (v) + 1.0) + 0.01)) * 3.3e-7 * srD * 0.25 * 2.0;
            punch[0] = static_cast<float> (1.0 - std::exp (-1.0 / denom));
        }

        // 4. Crunch (0x50000B200)
        {
            const float v = applyTaper (crunchPct);
            const double denom = (101.0 - static_cast<double> (v)) * 2.2e-5 * srD * 2.0;
            crunch[0] = static_cast<float> (1.0 - std::exp (-1.0 / denom));
            crunch[1] = 101.0f / (101.0f - v);
        }

        // 5. Drive (0x50000B330)
        {
            const float v = applyTaper (drivePct);
            const double denom = (101.0 - static_cast<double> (v)) * 0.0022 * srD * 0.25 * 2.0;
            drive[0] = static_cast<float> (1.0 - std::exp (-1.0 / denom));
            drive[1] = 101.0f / (101.0f - v);
        }

        // 6. Low (0x50000B460)
        {
            const float v = applyTaper (lowPct);
            const float v10 = v / 10.0f;
            low[1] = static_cast<float> (1.0 - std::exp (-1.0 / (srD * 0.006)));
            const double w = (v10 >= 5.0f) ? -1570.7963267948967 : -3141.592653589793;
            low[0] = static_cast<float> (1.0 - std::exp (w / srD));
            const float t1 = (12.0f - v10) / 10.0f + 0.3000000119f;
            const float t2 = (11.0f - v10) / (v10 + 1.0f) + t1;
            low[2] = (1.0f / t2) - 0.9237499833f;
        }

        // 7. High (0x50000B5F0)
        {
            const float v = applyTaper (highPct);
            if (v <= 50.0f)
            {
                high[1] = 1.0f - (v / 50.0f);
                high[0] = static_cast<float> (1.0 - std::exp (-6283.185307179586 / srD));
            }
            else
            {
                const float d = v - 50.0f;
                double w;
                if (sampleRate > 50000.0f)
                {
                    high[1] = d * 0.1923773438f;
                    w = -40840.70449666731;
                }
                else
                {
                    high[1] = d * 0.3000000119f;
                    w = -47123.88980384689;
                }
                high[0] = static_cast<float> (1.0 - std::exp (w / srD));
            }
        }

        // 8. Level (0x50000B750)
        {
            const float v = applyTaper (levelPct);
            level = ((v + 4.699999809f) / (v + 104.6999969f)) * (100.0f / (106.9000015f - v) + 1.0f);
        }
    }
};

class ChannelState
{
public:
    ChannelState() noexcept
    {
        psa1Table = reinterpret_cast<const float*> (BinaryData::g_psa1_bin);
        psa2Table = reinterpret_cast<const float*> (BinaryData::g_psa2_bin);
        reset();
    }

    void reset() noexcept
    {
        upWriteIdx = 0;
        downWriteIdx = 0;
        bypassIdx = 0;
        prevInput = 0.0f;
        std::memset (upDelay, 0, sizeof (upDelay));
        std::memset (downDelay, 0, sizeof (downDelay));
        std::memset (bypassDelay, 0, sizeof (bypassDelay));

        preampS1 = 0.0f;
        preampS2 = 0.0f;
        hp1State = 0.0f;
        buzzLp1 = 0.0f;
        buzzLp2 = 0.0f;
        hp2State = 0.0f;
        punchLp = 0.0f;
        punchBp = 0.0f;
        punchOut = 0.0f;
        crunchDc = 0.0f;
        crunchHp = 0.0f;
        crunchLp = 0.0f;
        driveHp = 0.0f;
        driveLp = 0.0f;
        hp3State = 0.0f;

        cabLp1 = 0.0f;
        cabHp1 = 0.0f;
        cabLp2 = 0.0f;
        cabLp3 = 0.0f;
        cabHp2 = 0.0f;
        cabLp4 = 0.0f;

        aaLp1 = 0.0f;
        aaLp2 = 0.0f;
        aaLp3 = 0.0f;
        aaLp4 = 0.0f;
        aaLp5 = 0.0f;

        lowLp1 = 0.0f;
        lowLp2 = 0.0f;
        highLp = 0.0f;
        toneSmooth = 0.0f;

        postHp1 = 0.0f;
        postLp1 = 0.0f;
        postHp2 = 0.0f;
        postLp2 = 0.0f;
    }

    inline float lookupPsa1 (float x) const noexcept
    {
        if (std::fabs (x) > 1.0f)
        {
            if (x > 0.0f)
                return x * kPsa1SlopePos + kPsa1InterceptPos;
            return x * kPsa1SlopeNeg + kPsa1InterceptNeg;
        }
        const float t = (x + 1.0f) * 32767.0f;
        const int idx = static_cast<int> (t);
        const float frac = t - static_cast<float> (idx);
        const int pairIdx = idx * 2;
        const float y0 = psa1Table[pairIdx];
        const float y1 = psa1Table[pairIdx + 1];
        return y0 + frac * (y1 - y0);
    }

    inline float lookupPsa2 (float x) const noexcept
    {
        if (std::fabs (x) > 1.0f)
        {
            if (x > 0.0f)
                return x * kPsa2SlopePos + kPsa2InterceptPos;
            return x * kPsa2SlopeNeg + kPsa2InterceptNeg;
        }
        const float t = (x + 1.0f) * 2048.0f;
        const int idx = static_cast<int> (t);
        const float frac = t - static_cast<float> (idx);
        const float y0 = psa2Table[idx];
        const float y1 = psa2Table[idx + 1];
        return y0 + frac * (y1 - y0);
    }

    inline float process2xSample (float x, const Coeffs& c) noexcept
    {
        // Stage 2: Pre-Amp (0x500004BA7..0x500004CC4)
        {
            const float diff = x - preampS1;
            preampS1 += diff * c.preamp[1];
            preampS2 += (diff - preampS2) * c.preamp[0];
            x = preampS2 * c.preamp[2];
        }

        // Stage 3: First g_psa1 Waveshaper + High-Pass (0x500004CC5..0x500004E4B)
        {
            const float w = lookupPsa1 (x);
            hp1State += (w - hp1State) * c.sr[0];
            x = w - hp1State;
        }

        // Stage 4: Buzz Filter + Second g_psa1 Waveshaper + High-Pass (0x500004E4C..0x50000511D)
        {
            buzzLp1 += (x - buzzLp1) * c.buzz[0];
            const float v = buzzLp1 * c.buzz[1] + x * 0.56f;
            buzzLp2 += (v - buzzLp2) * c.sr[1];
            const float w = lookupPsa1 (buzzLp2 / 0.147f);
            hp2State += (w - hp2State) * c.sr[0];
            x = (w - hp2State) * 0.56424f;
        }

        // Stage 5: Punch Filter + Third g_psa1 Waveshaper (0x50000511E..0x500005372)
        {
            const float u = x + punchBp * c.punch[2];
            punchLp += (u - punchLp) * c.sr[2];
            punchBp += (u - punchLp - punchBp) * c.sr[3];
            punchOut += (u + punchBp * c.punch[1] - punchOut) * c.punch[0];
            x = lookupPsa1 (punchOut / 100.0f);
        }

        // Stage 6: Crunch + Drive + g_psa2 Waveshaper (0x500005373..0x500005724)
        {
            crunchDc += (x - crunchDc) * c.sr[0];
            const float x1 = (x - crunchDc) * 100.0f;
            crunchHp += (x1 - crunchHp) * c.crunch[0];
            crunchLp += (x1 - crunchHp - crunchLp) * c.sr[1];
            const float x2 = crunchLp * c.crunch[1];
            driveHp += (x2 - driveHp) * c.drive[0];
            driveLp += (x2 - driveHp - driveLp) * c.sr[1];
            const float x3 = driveLp * c.drive[1];
            x = lookupPsa2 (x3);
        }

        // Stage 7: High-Pass Filter + Gain (0x500005725..0x500005984)
        {
            hp3State += (x - hp3State) * c.sr[0];
            x = (x - hp3State) * 0.14563f;
        }

        // Stage 8: Cabinet / Shaping Filter Cascade (0x500005985..0x500005D39)
        {
            cabLp1 += (x - cabLp1) * c.sr[5];
            const float u = cabLp1 + x * 0.5625f;
            cabHp1 += (u - cabHp1) * c.sr[6];
            cabLp2 += ((u - cabHp1) * 22.5f - cabLp2) * c.sr[7];
            cabLp3 += (cabLp2 - cabLp3) * c.sr[8];
            const float v = cabLp3 * 0.21f;
            cabHp2 += (v - cabHp2) * c.sr[9];
            cabLp4 += ((v - cabHp2) + v - cabLp4) * c.sr[10];
            x = cabLp4;
        }

        // Stage 9a: 5-Pole Anti-Aliasing Low-Pass Cascade (0x500005D3A..0x500005FB4)
        {
            const float g = c.sr[11];
            aaLp1 += (x - aaLp1) * g;
            aaLp2 += (aaLp1 - aaLp2) * g;
            aaLp3 += (aaLp2 - aaLp3) * g;
            aaLp4 += (aaLp3 - aaLp4) * g;
            aaLp5 += (aaLp4 - aaLp5) * g;
            x = aaLp5 * 0.532f;
        }

        return x;
    }

    float processSample (float rawInput, const Coeffs& c, bool bypassed) noexcept
    {
        // Clamp input to [-1.0f, 1.0f] (0x5000049A9..0x5000049CF)
        const float xIn = std::clamp (rawInput, -1.0f, 1.0f);

        // Stage 1: 2x Upsampling FIR (0x500004973..0x500004BA6)
        const float mid = (xIn + prevInput) * 0.5f;
        prevInput = xIn;

        upDelay[upWriteIdx + 1] = mid;
        upDelay[upWriteIdx + 93] = mid;
        upDelay[upWriteIdx] = xIn;
        upDelay[upWriteIdx + 92] = xIn;

        float accA = 0.0f;
        float accB = 0.0f;
        const float* firUp = c.upFIR;
        for (int k = 0; k < 92; ++k)
        {
            accA += upDelay[upWriteIdx + 1 + k] * firUp[k];
            accB += upDelay[upWriteIdx + k] * firUp[k];
        }
        if (upWriteIdx < 2)
            upWriteIdx += 90;
        else
            upWriteIdx -= 2;

        float yA = accA * 0.030599999f;
        float yB = accB * 0.030599999f;

        // Stages 2..9a at 2x rate
        yA = process2xSample (yA, c);
        yB = process2xSample (yB, c);

        // Stage 9b: 2x Downsampling 28-tap FIR (0x500005FB4..0x5000060FC)
        downDelay[downWriteIdx + 1] = yA;
        downDelay[downWriteIdx + 29] = yA;
        downDelay[downWriteIdx] = yB;
        downDelay[downWriteIdx + 28] = yB;

        float downAcc = 0.0f;
        const float* firDown = c.downFIR;
        for (int k = 0; k < 28; ++k)
            downAcc += downDelay[downWriteIdx + k] * firDown[k];

        if (downWriteIdx < 2)
            downWriteIdx += 26;
        else
            downWriteIdx -= 2;

        float y = downAcc * 10.0f;

        // Stage 10: 1x Rate Tone Stack & Post-Filter Cascade (0x5000060FF..0x50000645C)
        // Low EQ
        lowLp1 += (y - lowLp1) * c.low[1];
        lowLp2 += (y - lowLp1 - lowLp2) * c.low[0];
        const float afterLow = lowLp2 * c.low[2] + y;

        // High EQ + Level
        highLp += (afterLow - highLp) * c.high[0];
        const float highTerm = (c.high[1] >= 1.0f)
                             ? (afterLow * (2.0f - c.high[1]) + highLp * (c.high[1] - 1.0f))
                             : ((afterLow - highLp) * c.high[1] + afterLow);
        toneSmooth += (highTerm - toneSmooth) * c.sr[4];
        const float afterLevel = toneSmooth * c.level;

        // Final Post-Filter Cascade
        postHp1 += (afterLevel - postHp1) * c.sr[12];
        const float p1 = (afterLevel - postHp1) * 0.035278f;
        postLp1 += (p1 - postLp1) * c.sr[13];
        const float p2 = p1 * 0.511258f + postLp1;
        postHp2 += (p2 - postHp2) * c.sr[14];
        const float p3 = (p2 - postHp2) + p2 * 0.200894f;
        postLp2 += (p3 - postLp2) * c.sr[15];
        const float wetOut = std::clamp ((p3 * 0.558852f + postLp2) * 19.45578f, -1.0f, 1.0f);

        // Latency-compensated dry delay line for sample-accurate Bypass (0x5000063FD..0x50000645C)
        const float dryDelayed = bypassDelay[bypassIdx];
        bypassDelay[bypassIdx] = rawInput;
        if (++bypassIdx >= c.latencySamples)
            bypassIdx = 0;

        return bypassed ? dryDelayed : wetOut;
    }

private:
    const float* psa1Table = nullptr;
    const float* psa2Table = nullptr;

    int upWriteIdx = 0;
    int downWriteIdx = 0;
    int bypassIdx = 0;
    float prevInput = 0.0f;

    float upDelay[188] {};
    float downDelay[60] {};
    float bypassDelay[128] {};

    float preampS1 = 0.0f, preampS2 = 0.0f;
    float hp1State = 0.0f;
    float buzzLp1 = 0.0f, buzzLp2 = 0.0f, hp2State = 0.0f;
    float punchLp = 0.0f, punchBp = 0.0f, punchOut = 0.0f;
    float crunchDc = 0.0f, crunchHp = 0.0f, crunchLp = 0.0f;
    float driveHp = 0.0f, driveLp = 0.0f;
    float hp3State = 0.0f;

    float cabLp1 = 0.0f, cabHp1 = 0.0f, cabLp2 = 0.0f;
    float cabLp3 = 0.0f, cabHp2 = 0.0f, cabLp4 = 0.0f;

    float aaLp1 = 0.0f, aaLp2 = 0.0f, aaLp3 = 0.0f, aaLp4 = 0.0f, aaLp5 = 0.0f;

    float lowLp1 = 0.0f, lowLp2 = 0.0f;
    float highLp = 0.0f, toneSmooth = 0.0f;
    float postHp1 = 0.0f, postLp1 = 0.0f, postHp2 = 0.0f, postLp2 = 0.0f;
};

} // namespace SansAmpDSP
