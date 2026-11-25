// assistant/include/tee/assistant/Chat.h: the in-app chat by the owner's API key (docs/plan/w3-a-assistant.md, "The key
// path"): the conversation, its turn loop over the Messages API, and ChatPort, what its view polls.
//
// The request (checked line by line against the Claude API reference at the time of writing, through the claude-api
// skill's raw-HTTP guidance; C++ has no official SDK), each line said once, in Chat.cpp's buildRequest:
//   POST {url}, headers content-type, x-api-key (the key: only here, never logged or kept elsewhere),
//   anthropic-version 2026-10-21, anthropic-beta server-side-fallback-2026-07-01;
//   body: model (kDefaultModel, the one constant), max_tokens, stream true, system (byte-stable for the chat's life),
//   tools (the tool table's, plus ask_user; byte-stable), messages, cache_control ephemeral (the API places it on the
//   last cacheable block), output_config.effort, fallbacks "default". No thinking parameter (the model's adaptive
//   thinking is always on), no tool_choice (forced tool use is refused by the model), no sampling parameters.
//
// The loop's rules, each tested on a scripted transport (tests/assistant/ChatTest.cpp):
//   - the app's state rides in the newest user message only; the system prompt never changes;
//   - every block the model returns is appended back unchanged (thinking and its signature included): the history is
//     append-only. After a fallback, the blocks the reference says to omit before the last fallback block (thinking,
//     redacted_thinking, tool_use, unknown model-internal ones) are omitted when the message is echoed;
//   - a tool's input is parsed strictly and validated against its schema (Tools.h) before it runs; invalid JSON is
//     answered with an INVALID_JSON error result, never run;
//   - nothing runs after max_tokens or a refusal: a refusal's partial reply is dropped whole; a reply cut at max_tokens
//     is kept and each of its tool calls answered "not run";
//   - on cancel the partial reply is dropped whole, and every tool call already appended gets a result;
//   - an empty reply is never appended (TestingPlayground's chat-audit TURN-2);
//   - retries: 408, 409, 429, 500, 502, 503, 504, 529, a broken connection, an error event mid-stream (overloaded or
//     api_error), unless x-should-retry is false or a 429 is the spend cap; retry-after honoured up to a ceiling (above
//     it: refused, with when to try again); at most `attempts` in all. A retry repeats the round from the start: nothing
//     of a failed attempt is kept;
//   - a turn's limits: the tools' Turn (calls, seconds, result bytes) and the output tokens; one driver at a time
//     (Claude Desktop's calls refuse the chat while they run, and the other way round).
//
// Threads: ChatPort's methods from the frame's (the view's) thread; the turn on a worker of the chat's own.
#pragma once

#include "tee/assistant/Tools.h"
#include "tee/assistant/Transport.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tee
{
    class Fields;
}

namespace tee::assistant
{
    // The default model: the one place a model id is written (the owner's answer 5, 25 Nov).
    inline constexpr const char* kDefaultModel = "claude-opus-5-5";
    inline constexpr const char* kApiUrl = "https://api.anthropic.com/v1/messages";
    inline constexpr const char* kApiVersion = "2026-10-21";
    inline constexpr const char* kFallbackBeta = "server-side-fallback-2026-07-01";

    // One line of the conversation as the view shows it.
    struct ChatItem
    {
        enum class Kind
        {
            User,
            Assistant,      // Claude's words (streaming while the reply comes)
            Tool,           // a tool Claude used: its name and what came of it
            Notice,         // the chat's own words: a refusal, a cut reply, an error, a limit, a cancel
        };
        Kind kind = Kind::Notice;
        std::string text;
        std::string tool;               // Kind::Tool
        bool error = false;             // Kind::Tool and Kind::Notice
        bool streaming = false;         // Kind::Assistant, while it arrives
    };

    // What the chat's view needs (TestingPlayground's ChatPort, in shape): it polls revision() each frame and reads
    // the rest when it moved.
    class ChatPort
    {
    public:
        virtual ~ChatPort() = default;
        virtual std::uint64_t revision() const = 0;
        virtual std::vector<ChatItem> items() const = 0;
        virtual bool busy() const = 0;                       // a turn runs
        virtual bool ready(std::string& why) const = 0;      // a message could be sent now (a key, a transport)
        virtual bool send(const std::string& text, std::string& why) = 0;
        virtual void stop() = 0;                             // the turn running cancelled
        virtual std::string question() const = 0;           // Claude's question waiting for the owner ("" none)
        virtual bool answer(const std::string& text) = 0;    // the owner's answer to it
    };

    struct ChatOptions
    {
        std::string url = kApiUrl;
        std::string model = kDefaultModel;
        int maxTokens = 32000;
        std::string effort = "medium";
        int attempts = 3;                       // tries of one round in all
        double retryCeilingSeconds = 30.0;      // a retry-after above it is not waited for
        long long turnOutputTokens = 200000;    // output tokens of a turn in all
        int continuations = 5;                  // pause_turn resumes in one turn
        double askSeconds = 300.0;              // the owner's answer to ask_user
        // Waits `seconds` unless `cancel` is raised first (tests give one that does not wait). Empty: the real clock.
        std::function<void(double seconds, const std::atomic<bool>& cancel)> sleep;
    };

    class Chat final : public ChatPort
    {
    public:
        // `key`: the owner's key, read when a request is made (KeyStore.h); empty string: none. `systemPrompt`: fixed
        // for the chat's life (systemPromptFor).
        Chat(Tools& tools, Drivers& drivers, Transport* transport, std::function<std::string()> key,
             std::string systemPrompt, std::function<double()> clock, ChatOptions options = {});
        ~Chat() override;
        Chat(const Chat&) = delete;
        Chat& operator=(const Chat&) = delete;

        std::uint64_t revision() const override;
        std::vector<ChatItem> items() const override;
        bool busy() const override;
        bool ready(std::string& why) const override;
        bool send(const std::string& text, std::string& why) override;
        void stop() override;
        std::string question() const override;
        bool answer(const std::string& text) override;

        // The history as it would be sent (a copy), for tests and for nothing else.
        nlohmann::json history() const;
        // Waits for the turn to end (tests).
        void wait();

    private:
        struct Round;
        void turn(std::string text);
        bool round(Round& r, const std::string& requestBody);
        std::string buildRequest() const;
        nlohmann::json toolDefinitions() const;
        Result runTool(Turn& t, const std::string& name, const nlohmann::json& input, bool inputValid,
                       const std::string& rawInput);
        void add(ChatItem item);
        void notice(const std::string& text, bool error = true);
        void sleepFor(double seconds);

        Tools& tools_;
        Drivers& drivers_;
        Transport* transport_;
        std::function<std::string()> key_;
        const std::string system_;
        std::function<double()> clock_;
        ChatOptions options_;
        const std::string toolsJson_;           // made once: byte-stable

        mutable std::mutex mutex_;
        nlohmann::json history_ = nlohmann::json::array();
        std::vector<ChatItem> items_;
        std::uint64_t revision_ = 0;
        std::string question_;
        std::string answer_;
        bool answered_ = false;
        bool busy_ = false;

        std::atomic<bool> cancel_{false};
        std::thread worker_;
    };

    // This app's system prompt: what the app is, how to work (change little, say what changed, tell measured from
    // assumed), and the build's sections with their fields' labels, from the program's Schema. Fixed for a chat's life.
    std::string systemPromptFor(const Fields& fields);

    // Server-sent events, a line at a time: each whole event (its type and data) handed on.
    class SseParser
    {
    public:
        // `onEvent(type, data)`; returning false stops the parse.
        bool feed(std::string_view bytes, const std::function<bool(const std::string&, const std::string&)>& onEvent);

    private:
        std::string line_, event_, data_;
        bool hasData_ = false;
    };

    // A message's content echoed back after a fallback (the reference's rule): before the last "fallback" block,
    // thinking, redacted_thinking, tool_use and any block type not known to be the conversation's own are omitted;
    // the fallback block stays where it is; everything after it is kept. Without a fallback block: unchanged.
    nlohmann::json echoContent(const nlohmann::json& content);
}
