#ifndef MOBILE_CONFIGURATION_H
#define MOBILE_CONFIGURATION_H

#include <map>

class MobileConfiguration {
public:
    MobileConfiguration();

    template <typename T>
    T getConfig ( const std::string& key );

private:
    std::map<std::string, std::string> configMap;
};

#include "MobileConfiguration.tcc"
#endif
