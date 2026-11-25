// tests/assistant/ChatTest.cpp: the key chat's turn loop (assistant/.../Chat.h) on a scripted transport that replays
// streams of the Messages API's shape (TestingPlayground's FakeClaude, in pattern), the tools real (Rig.h: a simulator,
// a document, a library on the simulated clock). No connection is opened and no key exists: the chat's key is a
// placeholder string, checked never to leave the headers.
#include "Rig.h"
#include "tee/assistant/Chat.h"

#include <catch2/catch_test_macros.hpp>

#include <deque>
#include <mutex>

using namespace tee;
using namespace tee::test;
using namespace tee::assistant;
using nlohmann::json;

namespace
{
    const std::string kKey = "placeholder-not-a-key";

    std::string sse(const std::string& type, const json& data)
    {
        return "event: " + type + "\ndata: " + data.dump() + "\n\n";
    }

    // A reply's stream, built block by block.
    struct Stream
    {
        std::vector<std::string> chunks;
        int index = 0;

        Stream()
        {
            chunks.push_back(sse("message_start", {{"type", "message_start"},
                                                   {"message", {{"id", "msg_test"}, {"type", "message"}, {"role", "assistant"},
                                                                {"model", kDefaultModel}, {"content", json::array()},
                                                                {"usage", {{"input_tokens", 10}, {"output_tokens", 1}}}}}}));
            chunks.push_back(": a comment, as a keep-alive might be\n\n");
            chunks.push_back(sse("ping", {{"type", "ping"}}));
        }
        Stream& text(const std::string& t)
        {
            start({{"type", "text"}, {"text", ""}});
            const std::size_t half = t.size() / 2;
            delta({{"type", "text_delta"}, {"text", t.substr(0, half)}});
            delta({{"type", "text_delta"}, {"text", t.substr(half)}});
            return stop();
        }
        Stream& thinking(const std::string& t, const std::string& signature)
        {
            start({{"type", "thinking"}, {"thinking", ""}});
            delta({{"type", "thinking_delta"}, {"thinking", t}});
            delta({{"type", "signature_delta"}, {"signature", signature}});
            return stop();
        }
        Stream& tool(const std::string& id, const std::string& name, const std::string& inputJson)
        {
            start({{"type", "tool_use"}, {"id", id}, {"name", name}, {"input", json::object()}});
            const std::size_t third = inputJson.size() / 3;
            delta({{"type", "input_json_delta"}, {"partial_json", inputJson.substr(0, third)}});
            delta({{"type", "input_json_delta"}, {"partial_json", inputJson.substr(third)}});
            return stop();
        }
        Stream& fallback()
        {
            start({{"type", "fallback"}, {"from", {{"model", "a"}}}, {"to", {{"model", "b"}}}});
            return stop();
        }
        Stream& end(const std::string& stopReason, json details = nullptr)
        {
            json d = {{"stop_reason", stopReason}};
            if (!details.is_null())
                d["stop_details"] = details;
            chunks.push_back(sse("message_delta", {{"type", "message_delta"}, {"delta", d}, {"usage", {{"output_tokens", 50}}}}));
            chunks.push_back(sse("message_stop", {{"type", "message_stop"}}));
            return *this;
        }
        Stream& error(const std::string& type)
        {
            chunks.push_back(sse("error", {{"type", "error"}, {"error", {{"type", type}, {"message", "busy"}}}}));
            return *this;
        }

    private:
        void start(const json& block)
        {
            chunks.push_back(sse("content_block_start", {{"type", "content_block_start"}, {"index", index}, {"content_block", block}}));
        }
        void delta(const json& d)
        {
            chunks.push_back(sse("content_block_delta", {{"type", "content_block_delta"}, {"index", index}, {"delta", d}}));
        }
        Stream& stop()
        {
            chunks.push_back(sse("content_block_stop", {{"type", "content_block_stop"}, {"index", index}}));
            ++index;
            return *this;
        }
    };

    struct Scripted
    {
        int status = 200;
        std::vector<std::pair<std::string, std::string>> headers;
        std::vector<std::string> chunks;
        bool cut = false;               // the connection breaks after the chunks
        bool hang = false;              // waits for the cancel after the chunks
    };

    Scripted ok(const Stream& s)
    {
        return Scripted{200, {}, s.chunks};
    }

    Scripted status(int code, const std::string& errorType, std::vector<std::pair<std::string, std::string>> headers = {})
    {
        return Scripted{code, std::move(headers),
                        {json({{"type", "error"}, {"error", {{"type", errorType}, {"message", "said " + errorType}}}}).dump()}};
    }

    // FakeClaude: replays the script, records each request.
    class ScriptedTransport final : public Transport
    {
    public:
        std::deque<Scripted> script;
        std::vector<HttpRequest> requests;
        std::mutex mutex;

        bool post(const HttpRequest& r, const BodySink& body, HttpResponse& out, const std::atomic<bool>* cancel) override
        {
            Scripted s;
            {
                std::lock_guard<std::mutex> lock(mutex);
                requests.push_back(r);
                if (script.empty())
                {
                    out.error = "no script left";
                    return false;
                }
                s = script.front();
                script.pop_front();
            }
            out = HttpResponse{};
            out.status = s.status;
            out.headers = s.headers;
            for (const std::string& c : s.chunks)
            {
                if (cancel != nullptr && cancel->load())
                {
                    out.cancelled = true;
                    return false;
                }
                if (!body(s.status, c))
                    return true;   // the sink stopped (an error event): the exchange ended
            }
            if (s.hang)
            {
                while (cancel == nullptr || !cancel->load())
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                out.cancelled = true;
                return false;
            }
            if (s.cut)
            {
                out.error = "connection reset";
                return false;
            }
            return true;
        }
        bool get(const HttpRequest&, const BodySink&, HttpResponse& out, const std::atomic<bool>*) override
        {
            out.error = "not scripted";
            return false;
        }

        json body(std::size_t i)
        {
            std::lock_guard<std::mutex> lock(mutex);
            return json::parse(requests.at(i).body);
        }
        std::size_t count()
        {
            std::lock_guard<std::mutex> lock(mutex);
            return requests.size();
        }
    };

    struct Bench
    {
        Rig rig;
        Drivers drivers;
        ScriptedTransport transport;
        std::vector<double> slept;
        std::unique_ptr<Chat> chat;

        explicit Bench(const std::string& name, Limits limits = {}, ChatOptions options = {})
            : rig(name, limits)
        {
            options.sleep = [this](double s, const std::atomic<bool>&) { slept.push_back(s); };
            chat = std::make_unique<Chat>(*rig.tools, drivers, &transport, [] { return kKey; },
                                          systemPromptFor(rig.build().fields()), [this] { return rig.clock; }, options);
        }

        // Sends and runs the frame until the turn has ended; `during` is called each frame.
        void say(const std::string& text, const std::function<void()>& during = {})
        {
            std::string why;
            REQUIRE(chat->send(text, why));
            while (chat->busy())
            {
                rig.frame();
                if (during)
                    during();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            chat->wait();
        }

        std::vector<ChatItem> notices()
        {
            std::vector<ChatItem> out;
            for (const ChatItem& i : chat->items())
                if (i.kind == ChatItem::Kind::Notice)
                    out.push_back(i);
            return out;
        }
    };

    // Each tool_use in the history has its tool_result in the next message, and the roles alternate from a user.
    void checkWellFormed(const json& messages)
    {
        REQUIRE(messages.is_array());
        for (std::size_t i = 0; i < messages.size(); ++i)
        {
            CHECK(messages[i]["role"] == (i % 2 == 0 ? "user" : "assistant"));
            if (messages[i]["role"] != "assistant")
                continue;
            std::set<std::string> uses;
            for (const json& b : messages[i]["content"])
                if (b["type"] == "tool_use")
                    uses.insert(b["id"].get<std::string>());
            if (uses.empty())
                continue;
            REQUIRE(i + 1 < messages.size());
            std::set<std::string> results;
            for (const json& b : messages[i + 1]["content"])
                if (b["type"] == "tool_result")
                    results.insert(b["tool_use_id"].get<std::string>());
            CHECK(results == uses);
        }
    }

    std::string prefixBeforeMessages(const std::string& body)
    {
        return body.substr(0, body.find("\"messages\":"));
    }
}

TEST_CASE("assistant: the chat's request, line by line, and the key only in its header", "[assistant][sharedsim]")
{
    Bench b("chat-shape");
    b.transport.script.push_back(ok(Stream().text("A V8 is open.").end("end_turn")));
    b.say("What is open?");
    REQUIRE(b.transport.count() == 1);
    const HttpRequest& r = b.transport.requests[0];
    CHECK(r.url == kApiUrl);
    std::map<std::string, std::string> h(r.headers.begin(), r.headers.end());
    CHECK(h["anthropic-version"] == "2026-10-21");
    CHECK(h["anthropic-beta"] == "server-side-fallback-2026-07-01");
    CHECK(h["content-type"] == "application/json");
    CHECK(h["x-api-key"] == kKey);
    const json body = b.transport.body(0);
    CHECK(body["model"] == kDefaultModel);
    CHECK(body["stream"] == true);
    CHECK(body["max_tokens"].get<int>() > 1000);
    CHECK(body["fallbacks"] == "default");
    CHECK(body["cache_control"]["type"] == "ephemeral");
    CHECK(body["output_config"]["effort"] == "medium");
    for (const char* absent : {"thinking", "tool_choice", "temperature", "top_p", "top_k"})
        CHECK_FALSE(body.contains(absent));
    CHECK(body["system"][0]["text"] == systemPromptFor(b.rig.build().fields()));
    std::set<std::string> names;
    for (const json& t : body["tools"])
    {
        names.insert(t["name"].get<std::string>());
        CHECK(t.contains("input_schema"));
        CHECK_FALSE(t.contains("eager_input_streaming"));
    }
    CHECK(names.count("ask_user") == 1);
    CHECK(names.size() == b.rig.tools->list().size() + 1);
    // the state rides in the user's message, before their words
    const json& user = body["messages"][0];
    CHECK(user["role"] == "user");
    CHECK(user["content"][0]["text"].get<std::string>().rfind("[The app's state", 0) == 0);
    CHECK(user["content"][1]["text"] == "What is open?");
    // the key: in its header and nowhere else
    CHECK(r.body.find(kKey) == std::string::npos);
    for (const ChatItem& i : b.chat->items())
        CHECK(i.text.find(kKey) == std::string::npos);
    const auto items = b.chat->items();
    REQUIRE(items.size() == 2);
    CHECK(items[1].kind == ChatItem::Kind::Assistant);
    CHECK(items[1].text == "A V8 is open.");
    CHECK_FALSE(items[1].streaming);
}

TEST_CASE("assistant: a tool turn: thinking echoed unchanged, the tool run once, one result message, the prefix stable",
          "[assistant][sharedsim]")
{
    Bench b("chat-tools");
    const auto fields = b.rig.settable(1);
    const std::string path = shortPath(*fields[0].first);
    const std::string input = json({{"changes", {{{"path", path}, {"value", fields[0].second}}}}}).dump();
    b.transport.script.push_back(
        ok(Stream().thinking("", "sig-A").text("I will change it.").tool("toolu_1", "set_fields", input).end("tool_use")));
    b.transport.script.push_back(ok(Stream().text("Done.").end("end_turn")));
    b.transport.script.push_back(ok(Stream().text("Still so.").end("end_turn")));
    const std::uint64_t stepBefore = b.rig.build().undoStep();
    b.say("Change it.");
    REQUIRE(b.transport.count() == 2);
    CHECK(b.rig.build().undoStep() != stepBefore);   // ran, once: one step
    REQUIRE(b.rig.build().undo());
    CHECK(b.rig.build().undoStep() == stepBefore);
    const json second = b.transport.body(1);
    const json& assistant = second["messages"][1];
    REQUIRE(assistant["role"] == "assistant");
    CHECK(assistant["content"][0] == json({{"type", "thinking"}, {"thinking", ""}, {"signature", "sig-A"}}));
    CHECK(assistant["content"][1] == json({{"type", "text"}, {"text", "I will change it."}}));
    CHECK(assistant["content"][2]["input"] == json::parse(input));
    const json& results = second["messages"][2];
    REQUIRE(results["content"].size() == 1);
    CHECK(results["content"][0]["tool_use_id"] == "toolu_1");
    CHECK_FALSE(results["content"][0].contains("is_error"));
    checkWellFormed(second["messages"]);
    // a second turn: the system and the tools byte for byte the same, the history before it unchanged
    b.say("Is it still so?");
    REQUIRE(b.transport.count() == 3);
    CHECK(prefixBeforeMessages(b.transport.requests[0].body) == prefixBeforeMessages(b.transport.requests[2].body));
    const json third = b.transport.body(2);
    for (std::size_t i = 0; i < second["messages"].size(); ++i)
        CHECK(third["messages"][i] == second["messages"][i]);
    CHECK(third["messages"].back()["content"][1]["text"] == "Is it still so?");
    checkWellFormed(third["messages"]);
}

TEST_CASE("assistant: invalid tool input is answered INVALID_JSON and never run", "[assistant][sharedsim]")
{
    Bench b("chat-invalid");
    b.transport.script.push_back(ok(Stream().tool("toolu_x", "set_fields", "{\"changes\": [{\"path\": \"engine.b").end("tool_use")));
    b.transport.script.push_back(ok(Stream().text("Sorry.").end("end_turn")));
    const std::uint64_t step = b.rig.build().undoStep();
    b.say("Change it.");
    CHECK(b.rig.build().undoStep() == step);
    const json second = b.transport.body(1);
    const json& tr = second["messages"][2]["content"][0];
    CHECK(tr["is_error"] == true);
    CHECK(json::parse(tr["content"].get<std::string>()).contains("INVALID_JSON"));
    checkWellFormed(second["messages"]);
}

TEST_CASE("assistant: nothing runs after max_tokens or a refusal", "[assistant][sharedsim]")
{
    Bench b("chat-stops");
    const auto fields = b.rig.settable(1);
    const std::string input =
        json({{"changes", {{{"path", shortPath(*fields[0].first)}, {"value", fields[0].second}}}}}).dump();
    const std::uint64_t step = b.rig.build().undoStep();
    // cut at max_tokens with a whole-looking call: kept, answered "not run"
    b.transport.script.push_back(ok(Stream().text("Changing").tool("toolu_m", "set_fields", input).end("max_tokens")));
    b.say("Change it.");
    CHECK(b.rig.build().undoStep() == step);
    CHECK_FALSE(b.notices().empty());
    {
        const json h = b.chat->history();
        REQUIRE(h.size() == 3);
        CHECK(h[2]["content"][0]["is_error"] == true);
    }
    // a refusal mid-stream, with a call: dropped whole, never run
    b.transport.script.push_back(ok(Stream().text("Sure, ").tool("toolu_r", "set_fields", input).end("refusal", {{"category", "cyber"}})));
    b.say("Again.");
    CHECK(b.rig.build().undoStep() == step);
    const json h = b.chat->history();
    CHECK(h.size() == 3);   // nothing appended: the new words joined the last user message
    CHECK(b.notices().back().text.find("declined") != std::string::npos);
    CHECK(b.notices().back().text.find("cyber") != std::string::npos);
    for (const ChatItem& i : b.chat->items())
        CHECK(i.text.find("Sure,") == std::string::npos);
    // the next request is well formed: roles alternate, every call answered
    b.transport.script.push_back(ok(Stream().text("OK.").end("end_turn")));
    b.say("Then just tell me.");
    checkWellFormed(b.transport.body(b.transport.count() - 1)["messages"]);
}

TEST_CASE("assistant: a cancel drops the partial reply whole and answers every call already appended", "[assistant][sharedsim]")
{
    Bench b("chat-cancel");
    // cancelled mid-stream
    Scripted s = ok(Stream().text("Half of a"));
    s.chunks.resize(6);   // the message_start, a comment, a ping, the text's start and two deltas: then it hangs
    s.hang = true;
    b.transport.script.push_back(s);
    bool stopped = false;
    b.say("Tell me a long story.", [&] {
        if (!stopped && b.transport.count() == 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            b.chat->stop();
            stopped = true;
        }
    });
    CHECK(b.chat->history().size() == 1);   // the user's message alone
    for (const ChatItem& i : b.chat->items())
        CHECK(i.kind != ChatItem::Kind::Assistant);
    // cancelled while Claude's question waits for the owner: the call gets a result
    b.transport.script.push_back(ok(Stream().tool("toolu_q", "ask_user", R"({"question":"Which one?"})").tool("toolu_s", "get_status", "{}").end("tool_use")));
    bool asked = false;
    b.say("Change something.", [&] {
        if (!asked && !b.chat->question().empty())
        {
            asked = true;
            b.chat->stop();
        }
    });
    CHECK(asked);
    const json h = b.chat->history();
    checkWellFormed(h);
    REQUIRE(h.back()["content"].size() == 2);
    CHECK(h.back()["content"][0]["is_error"] == true);
    CHECK(h.back()["content"][1]["is_error"] == true);
    CHECK(b.chat->question().empty());
}

TEST_CASE("assistant: ask_user puts Claude's question to the owner and returns the answer", "[assistant][sharedsim]")
{
    Bench b("chat-ask");
    b.transport.script.push_back(ok(Stream().tool("toolu_q", "ask_user", R"({"question":"Which build?"})").end("tool_use")));
    b.transport.script.push_back(ok(Stream().text("Thanks.").end("end_turn")));
    b.say("Load one.", [&] {
        if (b.chat->question() == "Which build?")
            b.chat->answer("the V10");
    });
    const json second = b.transport.body(1);
    CHECK(json::parse(second["messages"][2]["content"][0]["content"].get<std::string>())["answer"] == "the V10");
}

TEST_CASE("assistant: retries: when, how often, how long; nothing of a failed attempt kept", "[assistant][sharedsim]")
{
    {
        Bench b("chat-retry");
        b.transport.script.push_back(status(529, "overloaded_error"));
        b.transport.script.push_back(status(429, "rate_limit_error", {{"retry-after", "2"}}));
        b.transport.script.push_back(ok(Stream().text("Here.").end("end_turn")));
        b.say("Hello.");
        CHECK(b.transport.count() == 3);
        CHECK(b.transport.requests[0].body == b.transport.requests[2].body);
        REQUIRE(b.slept.size() == 2);
        CHECK(b.slept[1] == 2.0);
        CHECK(b.chat->history().size() == 2);
    }
    {
        // a retry-after over the ceiling: not waited for, and the owner told when
        Bench b("chat-retry-long");
        b.transport.script.push_back(status(429, "rate_limit_error", {{"retry-after", "120"}}));
        b.say("Hello.");
        CHECK(b.transport.count() == 1);
        CHECK(b.notices().back().text.find("120") != std::string::npos);
    }
    {
        // x-should-retry false, a 400, a 401: one request each, the API's words (and never the key) shown
        Bench b("chat-noretry");
        b.transport.script.push_back(status(500, "api_error", {{"x-should-retry", "false"}}));
        b.say("Hello.");
        b.transport.script.push_back(status(400, "invalid_request_error"));
        b.say("Hello.");
        b.transport.script.push_back(status(401, "authentication_error"));
        b.say("Hello.");
        CHECK(b.transport.count() == 3);
        const auto n = b.notices();
        REQUIRE(n.size() == 3);
        CHECK(n[1].text.find("said invalid_request_error") != std::string::npos);
        CHECK(n[2].text.find("refused") != std::string::npos);
        for (const ChatItem& i : n)
            CHECK(i.text.find(kKey) == std::string::npos);
    }
    {
        // an error mid-stream (overloaded) and a broken connection: the round again from the start, its text once
        Bench b("chat-midstream");
        Scripted broken = ok(Stream().text("Partial "));
        broken.chunks.resize(6);
        broken.chunks.push_back(Stream().error("overloaded_error").chunks.back());
        b.transport.script.push_back(broken);
        Scripted cut = ok(Stream().text("Partial "));
        cut.chunks.resize(6);
        cut.cut = true;
        b.transport.script.push_back(cut);
        b.transport.script.push_back(ok(Stream().text("Whole answer.").end("end_turn")));
        b.say("Hello.");
        CHECK(b.transport.count() == 3);
        int assistantItems = 0;
        for (const ChatItem& i : b.chat->items())
            if (i.kind == ChatItem::Kind::Assistant)
            {
                ++assistantItems;
                CHECK(i.text == "Whole answer.");
            }
        CHECK(assistantItems == 1);
        CHECK(b.chat->history().size() == 2);
    }
}

TEST_CASE("assistant: after a fallback, the echo omits what came before the boundary, and only later calls run",
          "[assistant][sharedsim]")
{
    // the rule itself
    const json content = json::array({{{"type", "thinking"}, {"thinking", ""}, {"signature", "s"}},
                                      {{"type", "text"}, {"text", "a"}},
                                      {{"type", "tool_use"}, {"id", "t0"}, {"name", "get_status"}, {"input", json::object()}},
                                      {{"type", "redacted_thinking"}, {"data", "x"}},
                                      {{"type", "fallback"}, {"from", {{"model", "a"}}}, {"to", {{"model", "b"}}}},
                                      {{"type", "thinking"}, {"thinking", ""}, {"signature", "t"}},
                                      {{"type", "tool_use"}, {"id", "t1"}, {"name", "get_status"}, {"input", json::object()}}});
    const json echoed = echoContent(content);
    REQUIRE(echoed.size() == 4);
    CHECK(echoed[0]["type"] == "text");
    CHECK(echoed[1]["type"] == "fallback");
    CHECK(echoed[2]["signature"] == "t");
    CHECK(echoed[3]["id"] == "t1");
    CHECK(echoContent(json::array({{{"type", "thinking"}, {"signature", "s"}}})).size() == 1);   // no boundary: unchanged

    // in a turn
    Bench b("chat-fallback");
    b.transport.script.push_back(ok(Stream()
                                        .thinking("", "sig-A")
                                        .text("a")
                                        .tool("toolu_0", "get_status", "{}")
                                        .fallback()
                                        .text("b")
                                        .tool("toolu_1", "get_status", "{}")
                                        .end("tool_use")));
    b.transport.script.push_back(ok(Stream().text("Done.").end("end_turn")));
    b.say("Status?");
    const json second = b.transport.body(1);
    const json& a = second["messages"][1]["content"];
    for (const json& blk : a)
    {
        CHECK(blk.value("signature", "") != "sig-A");
        CHECK(blk.value("id", "") != "toolu_0");
    }
    const json& results = second["messages"][2]["content"];
    REQUIRE(results.size() == 1);
    CHECK(results[0]["tool_use_id"] == "toolu_1");
    checkWellFormed(second["messages"]);
}

TEST_CASE("assistant: the chat's turn limits, one driver at a time, an empty reply, a pause, no key", "[assistant][sharedsim]")
{
    {
        Limits l;
        l.turnCalls = 2;
        Bench b("chat-limits", l);
        for (int i = 0; i < 3; ++i)
            b.transport.script.push_back(ok(Stream().tool("toolu_" + std::to_string(i), "get_status", "{}").end("tool_use")));
        b.transport.script.push_back(ok(Stream().text("Enough.").end("end_turn")));
        b.say("Look three times.");
        const json last = b.transport.body(3);
        const json& third = last["messages"][6]["content"][0];
        CHECK(third["is_error"] == true);
        CHECK(third["content"].get<std::string>().find("tool calls") != std::string::npos);
    }
    {
        ChatOptions o;
        o.turnOutputTokens = 60;   // each scripted reply reports 50
        Bench b("chat-tokens", {}, o);
        for (int i = 0; i < 3; ++i)
            b.transport.script.push_back(ok(Stream().tool("toolu_" + std::to_string(i), "get_status", "{}").end("tool_use")));
        b.say("Look.");
        CHECK(b.transport.count() == 2);
        CHECK(b.notices().back().text.find("tokens") != std::string::npos);
        checkWellFormed(b.chat->history());
    }
    {
        Bench b("chat-driver");
        std::string why;
        REQUIRE(b.drivers.take(Driver::Desktop, why));
        b.say("Hello.");
        CHECK(b.transport.count() == 0);
        CHECK(b.notices().back().text.find("Claude Desktop") != std::string::npos);
        b.drivers.release(Driver::Desktop);
    }
    {
        // an empty reply is never appended; a pause resumes without a new user message
        Bench b("chat-empty");
        b.transport.script.push_back(ok(Stream().end("end_turn")));
        b.say("Hello?");
        CHECK(b.chat->history().size() == 1);
        b.transport.script.push_back(ok(Stream().text("Working").end("pause_turn")));
        b.transport.script.push_back(ok(Stream().text(" on it.").end("end_turn")));
        b.say("Hello again.");
        const json last = b.transport.body(2);
        CHECK(last["messages"].size() == 2);   // the user's (both messages joined) and the paused reply
        CHECK(last["messages"][1]["content"][0]["text"] == "Working");
        CHECK(b.chat->history().size() == 3);
    }
    {
        // no key, no transport: refused at once, in words
        Rig rig("chat-nokey");
        Drivers d;
        Chat noKey(*rig.tools, d, nullptr, [] { return std::string(); }, "x", [] { return 0.0; });
        std::string why;
        CHECK_FALSE(noKey.send("Hello.", why));
        CHECK(why.find("Claude API") != std::string::npos);
        ScriptedTransport t;
        Chat noKey2(*rig.tools, d, &t, [] { return std::string(); }, "x", [] { return 0.0; });
        CHECK_FALSE(noKey2.send("Hello.", why));
        CHECK(why.find("key") != std::string::npos);
        CHECK(t.count() == 0);
    }
}

TEST_CASE("assistant: the system prompt is the app's own: short, and built from the Schema", "[assistant][sharedsim]")
{
    Rig rig("chat-system");
    const std::string s = systemPromptFor(rig.build().fields());
    CHECK(s.size() < 12000);
    CHECK(s.find("Tell measured from assumed") != std::string::npos);
    CHECK(s.find("- engine: ") != std::string::npos);
    CHECK(s == systemPromptFor(rig.build().fields()));   // the same every time it is made
    for (const FieldInfo& f : rig.build().fields().all())
        if (f.hidden && f.block == "spec" && !f.label.empty() && f.label.size() > 12)
        {
            // (a hidden field's label is not offered: a label shared with a visible field may still appear)
            bool shared = false;
            for (const FieldInfo& g : rig.build().fields().all())
                shared = shared || (!g.hidden && g.label == f.label);
            if (!shared)
                CHECK(s.find(f.label) == std::string::npos);
        }
}
