'use strict';
const fs=require('node:fs'),path=require('node:path');
module.exports=h=>{
 require('./support/mocks.cjs')(h);
 const {extract}=require('./support/extract.cjs');
 const source=h.read('src/core/WebServerHandler.cpp'),state=h.read('src/core/SolarState.cpp');
 const ota=h.read('src/core/GitHubOtaUpdater.cpp');
 h.write('production-ota-guard.inc',extract(ota,'GitHubOtaUpdater::GitHubOtaUpdater')+'\n'+extract(ota,'GitHubOtaUpdater::StatusGuard::StatusGuard')+'\n'+extract(ota,'GitHubOtaUpdater::StatusGuard::~StatusGuard')+'\n'+extract(ota,'void GitHubOtaUpdater::unlock'));
 if(!h.read('src/core/WebServerHandler.h').includes('bool buildStatusJson(JsonDocument &doc)'))throw Error('Missing status builder overflow result');
 const serializer=source.indexOf('bool serializeCompleteJson');
 if(serializer<0||!source.includes('#include "core/JsonOutput.h"'))throw Error('Missing complete JSON append writer');
 const checked=source.indexOf('class CheckedJsonResponse');
 if(checked<0)throw Error('Missing checked JSON response');
 const responseHelper=source.slice(checked,source.indexOf('void sendCompleteJson',checked))+extract(source,'void sendCompleteJson');
 const helper='#include "core/JsonOutput.h"\n'+extract(source,'bool serializeCompleteJson')+'\n'+responseHelper;
 const start=source.indexOf('    _server.on("/api/status", HTTP_GET,'),end=source.indexOf('    _server.on("/api/settings", HTTP_GET,',start);
 if(start<0||end<0)throw Error('Missing production data route');
 const snapshotSerializer=state.indexOf('template <typename Writer>\nclass SnapshotJsonSerializer'),snapshotSerializerEnd=state.indexOf('JsonObject ensureObject',snapshotSerializer);
 if(snapshotSerializer<0||snapshotSerializerEnd<0)throw Error('Missing production snapshot serializer');
 h.write('production-overflow.inc',extract(source,'void copyObjectSection')+'\n'+helper+'\n'+state.slice(snapshotSerializer,snapshotSerializerEnd)+'\n'+extract(state,'bool SolarState::snapshotTo')+'\n'+extract(state,'bool SolarState::serializeSnapshot')+'\n'+extract(source,'bool WebServerHandler::buildStatusJson')+'\n'+extract(source,'void WebServerHandler::refreshStatusPayload')+'\n'+extract(source,'bool WebServerHandler::copyLastStatusPayload')+'\n'+extract(source,'void WebServerHandler::setLastStatusPayload')+'\n'+extract(source,'void WebServerHandler::setupStatusWebSocket')+'\nvoid WebServerHandler::registerDataRoute(){\n'+source.slice(start,end)+'\n}');
 // Installed ESPAsyncWebServer builds a vector before inspecting subscribers.
 // This transport boundary preserves that allocation contract without making
 // the host group depend on an installed ESPAsyncWebServer source checkout.
 h.write('production-websocket.inc',
  'namespace { AsyncWebSocketSharedBuffer makeSharedBuffer(const uint8_t *message,size_t len){auto buffer=std::make_shared<std::vector<uint8_t>>(len);std::memcpy(buffer->data(),message,len);return buffer;} }\n'+
  'AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const uint8_t *message,size_t len){return textAll(makeSharedBuffer(message,len));}\n'+
  'AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const char *message,size_t len){return textAll(reinterpret_cast<const uint8_t *>(message),len);}\n'+
  'AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const String &message){return textAll(message.c_str(),message.length());}\n');
 const file=path.join(h.build,'Arduino.h');let arduino=fs.readFileSync(file,'utf8');
 arduino=arduino.replace('class String {','struct StringFaultState { bool reserveFailure=false;size_t writeBudget=SIZE_MAX,copyFailureSize=SIZE_MAX; };\nextern StringFaultState stringFault;\nclass String {');
 arduino=arduino.replace(' std::string text;', ' std::string text;\n String(const String &)=default;\n String &operator=(const String &other){if(other.text.size()==stringFault.copyFailureSize)text.clear();else text=other.text;return *this;}');
 arduino=arduino.replace(' bool reserve(size_t n){text.reserve(n);return true;}', ' bool reserve(size_t n){observeStringReserve(this);if(stringFault.reserveFailure)return false;text.reserve(n);return true;}');
 arduino=arduino.replace('size_t write(uint8_t c){text.push_back(static_cast<char>(c));return 1;}', 'size_t write(uint8_t c){if(!stringFault.writeBudget)return 0;if(stringFault.writeBudget!=SIZE_MAX)--stringFault.writeBudget;text.push_back(static_cast<char>(c));return 1;}');
 arduino=arduino.replace('size_t write(const uint8_t *p,size_t n){text.append(reinterpret_cast<const char *>(p),n);return n;}', 'size_t write(const uint8_t *p,size_t n){const size_t accepted=std::min(n,stringFault.writeBudget);if(stringFault.writeBudget!=SIZE_MAX)stringFault.writeBudget-=accepted;text.append(reinterpret_cast<const char *>(p),accepted);return accepted;}');
 if(!arduino.includes('observeStringReserve(this)')||!arduino.includes('const size_t accepted=std::min(n,stringFault.writeBudget)')||!arduino.includes('bool concat(const uint8_t'))throw Error('String fault adapter did not match');
 arduino='void observeStringReserve(const void *);\n'+arduino;
 fs.writeFileSync(file,arduino);
 const semaphore=path.join(h.build,'freertos/semphr.h');let sem=fs.readFileSync(semaphore,'utf8');sem=sem.replace('inline void xSemaphoreTake(SemaphoreHandle_t p,unsigned long){p->lock();}','constexpr int pdTRUE=1;\ninline int xSemaphoreTake(SemaphoreHandle_t p,unsigned long timeout){if(timeout==0)return p->try_lock()?pdTRUE:0;p->lock();return pdTRUE;}');fs.writeFileSync(semaphore,sem);
};
