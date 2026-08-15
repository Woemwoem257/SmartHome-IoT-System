#pragma once
#include <string>

class AwsMqtt{
    public:
        static void init();
        static void publish(const std::string& topic, const std::string& payload);
};