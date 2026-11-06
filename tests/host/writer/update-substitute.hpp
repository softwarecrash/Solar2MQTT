// Models the ownership-relevant native Updater.cpp contract, not flash/image validation.
// begin while active: false with previous error/session untouched (Updater.cpp:187).
// buffer allocation failure: false, no running session, error remains zero (:291).
// write without running: zero without setting error (:832); oversize aborts.
// end without running: false without setting error (:766); end(true) finalizes prefix.
struct TestUpdate {
 bool active=false,error=false,allocationFail=false,endFail=false,writeFail=false;
 uint32_t limit=0;unsigned begins=0,writes=0,ends=0,aborts=0;
 std::string bytes,committed;std::function<void()> afterBegin;
 bool begin(uint32_t size){
  ++ota.updateBegins;++begins;if(active)return false;
  error=false;bytes.clear();if(allocationFail)return false;
  active=true;limit=size;
  auto hook=std::move(afterBegin);afterBegin={};if(hook)hook();return true;
 }
 size_t write(const uint8_t *p,size_t n){
  ++writes;if(error||!active)return 0;
  if(writeFail||n>limit-bytes.size()){abort();return 0;}
  bytes.append(reinterpret_cast<const char *>(p),n);return n;
 }
 bool end(bool evenIfRemaining){
  ++ends;if(error||!active)return false;
  if(endFail||(!evenIfRemaining&&bytes.size()!=limit)){abort();return false;}
  committed=bytes;active=false;return true;
 }
 void abort(){++aborts;active=false;error=true;}
 bool hasError()const{return error;}bool isRunning()const{return active;}
 template<class T>void printError(T &){}
};
