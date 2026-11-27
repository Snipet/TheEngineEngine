// assistant/include/tee/assistant/Json.h: the one way the assistant writes JSON as text (X-063).
//
// nlohmann's dump() throws type_error 316 on a string that is not valid UTF-8, and nothing in the assistant catches it:
// a build named by a file whose name is not UTF-8 (Linux allows any bytes) ended the app on the chat's worker or the
// link's thread. dumpText() writes such a byte as U+FFFD instead, the same bytes every time (so a request's earlier
// messages are still the next one's first bytes), and the rest unchanged. Every serialisation in assistant/ goes
// through it; a test (InboxTest.cpp) fails on any `.dump(` anywhere else in assistant/.
#pragma once

#include <string>

namespace tee::assistant
{
    template <class Json>
    std::string dumpText(const Json& j)
    {
        return j.dump(-1, ' ', false, Json::error_handler_t::replace);
    }
}
