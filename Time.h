#pragma once
#ifndef TIME
#define TIME
#ifndef WINAPI
#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY
#endif
#include "Windows.h"
#endif // !WINAPI

#define TIME_MIN(a,b) (a < b) ? a : b
#define TIME_MAX(a,b) (a > b) ? a : b

///////////////////////////////////////////////////////////////////////
//ALL TIME IS IN SECONDS
///////////////////////////////////////////////////////////////////////

//Used for the number of past samples to keep for smooth delta time
#define TIME_NUMSAMPLES 10

//This class can be turned into a singleton if no other instance is needed
//
//This is how to do that
//
//Move the constructor and destructor into the private section
//Add this function in public
/*
static Time GetTime()
{
	static Time instance;
	return instance;
}
*/

class Time
{
private:
	friend class Timer;
	//For use in program
	float _deltaTime;
	double _doubleDeltaTime;
	float _smoothDeltaTime;
	double _doubleSmoothDeltaTime;
	float _elapsedTime;
	float _fixedDeltaTime;
	float _timeScale;
	float _fixedTimeScale;
	float _fixedTimeCounter;

	//For use behind scenes
	long long _prevTicksCount[TIME_NUMSAMPLES];
	long long _firstTickCount;
	long long _frequency;
	unsigned char _signalCount;
	unsigned char _elapsedSignals;

public:
	Time() 
	{
		ZeroMemory(_prevTicksCount, sizeof(_prevTicksCount));
		_signalCount = _elapsedSignals = 0;
		_doubleDeltaTime = _doubleSmoothDeltaTime = 0.0;
		_firstTickCount = _frequency = 0;
		_deltaTime = _smoothDeltaTime = _elapsedTime = 0.0f;
		_fixedDeltaTime = 0.016667f;
		_timeScale = _fixedTimeScale = 1.0f;
		Restart();
	};
	~Time()
	{
		ZeroMemory(_prevTicksCount, sizeof(_prevTicksCount));
		_signalCount = _elapsedSignals = 0;
		_doubleDeltaTime = _doubleSmoothDeltaTime = 0.0;
		_firstTickCount = _frequency = 0;
		_deltaTime = _smoothDeltaTime = _elapsedTime = 0.0f;
		_fixedDeltaTime = 0.0f;
		_timeScale = _fixedTimeScale = 0.0f;
	};
	//Time elapsed since last call to Signal() or Restart()
	float deltaTime() { return _deltaTime; }
	//Time elapsed since last call to Signal() or Restart() in double
	double doubleDeltaTime() { return _doubleDeltaTime; }
	//Average of the past 10 delta's. i.e smoother delta time
	float smoothDeltaTime() { return _smoothDeltaTime; }
	//Average of the past 10 delta's. i.e smoother delta time
	double doubleSmoothDeltaTime() { return _doubleSmoothDeltaTime; }
	//Complete elapsed time since last call to Restart()
	float elapsedTime() { return _elapsedTime; }
	//Returns the time interval for fixed updates such as physics
	float fixedDeltaTime() { return _fixedDeltaTime; }
	//Returns the time multiplier for delta time
	float timeScale() { return _timeScale; }
	//Returns the time multiplier for fixed time
	float fixedTimeScale() { return _fixedTimeScale; }
	//Change the time scale more is faster less is slower
	void timeScale(float TimeScale) { _timeScale = TimeScale; }
	//Change the fixed time scale more is faster less is slower
	void fixedTimeScale(float FixedTimeScale) { _fixedTimeScale = FixedTimeScale; }
	//Change the rate at which fixed time functions are called lower is more called
	void fixedDeltaTime(float FixedDeltaTime) { _fixedDeltaTime = FixedDeltaTime; }
	//Called to see if its time to do fixed stuff
	bool fixedTimeCall()
	{
		if (_fixedTimeCounter >= _fixedDeltaTime)
		{
			_fixedTimeCounter = 0.0f;
			return true;
		}
		return false;
	}
	//Used at the beginning of each loop cycle
	void Signal()
	{
		memmove_s(_prevTicksCount + 1u, sizeof(long long) * TIME_NUMSAMPLES, _prevTicksCount, sizeof(long long) * TIME_NUMSAMPLES);

		QueryPerformanceCounter((LARGE_INTEGER*)_prevTicksCount);
		_signalCount = TIME_MIN(_signalCount + 1, TIME_NUMSAMPLES - 1);

		_elapsedTime = float(_prevTicksCount[0] - _firstTickCount) / float(_frequency);
		_doubleDeltaTime = double(_prevTicksCount[0] - _prevTicksCount[1]) / double(_frequency);
		_fixedTimeCounter += (float)_doubleDeltaTime * _fixedTimeScale;
		_doubleDeltaTime *= _timeScale;
		_deltaTime = float(_doubleDeltaTime);

		double totalWeight = 0, runningWeight = 1;
		LONGLONG totalValue = 0, sampleDelta;

		unsigned char size = TIME_MIN(TIME_NUMSAMPLES, _signalCount - 1);
		// loop up to num samples or as many as we have available
		for (unsigned char i = 0; i < size; ++i)
		{
			// determine each delta as we go
			sampleDelta = _prevTicksCount[i] - _prevTicksCount[i + 1];
			totalValue += LONGLONG(sampleDelta * runningWeight + 0.5); // this cast is expensive, need to look into optimizing
			totalWeight += runningWeight; // tally all the weights used
			runningWeight *= 0.75; // adjust the weight of next delta
		}

		_doubleSmoothDeltaTime = (totalValue / totalWeight) / double(_frequency);
		_doubleSmoothDeltaTime *= _timeScale;
		_smoothDeltaTime = float(_doubleSmoothDeltaTime);
	};
	//Used to clean up an start new
	void Restart()
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&_frequency);

		_signalCount = _elapsedSignals = 0;
		_doubleDeltaTime = _doubleSmoothDeltaTime = _fixedTimeCounter = 0.0;
		_deltaTime = _smoothDeltaTime = _elapsedTime = 0.0f;
		_timeScale = _fixedTimeScale = 1.0f;

		QueryPerformanceCounter((LARGE_INTEGER*)&_firstTickCount);
		_prevTicksCount[_signalCount++] = _firstTickCount;
	};
};

class Timer
{
private:
	Time* _friendTime;

	float _deltaTime;
	float _elapsedTime;

	float _executeInterval;

	//For use behind scenes
	long long _prevTickCount;
	long long _thisTickCount;
	long long _frequency;
public:
	//This Constructor is basicly a smaller version on the Time class
	Timer(float executionInterval)
	{
		_frequency = 0;
		_deltaTime = _elapsedTime = 0.0f;
		_executeInterval = executionInterval;
		_friendTime = nullptr;
		QueryPerformanceFrequency((LARGE_INTEGER*)&_frequency);
	}
	//Makes the Time class a parent so that we can get the delta information from it fast and easy
	Timer(float executionInterval, Time* parentTime)
	{
		_frequency = 0;
		_deltaTime = _elapsedTime = 0.0f;
		_executeInterval = executionInterval;
		_friendTime = parentTime;
		_elapsedTime = _friendTime->_elapsedTime;
		_deltaTime = _friendTime->_deltaTime;
	}
	~Timer()
	{
		_friendTime = nullptr;
		_frequency = 0;
		_deltaTime = _elapsedTime = 0.0f;
		_prevTickCount = 0;
		_thisTickCount = 0;
	}
	//IF YOU USED THE OVERLOADED CONSTRUCTOR WITH TIME THEN THIS IS ALL YOU NEED TO CALL
	//Used to check if the specified executeInterval has elapsed
	bool canExecute()
	{
		if (_friendTime)
		{
			if (_friendTime->_elapsedTime - _elapsedTime >= _executeInterval)
			{
				_elapsedTime = _friendTime->_elapsedTime;
				return true;
			}
		}
		else if(_elapsedTime >= _executeInterval)
		{
			_elapsedTime = 0;
			return true;
		}
		return false;
	}
	//IF YOU USED THE OVERLOADED CONSTRUCTOR WITH TIME THEN DONT USE THIS
	//Used at the beginning of each loop cycle
	void Signal()
	{
		if (_friendTime) return;
		QueryPerformanceCounter((LARGE_INTEGER*)&_thisTickCount);

		_deltaTime = float(_prevTickCount - _thisTickCount) / float(_frequency);
		_elapsedTime += _deltaTime;

		_prevTickCount = _thisTickCount;
	};
	//IF YOU USED THE OVERLOADED CONSTRUCTOR WITH TIME THEN DONT USE THIS
	//Used at the beginning of each loop cycle -- More efficient
	void Signal(float deltaTime)
	{
		if (_friendTime) return;
		_elapsedTime += deltaTime;
		_deltaTime = deltaTime;

		QueryPerformanceCounter((LARGE_INTEGER*)&_thisTickCount);
		_prevTickCount = _thisTickCount;
	};
};
#endif

/*
Example on how to use.

int main ()
{
	App app;
	Time timer;
	timer.Restart() //Not needed but I like to make sure that its ready for useTime timer;
	while(true) // Game Loop
	{
		timer.Signal(); // Just call Signal at the beginning of the game loop
		app.update();//If its not a singleton then make sure you either pass in deltatime or have app(whatever your game stuff is) have a time member
		if(timer.fixedTimeCall())
		{
			app.fixedUpdate();
		}
		app.render();
	}

}
*/