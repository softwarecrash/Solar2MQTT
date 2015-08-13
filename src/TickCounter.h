
//Tick count that takes longer to roll over
class TickCounter
{
  private:
  
    unsigned long long _tickCounter = 0;
    unsigned long _lastCpuTickCount = 0;

  public:
  
    //The tick counter will roll over in 53 seconds, this must be called at least every 26 sec
    unsigned long long getTicks()
    {
      unsigned long ticks = ESP.getCycleCount();
      unsigned long delta = (ticks - _lastCpuTickCount) & 0x7FFFFFFF;
      _lastCpuTickCount = ticks;
      _tickCounter += delta;
      return _tickCounter;
    }

    unsigned long long getMillis()
    {
      return getTicks() / (ESP8266_CLOCK / 1000);
    }

    unsigned long getSeconds()
    {
      return getTicks() / ESP8266_CLOCK;
    }
};

class PollDelay
{
  private:
    
    unsigned long long _startMillis;
    TickCounter* _tickCounter;

  public:
    
    PollDelay(TickCounter& tickCounter)
    {
      _tickCounter = &tickCounter;
      _startMillis = _tickCounter->getMillis();
    }
  
    void reset()
    {
      _startMillis = _tickCounter->getMillis();
    }
  
    int compare(unsigned int millisSinceStart)
    {
      return (int)( _tickCounter->getMillis() - (_startMillis + millisSinceStart) );
    }
};

