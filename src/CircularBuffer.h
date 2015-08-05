
template <typename T, unsigned int LEN>
class CircularBuffer
{
  private:
    
    const unsigned int _capacity = LEN;

    //Index of the oldest element
    unsigned int _out = 0;
    //The next empty index we can write to
    unsigned int _nextIn = 0;
    T _buffer[LEN];

  public:
  
    bool write(T value)
    {
      _buffer[_nextIn] = value;
      _nextIn = (_nextIn+1) % LEN;

      //If we've overwritten the first byte then move up the out value to keep overwriting the oldest stuff
      if (_nextIn == _out)
        _out = (_out+1) % LEN;
    }

    bool read(T& value)
    {
      if (_nextIn == _out)
        return false;

      value = _buffer[_out];
      _out = (_out+1) % LEN;
      return true;
    }

    T read()
    {
      if (_nextIn == _out)
        return 0;
      
      T value = _buffer[_out];
      _out = (_out+1) % LEN;

      return value;
    }

    unsigned int size()
    {
      unsigned int len = (_nextIn - _out) & 0x7FFF;
    }
};

