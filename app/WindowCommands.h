// app/WindowCommands.h: what the window's keys and menus do with files and the audio device (TheEngineEngine only;
// tee-headless has neither). The menus are FunkGui's (HostServices::showMenu, drawn by the Canvas over the frame); the
// file choosers are the system's (HostServices::chooseFiles: SDL's dialogs).
//
//   Ctrl+N (Cmd on a Mac)    a new build (every field at its default); asks first when the build has unsaved changes
//   Ctrl+O                   open a build (a whole build's file)
//   Ctrl+S, Ctrl+Shift+S     save; save as (an untitled build, or a shipped one, asks for a file)
//   F2                       the File menu: the above, the recent files, the library's builds, and the three parts
//                            (engine, car, tune) loaded or saved apart
//   F9                       the Audio menu: the device by name, the rate, the buffer; the recorder on and off
//
// A change of the audio device closes the stream and opens the new one (the source is prepared again at its rate, with
// the audio stopped); what was chosen goes to the settings file when the window closes. The keys are taken before the
// driving keys (Ctrl+S is not the brake). docs/controls.md lists them.
#pragma once

#include "audio/AudioDevice.h"

#include <filesystem>
#include <functional>
#include <string>

union SDL_Event;

namespace funkgui
{
    class HostServices;
}

namespace tee::audio
{
    class Recorder;
}

namespace tee::files
{
    class BuildFile;
    class BuildLibrary;
}

namespace tee::settings
{
    struct Settings;
}

namespace tee::app
{
    class WindowCommands
    {
    public:
        struct Context
        {
            files::BuildFile* document = nullptr;     // null: no document (not the simulator): the file keys do nothing
            files::BuildLibrary* library = nullptr;
            audio::Device* device = nullptr;          // null: --no-audio
            audio::Request request;                   // what the device was opened with
            audio::Recorder* recorder = nullptr;
            settings::Settings* settings = nullptr;   // what is remembered: changed here, saved by the caller
            std::string recordFolder;                 // where the recorder's files go (the user's folder)
            std::function<void(const std::string&)> say;        // a line for the status bar and the console
            std::function<void()> audioChanged;                 // the device was opened again (its line changed)
            std::function<int()> quality;                       // the engine's quality, 0 full, 1 economy (none: no item)
            std::function<void(int)> setQuality;
            std::function<std::string()> wheel;                 // the racing wheel's line ("on: ..."; empty: no item)
            std::function<bool()> wheelOn;
            std::function<void(bool)> setWheelOn;
            // The wheel's buttons (input/g29/G29Input.h): the line of the i-th bindable action ("shift up: button 5");
            // learn(i): the next button pressed is bound to it; learn(-1): the defaults again. None: no submenu.
            std::function<std::string(int)> wheelButton;
            std::function<int()> wheelButtonCount;
            std::function<void(int)> learnWheelButton;
            std::function<bool()> coldStart;                    // the engine starts cold (the settings'; none: no item)
            std::function<void(bool)> setColdStart;
            // The automatic clutch (the settings' driver.autoClutch, at once) and what it means for the gearbox loaded
            // (Controls::clutchLine). None: no item.
            std::function<bool()> autoClutch;
            std::function<void(bool)> setAutoClutch;
            std::function<std::string()> autoClutchSays;
        };

        explicit WindowCommands(Context context) : ctx_(std::move(context)) {}

        // The host, once it exists (the menus and choosers are its).
        void attach(funkgui::HostServices* host) { host_ = host; }

        // An SDL event before the host reads it: true when it was one of the keys above (taken).
        bool event(const SDL_Event&);

        void newBuild();
        void open();
        void save();
        void saveAs();
        void fileMenu();
        // A text to a file the person chooses (FrameServices::saveText: a dyno run's CSV). The system's dialog answers
        // later, so this returns true once the dialog is open (false, with `error`, when it cannot open), and the file's
        // fate is said on the status line when the person has chosen.
        bool saveText(const std::string& suggestedName, const std::string& text, std::string& error);
        void audioMenu();

        // What the menus changed (the request the device is open with now).
        const Context& context() const noexcept { return ctx_; }

    private:
        void openFile(const std::string& path, int part);
        void reopenAudio(const audio::Request&);
        void say(const std::string& line) const;
        bool shipped(const std::filesystem::path& file) const;      // under the library's shipped folder

        Context ctx_;
        funkgui::HostServices* host_ = nullptr;
    };
}
