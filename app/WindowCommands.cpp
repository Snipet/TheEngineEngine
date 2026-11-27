// app/WindowCommands.cpp: WindowCommands.h.
#include "WindowCommands.h"

#include "audio/Recorder.h"
#include "files/BuildFiles.h"
#include "settings/Settings.h"

#include <funkgui/core/Theme.h>
#include <funkgui/panel/HostServices.h>

#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iterator>
#include <memory>
#include <vector>

namespace tee::app
{
    namespace
    {
        constexpr const char* kPattern = "*.json";

        // The menus' ids: a range a kind.
        enum : int
        {
            kNew = 1, kOpen, kSave, kSaveAs, kDiscardAndNew,
            kRecent = 100,          // + index
            kLibrary = 200,         // + index
            kLoadPart = 400,        // + part (1 engine, 2 car, 3 tune)
            kSavePart = 410,
            kDevice = 500,          // + index; kDevice itself: the default
            kRate = 600,            // + index of kRates
            kBuffer = 700,          // + index of kBuffers
            kRecord = 800,
            kQuality = 900,         // + 0 full, 1 economy
            kColdStart = 950,
            kWheel = 960,
            kAutoClutch = 990,
            kWheelDefaults = 970,
            kWheelLearn = 971,      // + index of the bindable actions
        };
        constexpr int kRates[] = { 0, 44100, 48000, 88200, 96000 };
        constexpr int kBuffers[] = { 64, 128, 256, 512, 1024, 2048 };

        tee::BuildPart partOf(int p)
        {
            return p == 1 ? BuildPart::Engine : p == 2 ? BuildPart::Car : p == 3 ? BuildPart::Tune : BuildPart::Whole;
        }

        const char* partWord(int p)
        {
            return p == 1 ? "engine" : p == 2 ? "car" : p == 3 ? "tune" : "build";
        }

        bool primary(SDL_Keymod mod)
        {
           #if defined(__APPLE__)
            return (mod & SDL_KMOD_GUI) != 0;
           #else
            return (mod & SDL_KMOD_CTRL) != 0;
           #endif
        }

        const char* primaryName()
        {
           #if defined(__APPLE__)
            return "Cmd";
           #else
            return "Ctrl";
           #endif
        }

        funkgui::MenuRequest request(funkgui::HostServices& host, std::vector<funkgui::MenuItem> items)
        {
            funkgui::MenuRequest r;
            r.items = std::move(items);
            r.anchor = { 120.0f, 40.0f, 1.0f, 1.0f };       // under the top bar, beside the rail
            r.theme = funkgui::Theme::byIndex(host.themeIndex());
            r.minWidth = 260.0f;
            return r;
        }
    }

    void WindowCommands::say(const std::string& line) const
    {
        std::printf("%s\n", line.c_str());
        if (ctx_.say)
            ctx_.say(line);
    }

    bool WindowCommands::event(const SDL_Event& e)
    {
        if (e.type != SDL_EVENT_KEY_DOWN || e.key.repeat)
            return false;
        const SDL_Keymod mod = e.key.mod;
        const SDL_Keycode k = e.key.key;
        if (k == SDLK_F2)
        {
            fileMenu();
            return true;
        }
        if (k == SDLK_F9)
        {
            audioMenu();
            return true;
        }
        if (!primary(mod) || ctx_.document == nullptr)
            return false;
        const bool shift = (mod & SDL_KMOD_SHIFT) != 0;
        if (k == SDLK_N)
            newBuild();
        else if (k == SDLK_O)
            open();
        else if (k == SDLK_S && shift)
            saveAs();
        else if (k == SDLK_S)
            save();
        else
            return false;
        return true;
    }

    // ---- files --------------------------------------------------------------------------------------------------
    void WindowCommands::newBuild()
    {
        if (ctx_.document == nullptr)
            return;
        if (!ctx_.document->dirty() || host_ == nullptr)
        {
            ctx_.document->makeNew();
            say("file: a new build (every field at its default)");
            return;
        }
        std::vector<funkgui::MenuItem> items;
        items.push_back({ .label = "The build has unsaved changes", .header = true });
        items.push_back({ kDiscardAndNew, "Discard them and start a new build" });
        items.push_back({ kSaveAs, "Save it first..." });
        host_->showMenu(request(*host_, std::move(items)), [this](int id) {
            if (id == kDiscardAndNew)
            {
                ctx_.document->makeNew();
                say("file: a new build (the changes were discarded)");
            }
            else if (id == kSaveAs)
                saveAs();
        });
    }

    void WindowCommands::openFile(const std::string& path, int part)
    {
        std::vector<std::string> warnings;
        std::string error;
        if (!ctx_.document->open(path, partOf(part), warnings, error))
        {
            say("file: " + path + " was not opened: " + error);
            return;
        }
        for (const std::string& w : warnings)
            std::fprintf(stderr, "file: %s: %s\n", path.c_str(), w.c_str());
        say(std::string("file: ") + (part == 0 ? "opened " : std::string("the ") + partWord(part) + " loaded from ") + path
            + (warnings.empty() ? "" : " (" + std::to_string(warnings.size()) + " notes in the console)"));
        if (ctx_.settings != nullptr && part == 0)
            ctx_.settings->lastBuild = path;
    }

    void WindowCommands::open()
    {
        if (ctx_.document == nullptr || host_ == nullptr)
            return;
        funkgui::FileRequest r;
        r.mode = funkgui::FileRequest::Mode::open;
        r.title = "Open a build";
        r.pattern = kPattern;
        if (!host_->chooseFiles(r, [this](const std::vector<std::string>& paths) {
                if (!paths.empty())
                    openFile(paths.front(), 0);
            }))
            say("file: this window has no file chooser");
    }

    bool WindowCommands::shipped(const std::filesystem::path& file) const
    {
        if (ctx_.library == nullptr || ctx_.library->shippedDirectory().empty())
            return false;
        std::error_code ec;
        const std::filesystem::path dir = std::filesystem::weakly_canonical(ctx_.library->shippedDirectory(), ec);
        const std::filesystem::path f = std::filesystem::weakly_canonical(file, ec);
        const std::filesystem::path rel = f.lexically_relative(dir);
        return !rel.empty() && *rel.begin() != "..";
    }

    void WindowCommands::save()
    {
        if (ctx_.document == nullptr)
            return;
        // an untitled build, and a shipped one (content/builds: the first build a launch opens, an archetype), ask
        // for a file of the person's own: a shipped build is not written over
        if (ctx_.document->file().empty() || shipped(ctx_.document->file()))
        {
            saveAs();
            return;
        }
        std::string error;
        if (ctx_.document->save(error))
            say("file: saved " + ctx_.document->file().string());
        else
            say("file: not saved: " + error);
    }

    void WindowCommands::saveAs()
    {
        if (ctx_.document == nullptr || host_ == nullptr)
            return;
        funkgui::FileRequest r;
        r.mode = funkgui::FileRequest::Mode::save;
        r.title = "Save the build as";
        r.pattern = kPattern;
        const std::string name = ctx_.document->build().name;
        r.suggestedName = (name.empty() ? std::string("Untitled") : name) + ".json";
        host_->chooseFiles(r, [this](const std::vector<std::string>& paths) {
            if (paths.empty())
                return;
            std::string error;
            if (ctx_.document->saveAs(paths.front(), BuildPart::Whole, error))
            {
                say("file: saved " + ctx_.document->file().string());
                if (ctx_.settings != nullptr)
                    ctx_.settings->lastBuild = ctx_.document->file().string();
            }
            else
                say("file: not saved: " + error);
        });
    }

    void WindowCommands::fileMenu()
    {
        if (host_ == nullptr)
            return;
        std::vector<funkgui::MenuItem> items;
        const bool doc = ctx_.document != nullptr;
        const std::string p = primaryName();
        items.push_back({ kNew, "New build", doc, false, false, false, false, p + "+N" });
        items.push_back({ kOpen, "Open...", doc, false, false, false, false, p + "+O" });
        items.push_back({ kSave, "Save", doc, false, false, false, false, p + "+S" });
        items.push_back({ kSaveAs, "Save as...", doc, false, false, false, false, p + "+Shift+S" });
        std::vector<std::string> recent, library;
        if (doc)
        {
            funkgui::MenuItem r{ .label = "Open recent" };
            for (const auto& f : ctx_.document->recent().list())
            {
                recent.push_back(f.string());
                r.submenu.push_back({ kRecent + static_cast<int>(recent.size()) - 1, f.filename().string() });
            }
            if (r.submenu.empty())
                r.enabled = false;
            items.push_back(std::move(r));
        }
        if (doc && ctx_.library != nullptr)
        {
            funkgui::MenuItem l{ .label = "Builds" };
            for (const files::BuildSummary& b : ctx_.library->list())
            {
                library.push_back(b.name);
                l.submenu.push_back({ kLibrary + static_cast<int>(library.size()) - 1,
                                      b.name + (b.user ? "" : "  (shipped)") });
            }
            if (l.submenu.empty())
                l.enabled = false;
            items.push_back(std::move(l));
        }
        items.push_back({ .separator = true });
        funkgui::MenuItem load{ .label = "Load a part" }, savePart{ .label = "Save a part" };
        load.enabled = savePart.enabled = doc;
        for (int part = 1; part <= 3; ++part)
        {
            load.submenu.push_back({ kLoadPart + part, std::string("The ") + partWord(part) + "..." });
            savePart.submenu.push_back({ kSavePart + part, std::string("The ") + partWord(part) + "..." });
        }
        items.push_back(std::move(load));
        items.push_back(std::move(savePart));
        host_->showMenu(request(*host_, std::move(items)), [this, recent, library](int id) {
            if (id == kNew)
                newBuild();
            else if (id == kOpen)
                open();
            else if (id == kSave)
                save();
            else if (id == kSaveAs)
                saveAs();
            else if (id >= kRecent && id < kRecent + static_cast<int>(recent.size()))
                openFile(recent[static_cast<std::size_t>(id - kRecent)], 0);
            else if (id >= kLibrary && id < kLibrary + static_cast<int>(library.size()))
            {
                std::vector<std::string> warnings;
                std::string error;
                const std::string& name = library[static_cast<std::size_t>(id - kLibrary)];
                if (ctx_.library->load(*ctx_.document, name, warnings, error))
                    say("file: the build " + name + " opened");
                else
                    say("file: " + name + " was not opened: " + error);
            }
            else if (id > kLoadPart && id <= kLoadPart + 3)
            {
                const int part = id - kLoadPart;
                funkgui::FileRequest r;
                r.mode = funkgui::FileRequest::Mode::open;
                r.title = std::string("Load the ") + partWord(part) + " from";
                r.pattern = kPattern;
                host_->chooseFiles(r, [this, part](const std::vector<std::string>& paths) {
                    if (!paths.empty())
                        openFile(paths.front(), part);
                });
            }
            else if (id > kSavePart && id <= kSavePart + 3)
            {
                const int part = id - kSavePart;
                funkgui::FileRequest r;
                r.mode = funkgui::FileRequest::Mode::save;
                r.title = std::string("Save the ") + partWord(part) + " as";
                r.pattern = kPattern;
                r.suggestedName = std::string(partWord(part)) + ".json";
                host_->chooseFiles(r, [this, part](const std::vector<std::string>& paths) {
                    if (paths.empty())
                        return;
                    std::string error;
                    if (ctx_.document->saveAs(paths.front(), partOf(part), error))
                        say(std::string("file: the ") + partWord(part) + " saved to " + paths.front());
                    else
                        say("file: not saved: " + error);
                });
            }
        });
    }

    bool WindowCommands::saveText(const std::string& suggestedName, const std::string& text, std::string& error)
    {
        if (host_ == nullptr)
        {
            error = "the window is not open";
            return false;
        }
        funkgui::FileRequest r;
        r.mode = funkgui::FileRequest::Mode::save;
        r.title = "Save as";
        const std::string ext = std::filesystem::path(suggestedName).extension().string();
        r.pattern = ext.empty() ? std::string() : "*" + ext;
        r.suggestedName = suggestedName;
        const bool opened = host_->chooseFiles(r, [this, text](const std::vector<std::string>& paths) {
            if (paths.empty())
                return;
            std::FILE* f = std::fopen(paths.front().c_str(), "wb");
            const bool ok = f != nullptr && std::fwrite(text.data(), 1, text.size(), f) == text.size();
            if (f != nullptr)
                std::fclose(f);
            say(ok ? "file: saved " + paths.front() : "file: " + paths.front() + " could not be written");
        });
        if (!opened)
            error = "this window has no file chooser";
        return opened;
    }

    // ---- the audio device ---------------------------------------------------------------------------------------
    void WindowCommands::reopenAudio(const audio::Request& wanted)
    {
        if (ctx_.device == nullptr)
            return;
        // the recorder writes at the device's rate: a file is ended before the device changes under it
        if (ctx_.recorder != nullptr && ctx_.recorder->recording())
        {
            ctx_.recorder->stop();
            say("record: stopped (the audio device changed)");
        }
        std::string why;
        ctx_.request = wanted;
        if (!ctx_.device->open(wanted, why))
            say("audio: no device opened: " + why);
        else
            say("audio: " + ctx_.device->info().line());
        if (ctx_.settings != nullptr)
        {
            ctx_.settings->audioDevice = wanted.deviceName;
            ctx_.settings->audioRate = wanted.rate;
            ctx_.settings->audioBuffer = wanted.bufferFrames;
        }
        if (ctx_.audioChanged)
            ctx_.audioChanged();
    }

    void WindowCommands::audioMenu()
    {
        if (host_ == nullptr)
            return;
        if (ctx_.device == nullptr)
        {
            say("audio: there is no audio device in this run (--no-audio)");
            return;
        }
        const std::vector<std::string> devices = ctx_.device->devices();
        std::vector<funkgui::MenuItem> items;
        items.push_back({ .label = ctx_.device->info().line(), .header = true });
        funkgui::MenuItem dev{ .label = "Device" };
        dev.submenu.push_back({ kDevice, "The system's default", true, ctx_.request.deviceName.empty(), false, false, true });
        for (std::size_t i = 0; i < devices.size(); ++i)
            dev.submenu.push_back({ kDevice + 1 + static_cast<int>(i), devices[i], true, devices[i] == ctx_.request.deviceName,
                                    false, false, true });
        items.push_back(std::move(dev));
        funkgui::MenuItem rate{ .label = "Rate" };
        for (std::size_t i = 0; i < std::size(kRates); ++i)
            rate.submenu.push_back({ kRate + static_cast<int>(i),
                                     kRates[i] == 0 ? std::string("The device's own") : std::to_string(kRates[i]) + " Hz",
                                     true, ctx_.request.rate == kRates[i], false, false, true });
        items.push_back(std::move(rate));
        funkgui::MenuItem buf{ .label = "Buffer" };
        for (std::size_t i = 0; i < std::size(kBuffers); ++i)
            buf.submenu.push_back({ kBuffer + static_cast<int>(i), std::to_string(kBuffers[i]) + " frames (asked)", true,
                                    ctx_.request.bufferFrames == kBuffers[i], false, false, true });
        items.push_back(std::move(buf));
        if (ctx_.quality && ctx_.setQuality)
        {
            funkgui::MenuItem q{ .label = "Quality" };
            const int now = ctx_.quality();
            q.submenu.push_back({ kQuality, "Full (the engine at 96 kHz inside)", true, now == 0, false, false, true });
            q.submenu.push_back({ kQuality + 1, "Economy (48 kHz inside: about half the load, the top octave lost)", true,
                                  now == 1, false, false, true });
            items.push_back(std::move(q));
        }
        if (ctx_.wheel && ctx_.wheelOn && ctx_.setWheelOn && !ctx_.wheel().empty())
            items.push_back({ kWheel, "Read the racing wheel (Logitech G29): " + ctx_.wheel(), true, ctx_.wheelOn(), false,
                              false, true });
        if (ctx_.wheel && ctx_.wheelButton && ctx_.wheelButtonCount && ctx_.learnWheelButton && !ctx_.wheel().empty())
        {
            funkgui::MenuItem b{ .label = "Wheel buttons (choose one, then press the button)" };
            for (int i = 0; i < ctx_.wheelButtonCount(); ++i)
                b.submenu.push_back({ kWheelLearn + i, ctx_.wheelButton(i), true });
            b.submenu.push_back({ .separator = true });
            b.submenu.push_back({ kWheelDefaults, "The defaults (paddles shift, PS starts, the rest edit)", true });
            items.push_back(std::move(b));
        }
        if (ctx_.autoClutch && ctx_.setAutoClutch)
            items.push_back({ kAutoClutch, std::string("Clutch: automatic (off: yours)") +
                                               (ctx_.autoClutchSays ? " -- " + ctx_.autoClutchSays() : std::string()),
                              true, ctx_.autoClutch(), false, false, true });
        if (ctx_.coldStart && ctx_.setColdStart)
            items.push_back({ kColdStart, "Start the engine cold (from the next launch)", true, ctx_.coldStart(), false,
                              false, true });
        if (ctx_.recorder != nullptr)
        {
            items.push_back({ .separator = true });
            items.push_back({ kRecord, ctx_.recorder->recording() ? "Stop recording" : "Record the output to a file",
                              ctx_.device->isOpen() });
        }
        host_->showMenu(request(*host_, std::move(items)), [this, devices](int id) {
            audio::Request r = ctx_.request;
            if (id == kWheel)
            {
                const bool on = !ctx_.wheelOn();
                ctx_.setWheelOn(on);
                say(on ? "wheel: on (a G29 is read when connected)" : "wheel: off (nothing read from it or sent to it)");
                return;
            }
            if (id == kWheelDefaults || (id >= kWheelLearn && ctx_.wheelButtonCount && id < kWheelLearn + ctx_.wheelButtonCount()))
            {
                ctx_.learnWheelButton(id == kWheelDefaults ? -1 : id - kWheelLearn);
                return;
            }
            if (id == kAutoClutch)
            {
                const bool on = !ctx_.autoClutch();
                ctx_.setAutoClutch(on);
                say(std::string("clutch: ") + (on ? "automatic" : "yours") +
                    (ctx_.autoClutchSays ? " (" + ctx_.autoClutchSays() + ")" : std::string()));
                return;
            }
            if (id == kColdStart)
            {
                const bool cold = !ctx_.coldStart();
                ctx_.setColdStart(cold);
                say(cold ? "start: cold from the next launch (the oil and water at the air's)"
                         : "start: warm from the next launch (the engine at its working temperatures)");
                return;
            }
            if (id == kQuality || id == kQuality + 1)
            {
                ctx_.setQuality(id - kQuality);
                say(id == kQuality ? "quality: full (the engine is compiled again)" : "quality: economy (the engine is "
                                                                                      "compiled again)");
                return;
            }
            if (id == kDevice)
                r.deviceName.clear();
            else if (id > kDevice && id <= kDevice + static_cast<int>(devices.size()))
                r.deviceName = devices[static_cast<std::size_t>(id - kDevice - 1)];
            else if (id >= kRate && id < kRate + static_cast<int>(std::size(kRates)))
                r.rate = kRates[id - kRate];
            else if (id >= kBuffer && id < kBuffer + static_cast<int>(std::size(kBuffers)))
                r.bufferFrames = kBuffers[id - kBuffer];
            else if (id == kRecord)
            {
                if (ctx_.recorder->recording())
                {
                    const bool whole = ctx_.recorder->stop();
                    say(std::string("record: stopped, ") + std::to_string(ctx_.recorder->written()) + " frames"
                        + (whole ? "" : " (the file is not whole: a write failed)"));
                }
                else
                {
                    const std::time_t t = std::time(nullptr);
                    char stamp[32] = "recording";
                    if (const std::tm* tm = std::localtime(&t); tm != nullptr)
                        std::strftime(stamp, sizeof stamp, "recording %Y-%m-%d %H%M%S", tm);
                    std::error_code ec;
                    std::filesystem::create_directories(ctx_.recordFolder, ec);
                    const std::string file = (std::filesystem::path(ctx_.recordFolder) / (std::string(stamp) + ".wav")).string();
                    std::string why;
                    if (ctx_.recorder->start(file, ctx_.device->info().rate, why))
                        say("record: writing " + file);
                    else
                        say("record: " + why);
                }
                return;
            }
            else
                return;
            reopenAudio(r);
        });
    }
}
