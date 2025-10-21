#include "gaze_sequence.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
// split line by comma - simple and fast
static inline void split_by_comma(const std::string &line, std::vector<std::string> &out)
{
    out.clear();
    std::string cur;
    for (char c : line)
    {
        if (c == ',')
        {
            out.push_back(cur);
            cur.clear();
        }
        else
        {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
}

GazeSequence::GazeSequence(const std::string &path, double og_sampling_rate, double target_sampling_rate)
{
    readSequence(path);
    undersample(og_sampling_rate, target_sampling_rate);
}
void GazeSequence::readSequence(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        std::cerr << "[GazeSequence] Error: cannot open file: " << path << std::endl;
        return;
    }
    std::vector<std::string> toks;
    std::string line;
    std::getline(ifs, line);
    int lineNo = 0;
    while (std::getline(ifs, line))
    {
        ++lineNo;
        if (line.empty())
            continue;
        split_by_comma(line, toks);

        if (toks.size() < 8)
        {
            std::cerr << "[GazeSequence] Warning: line " << lineNo << " has fewer than 8 columns -> skipping\n";
            continue;
        }
        try
        {
            Gazes gaze = {std::stoi(toks[0]), std::stoi(toks[7]), std::stod(toks[1]), std::stod(toks[2]), std::stod(toks[4]), std::stod(toks[5])};
            gazeSequence.push_back(gaze);
        }
        catch (const std::exception &ex)
        {
            std::cerr << "[GazeSequence] parse error at line " << lineNo << ": " << ex.what() << " -> skipping\n";
            continue;
        }
    }
    std::cerr << "[GazeSequence] Initial GazeSequence of " << gazeSequence.size() << std::endl;
}

void GazeSequence::undersample(double og_sampling_rate, double target_sampling_rate)
{
    const int t0 = gazeSequence.front().ts;
    const int tLast = gazeSequence.back().ts;
    const double target_period_ms = 1000.0 / target_sampling_rate; // ms per sample

    std::vector<Gazes> target_sequence;
    target_sequence.reserve(static_cast<size_t>((tLast - t0) / std::max(1.0, target_period_ms) + 4));

    size_t idx = 0;

    // For each target time, find the last sample with ts <= target_time.
    for (double t = static_cast<double>(t0); t <= static_cast<double>(tLast); t += target_period_ms)
    {
        // advance idx while next sample timestamp <= t
        while (idx + 1 < gazeSequence.size() && gazeSequence[idx + 1].ts <= static_cast<int>(std::round(t)))
        {
            ++idx;
        }
        target_sequence.push_back(gazeSequence[idx]);
    }

    gazeSequence.swap(target_sequence);

    std::cerr << "[GazeSequence] Undersampled to " << gazeSequence.size()
              << " samples at " << target_sampling_rate << " Hz (period " << target_period_ms << " ms)" << std::endl;
}

Gazes GazeSequence::getLatestGaze(int ts)
{
    if (ts <= gazeSequence.front().ts)
        return gazeSequence.front();
    if (ts >= gazeSequence.back().ts)
        return gazeSequence.back();
    if (ts >= gazeSequence[cursor].ts)
    {
        // stop at last sample <= ts
        while (cursor + 1 < gazeSequence.size() && gazeSequence[cursor + 1].ts <= ts)
        {
            ++cursor;
        }
        return gazeSequence[cursor];
    }
    return Gazes{0, 0, 0.0, 0.0, 0.0, 0.0};
}

double GazeSequence::getLastTimestamp()
{
    return gazeSequence.back().ts;
}
void GazeSequence::resetCursor()
{
    cursor = 0;
}
