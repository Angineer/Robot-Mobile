#include "MobileConfiguration.h"

MobileConfiguration::MobileConfiguration()
{
    configMap.emplace ( "home_id", "0" );
    configMap.emplace ( "couch_id", "1" );
    configMap.emplace ( "sleep_time_s", "5" );
}
