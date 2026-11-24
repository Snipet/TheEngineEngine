// tests/sim/valvetrain/Lab.h: what card E's standalone tests share (namespace tee::test::lab): a program compiled from
// sim/ (the new simulator's files, no engine needed), instances of its modules with their fields' defaults written,
// and calls of their methods by name.
#pragma once

#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <memory>
#include <string>

namespace tee::test::lab
{
    // One program, compiled once a process for each entry file under sim/ ("valvetrain/lab.fm").
    inline tee::Program& program(const std::string& entry)
    {
        static std::map<std::string, std::unique_ptr<tee::Program>> programs;
        auto& slot = programs[entry];
        if (!slot)
        {
            std::string errors;
            tee::Program::Options options;
            options.optimisation = 2;
            slot = tee::Program::fromDirectory(TEE_SOURCE_DIR "/sim", entry, options, errors);
            INFO(errors);
            REQUIRE(slot != nullptr);
            REQUIRE(slot->link(errors));
        }
        return *slot;
    }

    // An instance of a module of the program, with the schema to reach its fields by path.
    struct Obj
    {
        tee::Program* prog = nullptr;
        std::unique_ptr<tee::Schema> schema;
        std::unique_ptr<tee::Instance> inst;

        Obj(tee::Program& p, const std::string& module) : prog(&p)
        {
            std::string error;
            schema = tee::Schema::of(p.fminus(), module, error);
            INFO(error);
            REQUIRE(schema != nullptr);
            const fminus::Program::Module* m = p.fminus().module(module);
            REQUIRE(m != nullptr);
            inst = std::make_unique<tee::Instance>(*m);
        }

        void* data() { return inst->data(); }

        double get(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            const bool ok = inst->read(*schema, path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
            return v;
        }

        void set(const std::string& path, double v)
        {
            std::string error;
            const bool ok = inst->write(*schema, path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        // A resolved field, for reads in a loop (`auto qn = box.field<float>("qn"); box.inst->get(qn)`)
        template <typename T>
        tee::Field<T> field(const std::string& path) const
        {
            std::string error;
            const tee::Field<T> f = schema->field<T>(path, error);
            INFO(path << ": " << error);
            REQUIRE(f.valid());
            return f;
        }

        // Every field with an fminus::default set to it (FMinus runs no constructor: the host writes them)
        void defaults()
        {
            schema->forEach([this](const std::string& path, const tee::Schema::Node& node, std::size_t) {
                const auto* d = node.attribute("fminus::default");
                if (d != nullptr && !d->arguments.empty() && node.extents.empty() && !node.parameter)
                    set(path, d->arguments[0].number);
            });
        }

        // A method's address, looked up once (`auto* f = fn<float(void*, float)>("lift01")`)
        template <typename Signature>
        Signature* fn(const std::string& method)
        {
            std::string errors;
            const std::string name = schema->module() + "::" + method;
            auto* f = prog->function<Signature>(name, errors);
            INFO(name << ": " << errors);
            REQUIRE(f != nullptr);
            return f;
        }

        // A method of this module, its result's type named and its arguments' deduced (`call<float>("lift01", 3.0f)`)
        template <typename R, typename... A>
        R call(const std::string& method, A... args)
        {
            std::string errors;
            const std::string name = schema->module() + "::" + method;
            auto* f = prog->function<R(void*, A...)>(name, errors);
            INFO(name << ": " << errors);
            REQUIRE(f != nullptr);
            return f(data(), args...);
        }
    };
} // namespace tee::test::lab
