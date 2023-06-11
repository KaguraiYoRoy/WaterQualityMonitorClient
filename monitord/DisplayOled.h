#pragma once

#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <cstring>
#include <iostream>

extern "C" {
	#include "oled.h"
	#include "font.h"
}

struct _SENSORS_DATA {
	double WaterTemp, LM35, PH, Turbidity;
	int TDS;
};

class DisplayOled
{
private:
	display_info disp;
	pthread_t tDisplayThread;
	_SENSORS_DATA Data;
	bool isExit;
	static void* DisplayThread(void* vParam);
public:
	bool Init(std::string I2C);
	void UpdateData(_SENSORS_DATA mData);
	void Exit();
};

