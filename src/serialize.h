#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "property.h"
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <map>

namespace nes {

class Archive {
public:
    Archive();
    ~Archive();

    bool Serialize(const std::string &name, const Property &prop);
    void Write( std::ostream &os) const;
    void Read(std::istream &is);

    void EnterNamespcae(const std::string &space);
    void LeaveNamespcae();

private:
    std::unordered_map<std::string,Property> map_;
    std::vector<std::string> vec_;
    std::vector<std::string> namespaces_;
};

// For basic types
template<typename T>
void Serialize(Archive &ar, const std::string &name, T *data)
{
    ar.Serialize(name, Property(data));
}

template<size_t COUNT>
void Serialize(Archive &ar, const std::string &name, std::array<uint8_t, COUNT> *data)
{
    ar.Serialize(name, Property(data->data(), COUNT));
}

} // namespace

#endif // _H
