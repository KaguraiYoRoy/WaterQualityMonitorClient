#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "Log.h"

Log Logger;

int main()
{
    Logger.OpenFile("./TempLog");
    Logger.WriteLog(INFO, "LogTest");
    Logger.Close();
    return 0;
} 