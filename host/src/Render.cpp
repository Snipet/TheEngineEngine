// host/src/Render.cpp: the offline renderer (Render.h).
#include "tee/Render.h"

#include "tee/Clock.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace tee
{
    namespace
    {
        // An input's field by the name a scenario gives it.
        bool setInput(RefPlayer::Input& input, const std::string& name, double value)
        {
            const float v = static_cast<float>(value);
            if (name == "throttle")
                input.throttle = v;
            else if (name == "brake")
                input.brake = v;
            else if (name == "clutch")
                input.clutch = v;
            else if (name == "steering")
                input.steering = v;
            else if (name == "gear")
                input.gear = static_cast<int>(std::lround(value));
            else if (name == "modwheel")
                input.modwheel = v;
            else if (name == "pitchbend")
                input.pitchbend = v;
            else if (name == "aftertouch")
                input.aftertouch = v;
            else if (name == "sustain")
                input.sustain = value != 0.0;
            else
                return false;
            return true;
        }
    } // namespace

    int RenderResult::column(std::string_view name) const noexcept
    {
        for (std::size_t c = 0; c < columns.size(); ++c)
            if (columns[c] == name)
                return static_cast<int>(c);
        return -1;
    }

    bool render(RefPlayer& player, const Preset& preset, const Scenario& scenario, const RenderOptions& options,
                RenderResult& result, std::string& error)
    {
        using Clock = std::chrono::steady_clock;
        result = RenderResult{};
        player.reset();
        const double rate = player.sampleRate();

        for (const auto& [name, value] : preset.dials)
        {
            if (!player.setDial(name, value))
            {
                error = "the preset " + preset.name + " sets '" + name + "', and the program has no such dial";
                return false;
            }
        }
        // Each event's sample, and each dial event's dial, found before anything plays.
        std::vector<std::int64_t> eventAt(scenario.events.size());
        std::vector<int> eventDial(scenario.events.size(), -1);
        for (std::size_t e = 0; e < scenario.events.size(); ++e)
        {
            const Event& event = scenario.events[e];
            eventAt[e] = std::llround(event.time * rate);
            if (event.kind == Event::Kind::Dial)
            {
                eventDial[e] = player.dialIndex(event.name);
                if (eventDial[e] < 0)
                {
                    error = "the scenario " + scenario.name + " sets '" + event.name + "', and the program has no such dial";
                    return false;
                }
            }
        }

        const double seconds = options.seconds > 0.0 ? options.seconds : scenario.seconds;
        const std::int64_t total = std::max<std::int64_t>(std::llround(seconds * rate), 0);
        const int block = std::max(options.blockSize, 1);
        result.audio.sampleRate = rate;
        result.columns = player.showNames();
        const std::size_t columns = result.columns.size();
        std::vector<float> left;
        std::vector<float> right;
        if (options.keepAudio)
        {
            result.audio.left.assign(static_cast<std::size_t>(total), 0.0f);
            result.audio.right.assign(static_cast<std::size_t>(total), 0.0f);
        }
        else
        {
            left.assign(static_cast<std::size_t>(block), 0.0f);
            right.assign(static_cast<std::size_t>(block), 0.0f);
        }
        result.rowTime.reserve(static_cast<std::size_t>(total / block + 1) + scenario.events.size());
        result.rows.reserve(result.rowTime.capacity() * columns);

        std::int64_t at = 0;
        std::size_t next = 0;
        while (at < total)
        {
            // every event due by this sample, in order
            while (next < scenario.events.size() && eventAt[next] <= at)
            {
                const Event& event = scenario.events[next];
                switch (event.kind)
                {
                    case Event::Kind::Dial:
                        player.setDial(static_cast<std::size_t>(eventDial[next]), event.value);
                        break;
                    case Event::Kind::Input:
                        setInput(player.input(), event.name, event.value);
                        break;
                    case Event::Kind::Press:
                        player.input().buttons |= event.bits;
                        break;
                    case Event::Kind::Release:
                        player.input().buttons &= ~event.bits;
                        break;
                    case Event::Kind::KeyDown:
                        player.start(static_cast<float>(event.value), static_cast<float>(event.velocity));
                        break;
                    case Event::Kind::KeyUp:
                        player.stop();
                        break;
                }
                ++next;
            }
            // one call: a block, cut short where the next event falls
            std::int64_t n = std::min<std::int64_t>(block, total - at);
            if (next < scenario.events.size())
                n = std::min(n, eventAt[next] - at);
            float* l = options.keepAudio ? result.audio.left.data() + at : left.data();
            float* r = options.keepAudio ? result.audio.right.data() + at : right.data();

            const bool sounded = player.sounding();
            const double cpuStart = threadCpuSeconds();
            const Clock::time_point start = Clock::now();
            player.process(l, r, static_cast<int>(n));
            const double took = std::chrono::duration<double>(Clock::now() - start).count();
            const double cpuTook = threadCpuSeconds() - cpuStart;
            ++result.calls;
            if (sounded)
            {
                result.soundingFrames += n;
                result.soundingSeconds += took;
                result.soundingCpuSeconds += cpuTook;
                result.worstCallShare = std::max(result.worstCallShare, took * rate / static_cast<double>(n));
            }
            for (std::int64_t i = 0; i < n; ++i)
                result.peak = std::max(result.peak, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));

            at += n;
            result.rowTime.push_back(static_cast<double>(at) / rate);
            for (std::size_t c = 0; c < columns; ++c)
                result.rows.push_back(player.show(c));
            if (player.stopped())
            {
                result.stopped = true;
                result.stopReason = player.stopReason();
                break;
            }
        }
        result.frames = at;
        return true;
    }
} // namespace tee
