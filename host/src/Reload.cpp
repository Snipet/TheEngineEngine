// host/src/Reload.cpp: the state carried across a reload, and the watcher (Reload.h).
#include "tee/Reload.h"

#include "tee/SimHost.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <map>

namespace tee
{
    namespace
    {
        std::size_t carryNodes(const std::vector<Schema::Node>& from, const unsigned char* source,
                               const std::vector<Schema::Node>& to, unsigned char* destination)
        {
            std::size_t carried = 0;
            for (const Schema::Node& field : to)
            {
                const Schema::Node* old = nullptr;
                for (const Schema::Node& candidate : from)
                    if (candidate.name == field.name)
                        old = &candidate;
                if (old == nullptr || old->type != field.type || old->parameter || field.parameter)
                    continue;
                if (field.scalar == Scalar::Pointer)
                    continue;
                if (!field.children.empty() && !old->children.empty())
                {
                    const std::size_t n = field.count();
                    for (std::size_t i = 0; i < n; ++i)
                        carried += carryNodes(old->children, source + old->offset + i * old->stride, field.children,
                                              destination + field.offset + i * field.stride);
                    continue;
                }
                if (field.scalar == Scalar::None || old->size != field.size)
                    continue;
                std::memcpy(destination + field.offset, source + old->offset, field.size);
                ++carried;
            }
            return carried;
        }
    } // namespace

    std::size_t carryState(const Schema& from, const void* source, const Schema& to, void* destination)
    {
        return carryNodes(from.fields(), static_cast<const unsigned char*>(source), to.fields(),
                          static_cast<unsigned char*>(destination));
    }

    std::size_t carryField(const Schema& from, const void* source, const Schema& to, void* destination,
                           std::string_view name)
    {
        std::vector<Schema::Node> a, b;
        for (const Schema::Node& n : from.fields())
            if (n.name == name)
                a.push_back(n);
        for (const Schema::Node& n : to.fields())
            if (n.name == name)
                b.push_back(n);
        return carryNodes(a, static_cast<const unsigned char*>(source), b, static_cast<unsigned char*>(destination));
    }

    bool devMode() noexcept
    {
        const char* v = std::getenv("TEE_DEV");
        return v != nullptr && *v != '\0' && std::strcmp(v, "0") != 0;
    }

    namespace
    {
        // The directory's .fm files with their times and sizes: what a change is.
        Reloader::Listing look(
            const std::filesystem::path& directory)
        {
            Reloader::Listing out;
            std::error_code ec;
            for (auto it = std::filesystem::recursive_directory_iterator(directory, ec);
                 !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
            {
                if (!it->is_regular_file(ec) || it->path().extension() != ".fm")
                    continue;
                out[it->path().generic_string()] = {it->last_write_time(ec), it->file_size(ec)};
            }
            return out;
        }
    } // namespace

    Reloader::Reloader(std::filesystem::path directory, Sim::Options options)
        : directory_(std::move(directory)), options_(std::move(options))
    {
        // the first look is taken here, so that an edit made as soon as this returns is a change
        thread_ = std::thread([this, seen = look(directory_)]() mutable { loop(std::move(seen)); });
    }

    Reloader::~Reloader()
    {
        stop_.store(true);
        if (thread_.joinable())
            thread_.join();
    }

    void Reloader::loop(Listing seen)
    {
        while (!stop_.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            auto now = look(directory_);
            if (now == seen)
                continue;
            seen = now;
            std::string errors;
            std::shared_ptr<Sim> sim = Sim::compile(directory_, options_, errors);
            const std::lock_guard<std::mutex> lock(mutex_);
            ready_ = sim;
            errors_ = sim ? std::string() : errors;
            compiles_.fetch_add(1);
        }
    }

    bool Reloader::poll(SimHost& host)
    {
        std::shared_ptr<Sim> sim;
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            sim.swap(ready_);
        }
        if (!sim)
            return false;
        std::string errors;
        if (!host.reload(sim, errors))
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            errors_ = errors;
            return false;
        }
        return true;
    }

    std::string Reloader::errors() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return errors_;
    }
} // namespace tee
