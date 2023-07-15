#pragma once
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <mutex>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <curl/curl.h>
#include <json/json.h>

#define DEFAULT_UA          "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0.1 QualityMonitor"
#define DEFAULT_INTERVAL    1800
#define DEFAULT_USE_OLED    true
#define DEFAULT_OLED_VCC    2

struct _SENSORS_DATA {
	double WaterTemp, LM35, PH, Turbidity, BatVoltage;
	int TDS;
};
