#include "serialize.h"
#include <sstream>
#include <iomanip>

namespace nes {

Archive::Archive()
{
}

Archive::~Archive()
{
}

bool Archive::Serialize(const std::string &name, const Property &prop)
{
    std::string name_ = name;
    for (auto it = namespaces_.crbegin(); it != namespaces_.crend(); ++it) {
        name_ = *it + "." + name_;
    }

    if (map_.find(name_) != map_.end()) {
        // assert
        return false;
    }

    map_.insert(std::make_pair(name_, prop));
    vec_.emplace_back(name_);

    return true;
}

void Archive::Write(std::ostream &os) const
{
    for (const auto &name : vec_) {
        const auto it = map_.find(name);
        if (it != map_.end()) {
            const Property &prop = it->second;

            if (prop.GetDataCount() == 1) {
                os << std::setw(12) << std::setfill(' ') << std::left <<
                    name << " " <<
                    prop.ToString() << std::endl;
            }
            else {
                os << std::setw(12) << std::setfill(' ') << std::left << name;

                for (int i = 0; i < prop.GetDataCount(); i++) {
                    if (i % 256 == 0)
                        os << std::endl;

                    if (i % 16 == 0)
                        os << "  ";

                    os << prop.ToString(i);

                    if (i % 16 == 15)
                        os << std::endl;
                    else if (i % 16 == 7)
                        os << "  ";
                    else
                        os << ' ';
                }
                os << std::endl;
            }
        }
    }
}

void Archive::Read(std::istream &is)
{
    while (is) {
        std::string name;
        std::string val;
        is >> name;

        const auto &it = map_.find(name);
        if (it != map_.end()) {
            Property &prop = it->second;

            if (prop.GetDataCount() == 1) {
                is >> val;
                prop.FromString(val);
            }
            else {
                for (int i = 0; i < prop.GetDataCount(); i++) {
                    is >> val;
                    prop.FromString(val, i);
                }
            }
        }
    }
}

void Archive::EnterNamespcae(const std::string &space)
{
    namespaces_.push_back(space);
}

void Archive::LeaveNamespcae()
{
    if (namespaces_.empty()) {
        //assert!!
    }
    namespaces_.pop_back();
}

} // namespace
