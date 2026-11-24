// shell/player/RefAdapter.h: the reference engine (the keel's tee::RefPlayer over sim/ref/program.fm) as a Player.
//
//   Parameters   the reference's 32 dials, from the program's `param` declarations (name, type, range, default). What
//                the declarations do not say (a label, a unit, an int's choices, a taper) the reference keeps in its
//                panel.gui, which nothing here reads: a table in RefAdapter.cpp carries those of the dials it knows,
//                and a dial it does not know shows by its name.
//   Telemetry    Shared's show* fields, unpacked into the names and SI units of tee::telemetry (Sources.h); and each
//                of them raw, by its own name ("showLamps", "showRpm", ...), exactly as the reference writes it: the
//                reference's own panel (sim/ref/panel.gui) hands those to its shaders, whose packing is theirs.
//   Controls     the frame onto the reference's Host block (host.fm): the pedals and the lever as they are; a press
//                counted in shiftUp or shiftDown as a short hold of the right or left paddle's bit (4 and 5); the start
//                key as note 36, held while the frame says ignition 2. Played from the keys (DRIVE KEYS or PITCH, where
//                the reference stops an engine nobody holds a key for), the note is held while ignition is 1 or 2, and
//                the throttle is the mod wheel, the reference's "throttle by hand". The frame's handbrake, steer and
//                mode have nothing to go to in the reference.
//
// Threads: the UI thread sets parameters and reads them and the telemetry; process() runs on the audio thread and is
// the only caller of the reference while it plays; prepare() runs with the audio stopped.
#pragma once

#include "player/Boxes.h"
#include "tee/app/Sources.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tee
{
    class RefPlayer;
}

namespace tee::player
{
    class RefAdapter final : public Player
    {
    public:
        struct Options
        {
            std::filesystem::path directory;    // sim/ref
            bool checks = false;                // the loop budget and the bounds checks (a development build)
            int optimisation = 2;
            double sampleRate = 48000.0;
        };

        // Compiles the reference. nullptr, and the compiler's words in `errors`, when it does not compile.
        static std::unique_ptr<RefAdapter> create(const Options&, std::string& errors);
        ~RefAdapter() override;

        // What the compile took and said, for --info.
        double compileSeconds() const noexcept { return compileSeconds_; }
        const std::string& warnings() const noexcept { return warnings_; }

        // True once the program was stopped by its loop budget or a bounds check (a build with checks): it plays
        // silence from then on. The audio thread sets it; why() is for the UI thread once stopped() is true.
        bool stopped() const noexcept { return stopped_.load(std::memory_order_acquire); }
        std::string why() const;

        // ParamSource
        int count() const override { return static_cast<int>(infos_.size()); }
        const ParamInfo& info(int i) const override;
        double get(int i) const override;
        void set(int i, double value) override;
        int findParam(std::string_view id) const override;

        // TelemetrySource
        int findValue(std::string_view name) const override;
        double value(int handle) const override;
        float outputPeak() const override { return peak_.load(std::memory_order_relaxed); }

        // ControlSink
        void setControls(const ControlsFrame& f) override { controls_.set(f); }

        // AudioSource
        void prepare(double rate, int maxBlock) override;
        void process(float* l, float* r, int n) noexcept override;

    private:
        RefAdapter();

        struct Pulse                            // a paddle's bit: held for a moment a press, then let go for one
        {
            int seen = 0;                       // the frame's count as last followed
            int hold = 0;                       // samples the bit is still held
            int gap = 0;                        // samples it must stay clear before the next press
        };
        bool pulse(Pulse&, int count, int n) noexcept;
        void publish() noexcept;

        std::unique_ptr<RefPlayer> ref_;
        std::vector<ParamInfo> infos_;
        std::unique_ptr<std::atomic<double>[]> shown_;      // what get() answers
        int driveDial_ = -1;
        double compileSeconds_ = 0.0;
        std::string warnings_;

        // Shared's show* fields by index in the reference (-1: it has none of that name)
        int sRpm_ = -1, sRed_ = -1, sGear_ = -1, sKmh_ = -1, sThr_ = -1, sMap_ = -1, sLamps_ = -1, sPedals_ = -1,
            sCycle_ = -1, sBoost_ = -1;
        std::vector<std::string> showNames_;                 // every show* field, in the reference's order
        std::unique_ptr<std::atomic<double>[]> tel_;         // the SI values, then the raw show* fields
        std::atomic<float> peak_{ 0.0f };
        std::atomic<bool> stopped_{ false };
        ControlsBox controls_;

        // the audio thread's own
        double rate_ = 48000.0;
        int pulseSamples_ = 480;
        Pulse up_, down_;
        bool noteHeld_ = false;
    };
}
