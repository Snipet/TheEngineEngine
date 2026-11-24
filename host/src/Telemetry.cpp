// host/src/Telemetry.cpp: the frames' readers (Telemetry.h).
#include "tee/Telemetry.h"

#include <algorithm>
#include <cmath>

namespace tee
{
    std::size_t scalarBytes(Scalar scalar) noexcept
    {
        switch (scalar)
        {
            case Scalar::Float: return 4;
            case Scalar::Double: return 8;
            case Scalar::Int: return 4;
            case Scalar::Bool: return 1;
            case Scalar::Char: return 1;
            case Scalar::Pointer: return sizeof(void*);
            case Scalar::None: break;
        }
        return 0;
    }

    double readScalar(const unsigned char* at, Scalar scalar) noexcept
    {
        switch (scalar)
        {
            case Scalar::Float:
            {
                float v;
                std::memcpy(&v, at, 4);
                return v;
            }
            case Scalar::Double:
            {
                double v;
                std::memcpy(&v, at, 8);
                return v;
            }
            case Scalar::Int:
            {
                std::int32_t v;
                std::memcpy(&v, at, 4);
                return v;
            }
            case Scalar::Bool: return *at != 0 ? 1.0 : 0.0;
            case Scalar::Char: return *at;
            case Scalar::Pointer:
            case Scalar::None: break;
        }
        return 0.0;
    }

    void writeScalar(unsigned char* at, Scalar scalar, double value) noexcept
    {
        switch (scalar)
        {
            case Scalar::Float:
            {
                const float v = static_cast<float>(value);
                std::memcpy(at, &v, 4);
                break;
            }
            case Scalar::Double: std::memcpy(at, &value, 8); break;
            case Scalar::Int:
            {
                const double c = std::isfinite(value) ? std::clamp(value, -2147483648.0, 2147483647.0) : 0.0;
                const std::int32_t v = static_cast<std::int32_t>(c);
                std::memcpy(at, &v, 4);
                break;
            }
            case Scalar::Bool: *at = value != 0.0 ? 1 : 0; break;
            case Scalar::Char: *at = static_cast<unsigned char>(std::isfinite(value) ? std::clamp(value, 0.0, 255.0) : 0.0); break;
            case Scalar::Pointer:
            case Scalar::None: break;
        }
    }

    namespace
    {
        double readIn(const std::vector<unsigned char>& bytes, std::size_t base, std::size_t offset, Scalar scalar,
                      std::size_t index)
        {
            const std::size_t size = scalarBytes(scalar);
            if (size == 0 || offset < base)
                return 0.0;
            const std::size_t at = offset - base + index * size;
            if (at + size > bytes.size())
                return 0.0;
            return readScalar(bytes.data() + at, scalar);
        }
    } // namespace

    double TelemetryFrame::get(const FieldInfo& field, std::size_t index) const noexcept
    {
        return readIn(tel, telOffset, field.offset, field.scalar, index);
    }

    double TelemetryFrame::get(const FieldRef& ref, std::size_t index) const noexcept
    {
        if (ref.generation != generation)
            return 0.0;
        return readIn(tel, telOffset, ref.offset, ref.scalar, index);
    }

    double ScopeFrame::get(const FieldInfo& field, std::size_t index) const noexcept
    {
        return readIn(bytes, scopeOffset, field.offset, field.scalar, index);
    }
} // namespace tee
