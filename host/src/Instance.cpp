// host/src/Instance.cpp: tee::Instance (Instance.h).
#include "tee/Instance.h"

#include <algorithm>
#include <cstdint>
#include <new>

namespace tee
{
    Instance::Instance(const fminus::Program::Module& module) : module_(&module)
    {
        // At least the strictest fundamental alignment, and a size that is a multiple of it, as FMinus's header asks.
        alignment_ = std::max(module.alignment, alignof(std::max_align_t));
        size_ = (std::max<std::size_t>(module.size, 1) + alignment_ - 1) / alignment_ * alignment_;
        bytes_ = static_cast<unsigned char*>(::operator new(size_, std::align_val_t{alignment_}));
        reset();
    }

    Instance::~Instance()
    {
        ::operator delete(bytes_, std::align_val_t{alignment_});
    }

    void Instance::reset() noexcept
    {
        std::memset(bytes_, 0, size_);
        fminus::Program::setDefaults(bytes_, *module_);
    }

    double Instance::read(const Schema::Handle& handle, std::size_t index) const noexcept
    {
        if (handle.node == nullptr || index >= handle.count)
            return 0.0;
        const unsigned char* at = bytes_ + handle.offset + index * handle.node->stride;
        switch (handle.node->scalar)
        {
            case Scalar::Float:
            {
                float v;
                std::memcpy(&v, at, sizeof v);
                return static_cast<double>(v);
            }
            case Scalar::Double:
            {
                double v;
                std::memcpy(&v, at, sizeof v);
                return v;
            }
            case Scalar::Int:
            {
                std::int32_t v;
                std::memcpy(&v, at, sizeof v);
                return static_cast<double>(v);
            }
            case Scalar::Bool:
                return *at != 0 ? 1.0 : 0.0;
            case Scalar::Char:
                return static_cast<double>(*at);
            case Scalar::Pointer:
            case Scalar::None:
                break;
        }
        return 0.0;
    }

    void Instance::write(const Schema::Handle& handle, double value, std::size_t index) noexcept
    {
        if (handle.node == nullptr || index >= handle.count)
            return;
        if (handle.node->parameter)
        {
            if (handle.node->param != nullptr)
            {
                // The slot's value is at node->offset within its module; the handle's offset is that value's place
                // in this instance, so the module that declares it starts this far before it.
                void* owner = bytes_ + (handle.offset - handle.node->param->valueOffset);
                fminus::Program::setValue(owner, *handle.node->param, value);
            }
            return;
        }
        unsigned char* at = bytes_ + handle.offset + index * handle.node->stride;
        switch (handle.node->scalar)
        {
            case Scalar::Float:
            {
                const float v = static_cast<float>(value);
                std::memcpy(at, &v, sizeof v);
                break;
            }
            case Scalar::Double:
                std::memcpy(at, &value, sizeof value);
                break;
            case Scalar::Int:
            {
                const double clamped = std::clamp(value, -2147483648.0, 2147483647.0);
                const std::int32_t v = static_cast<std::int32_t>(clamped);
                std::memcpy(at, &v, sizeof v);
                break;
            }
            case Scalar::Bool:
                *at = value != 0.0 ? 1 : 0;
                break;
            case Scalar::Char:
                *at = static_cast<unsigned char>(std::clamp(value, 0.0, 255.0));
                break;
            case Scalar::Pointer:
            case Scalar::None:
                break;
        }
    }

    bool Instance::read(const Schema& schema, std::string_view path, double& value, std::string& error) const
    {
        Schema::Handle handle;
        if (!schema.find(path, handle, error))
            return false;
        if (handle.scalar() == Scalar::None || handle.scalar() == Scalar::Pointer)
        {
            error = "'" + std::string(path) + "' is '" + handle.node->type + "', which is not read as a number";
            return false;
        }
        value = read(handle);
        return true;
    }

    bool Instance::write(const Schema& schema, std::string_view path, double value, std::string& error)
    {
        Schema::Handle handle;
        if (!schema.find(path, handle, error))
            return false;
        if (handle.scalar() == Scalar::None || handle.scalar() == Scalar::Pointer)
        {
            error = "'" + std::string(path) + "' is '" + handle.node->type + "', which is not written as a number";
            return false;
        }
        write(handle, value);
        return true;
    }
} // namespace tee
