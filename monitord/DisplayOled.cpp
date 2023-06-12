#include "DisplayOled.h"

void* DisplayOled::DisplayThread(void* vParam)
{
	DisplayOled* pClass = (DisplayOled*)vParam;
	char buf[32];
	while (!pClass->isExit){
		sprintf(buf, "Core Temp: %.2f C   ", pClass->Data.LM35);
		oled_putstrto(&(pClass->disp), 0, 0, buf);
		sprintf(buf, "TDS: %d   ", pClass->Data.TDS);
		oled_putstrto(&(pClass->disp), 0, 13, buf);
		sprintf(buf, "Water Temp: %.2f C   ", pClass->Data.WaterTemp);
		oled_putstrto(&(pClass->disp), 0, 26, buf);
		sprintf(buf, "PH: %.2f   ", pClass->Data.PH);
		oled_putstrto(&(pClass->disp), 0, 39, buf);
		sprintf(buf, "Turbidity: %.2f   ", pClass->Data.Turbidity);
		oled_putstrto(&(pClass->disp), 0, 52, buf);
		oled_send_buffer(&(pClass->disp));
		sleep(10);
	}

	return 0;
}

bool DisplayOled::Init(std::string I2C)
{
	memset(&disp, 0, sizeof(disp));
	disp.address = OLED_I2C_ADDR;
	disp.font = font2;

	int ret = oled_open(&disp, const_cast<char*>(I2C.c_str()));
	if (ret < 0) {
		return false;
	}
	ret = oled_init(&disp);
	if (ret < 0) {
		return false;
	}

	isExit = false;
	pthread_create(&tDisplayThread, NULL, DisplayThread, this);
	return true;
}

void DisplayOled::UpdateData(_SENSORS_DATA mData)
{
	Data = mData;
}

void DisplayOled::Exit()
{
	isExit = true;
	pthread_join(tDisplayThread, nullptr);
}
