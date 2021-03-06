/*
 Name:		cremaSensor.h
 Created:	2/13/2018 8:07:56 PM
 Author:	crist
 Editor:	http://www.visualmicro.com
*/

#ifndef _cremaSensor_h
#define _cremaSensor_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// upload to IoT cloud
#include <WiFi.h>
//#include <PubSubClient.h>  // para MQTT
#include <HTTPClient.h>
#include <ArduinoJson.h>

// sensors
#include <BH1750.h>            // Luminosidade
#include <Adafruit_BME280.h>   // umidade, temperatura e press�o
// GPS
#include <HardwareSerial.h>    // GPS: fazer conex�o com o m�dulo e efetuar leitura
#include <TinyGPS++.h>         // GPS: para validar as strings lidas do m�dulo GPS
#include "driver/periph_ctrl.h"
#include "esp32-hal-i2c.h"


// ubidots configs
#define DEVICE_LABEL "esp32_bh"                    // Assig the device label
#define TOKEN "A1E-nuWgdhFqYZUQAIqItVXN67ssBhtJYV" // Put your Ubidots' TOKEN

// cremaPinos.h deve ser inclu�do por �ltimo
#include "cremaErr.h"
#include "cremaPinos.h"


typedef enum cremaSensorsId {
	csLuminosidade,
	csUmidade,
	csTemperatura,
	csPressao,
	csAltitude,
	csUltraVioleta,
	csMemory,
	csLog,
	csCount,
} cremaSensorsId;
// TODO: fazer upload em Ubidots ao iniciar CREMA (necess�rio para Ubidots reenviar alerta, pois �ltimo valor na nuvem deve ser diferente daquele valor que provovou envio do alerta).

const cremaSensorsId _IoT_Update_IniSensor = csLuminosidade;
const cremaSensorsId _IoT_Update_FimSensor = csMemory;

static Adafruit_BME280 _bme(_CREMA_I2C_SDA, _CREMA_I2C_SCL);
static HardwareSerial Serial_GPS(2);

struct cremaGPS
{
	int32_t HDOP;
	byte satellites;
	uint altitude;
	ulong age;
	bool updatedLocation, validLocation;
	bool validAltitude;
	double lat, lng;
};

class cremaSensorClass
{
private:
	int _average_UV_AnalogRead(gpio_num_t pin);
	float _mapfloat_UV_Calc(float x, float in_min, float in_max, float out_min, float out_max);
	float _getUV();
	BH1750 _luxSensor;
	TinyGPSPlus _gps;
	void _saveGPS();
	byte _gpsReadsWithError = 0;
#if defined(_DEBUG)
	void _displayGPSInfo();
#endif // _CREMA_DEBUG

public:
	cremaSensorClass();
	bool init();
	bool readSensors();
	void publishHTTP(const cremaSensorsId first, const cremaSensorsId last, const cremaErrorDescription desc = "", const cremaSystemErrorDescription sysErrorMsg = "");
	byte Decimals[csCount] = { 0,1,1,0,0,3,0 };
	char* Names[csCount] = { "Luminosidade", "Umidade", "Temperatura", "Press�o", "Altitude", "Intensidade Ultra violeta", "memory", "log"};
	char* Labels[csCount] = { "luminosidade", "umidade", "temperatura", "pressao", "altitude", "uv", "memory", "log" };
	float Values[csCount];
	void readGPS();
	cremaGPS gpsData;
};

static void g_SetLastSystemError(const char * functionName, const char * errorMessage)
{
	cremaSystemErrorDescription _lastSystemError;
	sprintf(_lastSystemError, "%s() (%d): %s", functionName, millis() / 1000, errorMessage);
	throw _lastSystemError;
}

#endif

