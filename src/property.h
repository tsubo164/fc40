#ifndef PROPERTY_H
#define PROPERTY_H

#include <cstdint>
#include <string>

namespace nes {

class Property {
public:
    Property(uint8_t *data, int count = 1)
        : type_(DataType::UInt8), p_uint8(data), count_(count) {}
    Property(uint16_t *data) : type_(DataType::UInt16), p_uint16(data) {}
    Property(uint32_t *data) : type_(DataType::UInt32), p_uint32(data) {}
    Property(uint64_t *data) : type_(DataType::UInt64), p_uint64(data) {}

    Property(int8_t *data) : type_(DataType::Int8), p_int8(data) {}
    Property(int16_t *data) : type_(DataType::Int16), p_int16(data) {}
    Property(int32_t *data) : type_(DataType::Int32), p_int32(data) {}
    Property(int64_t *data) : type_(DataType::Int64), p_int64(data) {}

    Property(bool *data) : type_(DataType::Bool), p_bool(data) {}
    Property(std::string *data) : type_(DataType::String), p_string(data) {}
    ~Property() {}

    std::string ToString(int index = 0) const;
    bool FromString(const std::string &str, int index = 0);
    int GetDataCount() const;

private:
    enum class DataType {
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Int8,
        Int16,
        Int32,
        Int64,
        Bool,
        String,
    } type_ = DataType::UInt8;

    union {
        uint8_t *p_uint8 = nullptr;
        uint16_t *p_uint16;
        uint32_t *p_uint32;
        uint64_t *p_uint64;
        int8_t *p_int8;
        int16_t *p_int16;
        int32_t *p_int32;
        int64_t *p_int64;
        bool *p_bool;
        std::string *p_string;
    };

    int count_ = 1;
};

} // namespace

#endif // _H
