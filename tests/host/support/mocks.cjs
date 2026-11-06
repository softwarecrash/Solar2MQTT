'use strict';
// Shared Arduino/OS/transport interfaces only; production behavior is generated per group.
module.exports = ({write,read}) => {
 write('BatteryStorageMock.h',read('tests/host/BatteryStorageMock.h'));
 for(const h of ['LittleFS.h','Preferences.h','esp_partition.h','esp_ota_ops.h'])write(h,'#include "BatteryStorageMock.h"\n');

 // Older native MinGW lacks C++17 inline variables; only table linkage changes.
 write('solar/SolarTypes.h',read('src/solar/SolarTypes.h').replace('inline const char *const protocolStrings[]','const char *const protocolStrings[]'));
 write('Arduino.h',String.raw`#pragma once
#include <string>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cstdint>
#include <math.h>
#include <functional>
#include <algorithm>
#include <cstdio>
#include <atomic>
using byte=uint8_t;
extern uint32_t testNow;
#ifdef SOLAR_TEST_ESP32_ABI
static_assert(sizeof(unsigned long)==4, "Timed host tests require ESP32 32-bit unsigned long; use HOST_CXXFLAGS_JSON=[\"-m32\"] on Linux LP64");
#endif
inline unsigned long millis(){return testNow;}
inline void delay(unsigned long ms){testNow+=ms;}
constexpr int OUTPUT=1;
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
inline bool isDigit(unsigned char x){return std::isdigit(x)!=0;}
class String {
public:
 std::string text;
 String()=default; String(const char *s):text(s?s:""){} String(const std::string &s):text(s){}
 String(char c):text(1,c){} String(unsigned long n):text(std::to_string(n)){} String(int n):text(std::to_string(n)){}
 const char *c_str() const{return text.c_str();} size_t length() const{return text.size();} bool isEmpty() const{return text.empty();}
 bool operator==(const char *s) const{return text==s;} bool operator!=(const char *s) const{return text!=s;}
 bool operator==(const String &s) const{return text==s.text;} bool operator!=(const String &s) const{return text!=s.text;}
 String operator+(const String &s) const{return text+s.text;} String &operator+=(const String &s){text+=s.text;return *this;}
 String &operator+=(const char *s){text+=s;return *this;} String &operator+=(char c){text+=c;return *this;}
 char charAt(size_t i) const{return i<text.size()?text[i]:'\0';} char operator[](size_t i) const{return charAt(i);}
 bool startsWith(const char *s) const{return text.rfind(s,0)==0;}
 bool endsWith(const char *s) const {const size_t n=std::strlen(s);return text.size()>=n&&text.compare(text.size()-n,n,s)==0;}
 bool equalsIgnoreCase(const char *s) const{if(text.size()!=std::strlen(s))return false;for(size_t i=0;i<text.size();++i)if(std::tolower((unsigned char)text[i])!=std::tolower((unsigned char)s[i]))return false;return true;}
 String substring(size_t first) const{return first<text.size()?text.substr(first):"";}
 String substring(size_t first,size_t end) const{return first<text.size()?text.substr(first,end-first):"";}
 void trim(){const size_t first=text.find_first_not_of(" \r\n\t");if(first==std::string::npos){text.clear();return;}text=text.substr(first,text.find_last_not_of(" \r\n\t")-first+1);}
 bool reserve(size_t n){text.reserve(n);return true;} void remove(size_t first,size_t n=std::string::npos){text.erase(first,n);}
 long toInt() const{return std::strtol(text.c_str(),nullptr,10);} float toFloat() const{return std::strtof(text.c_str(),nullptr);}
 int indexOf(char c) const{const auto i=text.find(c);return i==std::string::npos?-1:static_cast<int>(i);}
 void toCharArray(char *out,size_t n) const{if(n)out[text.copy(out,n-1)]='\0';}
 size_t write(uint8_t c){text.push_back(static_cast<char>(c));return 1;}
 size_t write(const uint8_t *p,size_t n){text.append(reinterpret_cast<const char *>(p),n);return n;}
 bool concat(char c){return write(static_cast<uint8_t>(c))==1;}
 bool concat(const uint8_t *p,size_t n){return write(p,n)==n;}
};
inline String operator+(const char *a,const String &b){return String(a)+b;}
class Print {public:virtual ~Print()=default;virtual size_t write(uint8_t)=0;virtual size_t write(const uint8_t *p,size_t n){for(size_t i=0;i<n;i++)write(p[i]);return n;} void print(const char *s){write(reinterpret_cast<const uint8_t *>(s),std::strlen(s));}};
struct FakeEsp{size_t getFreeHeap()const{return 200000;} uint64_t getEfuseMac()const{return 0x90c23f55b594ULL;}};
extern FakeEsp ESP;
#include <ArduinoJson.h>
namespace ArduinoJson {template<>struct Converter<String>{static void toJson(const String &s,JsonVariant v){v.set(s.c_str());} static String fromJson(JsonVariantConst v){return v.as<const char *>();} static bool checkJson(JsonVariantConst v){return v.is<const char *>();}};}
`);
 write('HardwareSerial.h','#pragma once\n#include <Arduino.h>\nclass HardwareSerial {};\n');
 write('modbus/modbus.h',String.raw`#pragma once
#include <Arduino.h>
#include "solar/SolarTypes.h"
class MODBUS {public:bool dynamic=true;unsigned restarted=0;void restartStateRead(){++restarted;}bool hasCompleteDynamicStateUpdate()const{return dynamic;}bool connection=false;BatterySampleTiming timing;String lastCommand;uint32_t interval=0;unsigned accepted=0;void loop(){}String requestData(String command){lastCommand=command;return "ACK-modbus";}bool consumePowMrLivePassCompleted(){return false;}void setPollIntervalMs(uint32_t ms){interval=ms;}void acceptBatterySample(){++accepted;}BatterySampleTiming batterySampleTiming()const{auto sample=timing;sample.available=sample.available&&connection;return sample;}};
`);
 write('freertos/semphr.h',String.raw`#pragma once
#include <mutex>
using SemaphoreHandle_t=std::mutex *;
constexpr unsigned long portMAX_DELAY=0xffffffffUL;
inline bool &failNextSemaphoreCreate(){static bool fail=false;return fail;}
inline SemaphoreHandle_t xSemaphoreCreateMutex(){if(failNextSemaphoreCreate()){failNextSemaphoreCreate()=false;return nullptr;}return new std::mutex;}
inline void xSemaphoreTake(SemaphoreHandle_t p,unsigned long){p->lock();}
inline void xSemaphoreGive(SemaphoreHandle_t p){p->unlock();}
inline void vSemaphoreDelete(SemaphoreHandle_t p){delete p;}
`);
 write('esp_heap_caps.h','#pragma once\n#include <cstddef>\nconstexpr int MALLOC_CAP_8BIT=0;\ninline size_t heap_caps_get_largest_free_block(int){return 180000;}\n');
 write('WiFiClient.h','#pragma once\nclass Client {};\nclass WiFiClient:public Client {};\n');
 write('WiFiClientSecure.h','#pragma once\n#include <WiFiClient.h>\nclass WiFiClientSecure:public Client {};\n');
 write('PubSubClient.h',String.raw`#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <vector>
struct Publication {std::string topic,value;bool retained;};
class PubSubClient {public:
 std::vector<Publication> messages;std::string pendingTopic,pendingValue;
 bool online=true; bool connected()const{return online;} bool loop(){return true;}
 bool publish(const char *topic,const char *value,bool retained=false){messages.push_back({topic,value,retained});return true;}
 bool beginPublish(const char *topic,size_t,bool){pendingTopic=topic;pendingValue.clear();return true;}
 bool endPublish(){messages.push_back({pendingTopic,pendingValue,false});return true;}
 size_t write(uint8_t c){pendingValue.push_back(static_cast<char>(c));return 1;}
 size_t write(const uint8_t *p,size_t n){pendingValue.append(reinterpret_cast<const char *>(p),n);return n;}
};
`);

};
