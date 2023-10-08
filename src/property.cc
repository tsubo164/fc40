#include <sstream>
#include <iomanip>
#include "property.h"

namespace nes {

template<typename T>
std::string to_hex_string(T *integer)
{
    const int HEX_DIGITS = 2 * sizeof(*integer);
    uint64_t num = *integer;
    std::string hex = "";

    for (int i = 0; i < HEX_DIGITS; i++) {
        static const char digits[] = "0123456789ABCDEF";
        hex = digits[num & 0xF] + hex;
        num >>= 4;
    }

    return hex;
}

template<typename T>
void from_hex_string(const std::string &str, T *integer)
{
    uint64_t num = 0;
    std::stringstream ss(str);
    ss >> std::hex >> num;
    *integer = static_cast<T>(num);
}

std::string Property::ToString(int index) const
{
    switch (type_) {
    case DataType::UInt8:
        return to_hex_string(p_uint8 + index);

    case DataType::UInt16:
        return to_hex_string(p_uint16);

    case DataType::UInt32:
        return to_hex_string(p_uint32);

    case DataType::UInt64:
        return to_hex_string(p_uint64);

    case DataType::Int8:
        return to_hex_string(p_int8);

    case DataType::Int16:
        return to_hex_string(p_int16);

    case DataType::Int32:
        return to_hex_string(p_int32);

    case DataType::Int64:
        return to_hex_string(p_int64);

    case DataType::Bool:
        return to_hex_string(p_bool);

    case DataType::String:
        return *p_string;

    default:
        break;
    }

    return "";
}

bool Property::FromString(const std::string &str, int index)
{
    switch (type_) {
    case DataType::UInt8:
        from_hex_string(str, p_uint8 + index);
        break;

    case DataType::UInt16:
        from_hex_string(str, p_uint16);
        break;

    case DataType::UInt32:
        from_hex_string(str, p_uint32);
        break;

    case DataType::UInt64:
        from_hex_string(str, p_uint64);
        break;

    case DataType::Int8:
        from_hex_string(str, p_int8);
        break;

    case DataType::Int16:
        from_hex_string(str, p_int16);
        break;

    case DataType::Int32:
        from_hex_string(str, p_int32);
        break;

    case DataType::Int64:
        from_hex_string(str, p_int64);
        break;

    case DataType::Bool:
        from_hex_string(str, p_bool);
        break;

    case DataType::String:
        *p_string = str;
        break;

    default:
        return false;
    }

    return true;
}

int Property::GetDataCount() const
{
    return count_;
}

} // namespace
