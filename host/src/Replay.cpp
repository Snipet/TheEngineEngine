// host/src/Replay.cpp: the session log and its replay (Replay.h).
#include "tee/Replay.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace tee
{
    namespace
    {
        std::string hex(const unsigned char* data, std::size_t n)
        {
            static const char digits[] = "0123456789abcdef";
            std::string out(2 * n, '0');
            for (std::size_t i = 0; i < n; ++i)
            {
                out[2 * i] = digits[data[i] >> 4];
                out[2 * i + 1] = digits[data[i] & 15];
            }
            return out;
        }

        bool unhex(const std::string& text, std::vector<unsigned char>& out)
        {
            if (text.size() % 2 != 0)
                return false;
            out.resize(text.size() / 2);
            for (std::size_t i = 0; i < out.size(); ++i)
            {
                int v = 0;
                for (int k = 0; k < 2; ++k)
                {
                    const char c = text[2 * i + static_cast<std::size_t>(k)];
                    const int d = c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1);
                    if (d < 0)
                        return false;
                    v = v * 16 + d;
                }
                out[i] = static_cast<unsigned char>(v);
            }
            return true;
        }
    } // namespace

    std::string SessionLog::json() const
    {
        nlohmann::ordered_json root;
        root["format"] = "tee-session";
        root["version"] = 1;
        root["rate"] = rate;
        root["samples"] = samples;
        root["start"] = hex(start.data(), start.size());
        nlohmann::ordered_json list = nlohmann::ordered_json::array();
        for (const LogEvent& e : events)
        {
            nlohmann::ordered_json j;
            j["sample"] = e.sample;
            if (e.kind == LogEvent::Kind::Controls)
            {
                nlohmann::ordered_json c;
                for (const ControlField& f : controlFields())
                {
                    double v = 0.0;
                    getControl(e.controls, f.name, v);
                    c[f.name] = v;
                }
                j["controls"] = c;
            }
            else
            {
                j["offset"] = e.offset;
                j["section"] = e.section;
                j["bytes"] = hex(e.bytes.data(), e.bytes.size());
            }
            list.push_back(std::move(j));
        }
        root["events"] = list;
        return root.dump(1) + "\n";
    }

    bool SessionLog::parse(std::string_view text, SessionLog& log, std::string& error)
    {
        const nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
        if (root.is_discarded() || root.value("format", std::string()) != "tee-session")
        {
            error = "not a session log (\"format\": \"tee-session\")";
            return false;
        }
        log = SessionLog{};
        log.rate = root.value("rate", 48000.0);
        log.samples = root.value("samples", std::int64_t{0});
        if (!unhex(root.value("start", std::string()), log.start))
        {
            error = "its \"start\" is not hex";
            return false;
        }
        for (const nlohmann::json& j : root.value("events", nlohmann::json::array()))
        {
            LogEvent e;
            e.sample = j.value("sample", std::int64_t{0});
            if (j.contains("controls"))
            {
                e.kind = LogEvent::Kind::Controls;
                for (auto it = j["controls"].begin(); it != j["controls"].end(); ++it)
                    setControl(e.controls, it.key(), it.value().get<double>());
            }
            else
            {
                e.kind = LogEvent::Kind::Spec;
                e.offset = j.value("offset", 0u);
                e.section = static_cast<std::int16_t>(j.value("section", -1));
                if (!unhex(j.value("bytes", std::string()), e.bytes))
                {
                    error = "an event's \"bytes\" are not hex";
                    return false;
                }
            }
            log.events.push_back(std::move(e));
        }
        return true;
    }

    bool SessionLog::saveFile(const std::filesystem::path& file, std::string& error) const
    {
        std::ofstream out(file, std::ios::binary);
        out << json();
        if (!out)
        {
            error = file.generic_string() + ": cannot be written";
            return false;
        }
        return true;
    }

    bool SessionLog::loadFile(const std::filesystem::path& file, SessionLog& log, std::string& error)
    {
        std::ifstream in(file, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        if (!in && !in.eof())
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        return parse(text.str(), log, error);
    }

    SessionLog takeSession(SimHost& host, std::int64_t samples)
    {
        SessionLog log;
        log.rate = host.sampleRate();
        log.samples = samples;
        log.start = host.logStart();
        log.events = host.takeLog();
        return log;
    }

    bool replay(SimHost& host, const SessionLog& log, int block, std::vector<float>& left, std::vector<float>& right,
                std::string& error)
    {
        if (host.sampleRate() != log.rate)
        {
            error = "the host plays at another rate than the log's";
            return false;
        }
        const Fields& fields = host.fields();
        // the Spec as the log began, a field at a time
        for (std::size_t i = 0; i < fields.all().size(); ++i)
        {
            const FieldInfo& f = fields.all()[i];
            if (f.block != "spec" || f.kind == FieldKind::Rev)
                continue;
            if (f.offset + f.size > log.start.size())
            {
                error = "the log was made with another program (its Spec is shorter)";
                return false;
            }
            if (!host.setBytes(fields.ref(static_cast<int>(i)), log.start.data() + f.offset, f.size))
            {
                error = "the host's queue is too small for the Spec";
                return false;
            }
        }
        // a field's ref for a write at an offset
        const auto refAt = [&](std::uint32_t offset, std::size_t size) {
            for (std::size_t i = 0; i < fields.all().size(); ++i)
            {
                const FieldInfo& f = fields.all()[i];
                if (f.block == "spec" && f.kind != FieldKind::Rev && offset >= f.offset && offset + size <= f.offset + f.size)
                {
                    FieldRef r = fields.ref(static_cast<int>(i));
                    r.offset = offset;
                    r.size = static_cast<std::uint32_t>(size);
                    return r;
                }
            }
            return FieldRef{};
        };
        left.assign(static_cast<std::size_t>(log.samples), 0.0f);
        right.assign(static_cast<std::size_t>(log.samples), 0.0f);
        std::size_t next = 0;
        std::int64_t pos = 0;
        while (pos < log.samples)
        {
            while (next < log.events.size() && log.events[next].sample <= pos)
            {
                const LogEvent& e = log.events[next++];
                if (e.kind == LogEvent::Kind::Controls)
                    host.setControls(e.controls);
                else
                {
                    const FieldRef r = refAt(e.offset, e.bytes.size());
                    if (!r.valid() || !host.setBytes(r, e.bytes.data(), e.bytes.size()))
                    {
                        error = "the log writes at an offset no field of this program has";
                        return false;
                    }
                }
            }
            std::int64_t n = std::min<std::int64_t>(block, log.samples - pos);
            if (next < log.events.size())
                n = std::min<std::int64_t>(n, std::max<std::int64_t>(1, log.events[next].sample - pos));
            host.process(left.data() + pos, right.data() + pos, static_cast<int>(n));
            host.service();
            pos += n;
        }
        return true;
    }
} // namespace tee
