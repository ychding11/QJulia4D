#pragma once

class Timer
{

  // for timing
  LARGE_INTEGER tm0, tm1;
  LARGE_INTEGER fq;
  
public:
  Timer(void)
  {
    QueryPerformanceFrequency(&fq);
    Start();
  }

  void Start()
  {
    QueryPerformanceCounter(&tm0);
  }

  // return elapsed time in miliseconds since last start
  float Elapsed()
  {
    QueryPerformanceCounter(&tm1);		// get reference time
    float tm = (float)((tm1.QuadPart-tm0.QuadPart)/(float)fq.QuadPart)*1000;
    return tm;
  }

  float Stop()
  {
    return Elapsed();
  }


};
