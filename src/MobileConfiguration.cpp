#include "MobileConfiguration.h"

MobileConfiguration::MobileConfiguration()
{
    configMap.emplace ( "home_id", "0" );
    configMap.emplace ( "couch_id", "1" );
    configMap.emplace ( "sleep_time_s", "10" );
    configMap.emplace ( "img_width", "640" );
    configMap.emplace ( "img_height", "480" );
}
