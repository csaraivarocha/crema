// 
// 
// 

#include "cremaClass.h"


cremaClass::cremaClass()
{
	visor = new cremaVisorClass();
	config = new cremaConfigClass();
	sensor = new cremaSensorClass();
	time = new cremaTimeClass();

	visor->showMessage(F("Inicializando"));

	_initWiFi();

	config->init();        // config.init tem que ser antes. para ler as configura��es do arquivo
	_wifi_autoConnect();
	treatLastError();
	if (!sensor->init())
	{
		_uploadErrorLog(ceSensorInit, _ERR_UPLOAD_LOG_RESTART, _ERR_UPLOAD_LOG_SAVE_CONFIG);
	}

	visor->clear();

	Serial_GPS.flush();
}

cremaClass::~cremaClass()
{
	delete visor;
	delete config;
	delete sensor;
	delete time;
}

void cremaClass::init()
{
}

// esta fun��o � chamada somente uma v�s, no in�cio do sistema, quando cria a classe cremaClass.
void cremaClass::treatLastError()
{
	if (config->Values[ccLastError].length() > 4) // maior que 4 indica conte�do inv�lido, pois a quantidade maior de caracteres � 4 (-999)
	{
		config->setLastError(ceNoError);
	}
	else if (config->Values[ccLastError].toInt() == _ERR_SENSOR_READ)
	{
		delay(5000);
		// necess�rio fazer upload de novo valor para gerar trigger de evento no Ubidots
		_uploadErrorLog(ceNoError, _ERR_UPLOAD_LOG_DONT_RESTART, _ERR_UPLOAD_LOG_SAVE_CONFIG);
	}
	else if (config->Values[ccLastError].toInt() == _ERR_NOERROR)  // uncrotoled restarted
	{
		_uploadErrorLog(ceUncrontrolledRestart, _ERR_UPLOAD_LOG_DONT_RESTART, _ERR_UPLOAD_LOG_DONT_SAVE_CONFIG);
		_uploadErrorLog(ceNoError, _ERR_UPLOAD_LOG_DONT_RESTART, _ERR_UPLOAD_LOG_SAVE_CONFIG);
	}
	else
	{
		_uploadErrorLog(ceNoError, _ERR_UPLOAD_LOG_DONT_RESTART, _ERR_UPLOAD_LOG_SAVE_CONFIG);
	}
}

void cremaClass::ShowSensorValues()
{
	if (time->IsTimeToAction(caShowSensorValues)) {
	
		visor->clearLine(0);
		visor->write(sensor->Values[csTemperatura], sensor->Decimals[csTemperatura]);
		visor->write(F("C    "));

		visor->write(sensor->Values[csUmidade], sensor->Decimals[csUmidade]);
		visor->write(F("%"));

		if (_whatShow) {
			visor->clearLine(2);
			visor->write(sensor->Values[csPressao], sensor->Decimals[csPressao]);
			visor->write(F("mP  "));

			visor->write(sensor->Values[csAltitude], sensor->Decimals[csAltitude]);
			visor->write(F("m"));

			visor->clearLine(3);
		}
		else {
			visor->clearLine(2);
			visor->write(sensor->Values[csLuminosidade], sensor->Decimals[csLuminosidade]);
			visor->writeln(F(" luz"));

			visor->clearLine(3);
			visor->write(sensor->Values[csUltraVioleta], sensor->Decimals[csUltraVioleta]);
			visor->write(F(" uv"));
		}
		_whatShow = !_whatShow;
	}
}

void cremaClass::ShowDateTime()
{
	if (time->IsTimeToAction(caShowDateTime, true)) {
		time->readTime();
		if (_timeSep == ":") {
			_timeSep = ".";
		}
		else {
			_timeSep = ":";
		}

		visor->clearLine(5);
		visor->write(time->strDMY(F("/"), true, true, false));
		visor->write(F(" - "));
		visor->write(time->strHMS(_timeSep, true, true, false));
	}
}

void cremaClass::ReadSensors()
{
	if (time->IsTimeToAction(caReadSensors))
	{
		if (!sensor->readSensors())   // TODO: false se erro na leitura de sensores (06/08/2018: Temp > 50)
		{
			_uploadErrorLog(ceSensorRead, _ERR_UPLOAD_LOG_RESTART, _ERR_UPLOAD_LOG_SAVE_CONFIG);
		}
	}
}

void cremaClass::_uploadToCloud(const cremaSensorsId first = csLuminosidade, const cremaSensorsId last = csUltraVioleta, const cremaErroDescription desc)
{
	//todo: verifica��o de conex�o antes da chamada
	if (!WiFi.isConnected()) 
	{
		config->setForceConfig(false);     // se necess�rio iniciar webServer, informar apenas dados de WiFi
		_wifi_autoConnect();
	}

	sensor->publishHTTP(first, last, desc);
}

//callback que indica que o ESP entrou no modo AP
void cremaClass::__wifi_configModeCallback(WiFiManager *myWiFiManager) 
{
	cremaClass::__displayConfigMode();
	cremaClass::__webServerConfigSaved = false;
}

//callback que indica que salvou as configura��es de rede
void cremaClass::__wifi_saveConfigCallback() 
{
	cremaClass::__webServerConfigSaved = true;
}

void cremaClass::__displayConfigMode()
{
	cremaVisorClass v;
	
	v.showMessage(F("_CONFIGURACAO_"));
	v.clearLine(1);
	v.clearLine(2); v.write(F("Conect. a rede"));
	v.clearLine(3); v.write(F("   \"")); v.write(_CREMA_SSID_AP); v.write(F("\""));
	v.clearLine(4); v.write(F("pelo computador"));
	v.clearLine(5); v.write(F("ou celular"));
}

void cremaClass::_initWiFi()
{
	//wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT(); 
	//esp_wifi_init(&config);
	//esp_wifi_start();
	//delay(500);

	pinMode(CREMA_WiFi_Manager_PIN, INPUT);

	//utilizando esse comando, as configura��es s�o apagadas da mem�ria
	//caso tiver salvo alguma rede para conectar automaticamente, ela � apagada.
	//_wifiManager.resetSettings();

	//por padr�o as mensagens de Debug v�o aparecer no monitor serial, caso queira desabilit�-la
	//utilize o comando setDebugOutput(false);
	_wifiManager.setDebugOutput(false);

	//caso queira iniciar o Portal para se conectar a uma rede toda vez, sem tentar conectar 
	//a uma rede salva anteriormente, use o startConfigPortal em vez do autoConnect
	//  _wifiManager.startConfigPortal(char const *apName, char const *apPassword = NULL);

	//setar IP fixo para o ESP (deve-se setar antes do autoConnect)
	//  setAPStaticIPConfig(ip, gateway, subnet);
	//  _wifiManager.setAPStaticIPConfig(IPAddress(192,168,16,2), IPAddress(192,168,16,1), IPAddress(255,255,255,0)); //modo AP

	//  setSTAStaticIPConfig(ip, gateway, subnet);
	//  _wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,99), IPAddress(192,168,0,1), IPAddress(255,255,255,0)); //modo esta��o

	//callback para quando entra em modo de configura��o AP
	_wifiManager.setAPCallback(__wifi_configModeCallback);

	//callback para quando se conecta em uma rede, ou seja, quando passa a trabalhar em modo esta��o
	_wifiManager.setSaveConfigCallback(__wifi_saveConfigCallback);

	//_wifiManager.autoConnect(_CREMA_SSID_AP, ""); //cria uma rede sem senha
	//_wifiManager.autoConnect(); //gera automaticamente o SSID com o chip ID do ESP e sem senha

	//  _wifiManager.setMinimumSignalQuality(10); // % minima para ele mostrar no SCAN

	//_wifiManager.setRemoveDuplicateAPs(false); //remover redes duplicadas (SSID iguais)
	//_wifiManager.resetSettings();
	_wifiManager.setConfigPortalTimeout(600); //timeout para o ESP nao ficar esperando para ser configurado para sempre
}

bool cremaClass::_wifi_autoConnect()
{
	if (!config->getConfigOk() || config->getForceConfig())
	{
		config->setForceConfig(false);

		WiFiManagerParameter * _wifiParam[ccCount];

		// The extra parameters to be configured (can be either global or just in the setup)
		// After connecting, parameter.getValue() will get you the configured value
		// id/name placeholder/prompt default length
		for (size_t i = 0; i < ccCount; i++)
		{
			cremaConfigId key = cremaConfigId(i);
			if (config->Imputable[key])
			{
				_wifiParam[key] = new WiFiManagerParameter(config->nameKeys[key].c_str(), config->descKeys[key].c_str(), config->Values[key].c_str(), _CREMA_CFG_VALUE_SIZE);
				//WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);

				//add all your parameters here
				_wifiManager.addParameter(_wifiParam[key]);
			}
		}
		_wifi_startWebServer();

		// todo: utilizar vari�vel global
		if (__webServerConfigSaved)
		{
			for (size_t i = 0; i < ccCount; i++)
			{
				cremaConfigId key = cremaConfigId(i);
				if (config->Imputable[key])
				{
					config->Values[key] = "";
					config->Values[key].concat(_wifiParam[key]->getValue());
				}
			}
			config->saveConfig();
		}

		for (size_t i = 0; i < ccCount; i++)
		{
			cremaConfigId key = cremaConfigId(i);
			if (config->Imputable[key])
			{
				delete _wifiParam[key];
			}
		}
	}
	else
	{
		visor->clearLine(2);
		visor->write(F("Conectando"));
		visor->clearLine(3);
		visor->write(F("WiFi..."));

		_wifiManager.autoConnect(_CREMA_SSID_AP, "");

		visor->clearLine(4);
		visor->write(_wifiManager.getSSID());
		delay(1500);
	}
	visor->clear();
}

void cremaClass::_wifi_startWebServer()
{
	if (!_wifiManager.startConfigPortal(_CREMA_SSID_AP)) {
		Restart();
	}
	visor->clear();
}

void cremaClass::_uploadErrorLog(const cremaErrorId error, const bool restart, const bool saveConfig)
{
	sensor->Values[csLog] = cremaErrors[error].code;                      // guarda valor do erro para subir para Ubidots
	_uploadToCloud(csLog, csLog, cremaErrors[error].description);

	if (saveConfig)
	{
		config->setLastError(error);   // guarda erro no arquivo do ESP32 localmente para ser tratado na reinicializa��o
	}

	if (restart)
	{
		Restart();
	}
}

void cremaClass::doGPS()
{
	_readGPS();
	_testGPSSignal();
}

void cremaClass::_readGPS()
{
	if (time->IsTimeToAction(caReadGPS))
	{
		sensor->readGPS();
	}
}

void cremaClass::_testGPSSignal()
{
	if (time->IsTimeToAction(caTestGPSSignal))
	{
		if (!sensor->gpsData.valid)
		{
			_uploadErrorLog(ceGPS_PoorSignal, _ERR_UPLOAD_LOG_DONT_RESTART, _ERR_UPLOAD_LOG_DONT_SAVE_CONFIG);
		}
	}
}

 //void __uploadSensorValues(void *parms)
 //{
	//IoT.publishHTTP(sensor, _IoT_Update_IniSensor, _IoT_Update_FimSensor);
	//vTaskDelete(NULL);
	//return;
 //}

void cremaClass::UploadSensorValues()
{
	if (time->IsTimeToAction(caUploadSensorsValues)) 
	{
		_uploadToCloud(_IoT_Update_IniSensor, _IoT_Update_FimSensor);
		// TODO Ler sensor em outra task do processador
		// https://www.dobitaobyte.com.br/selecionar-uma-cpu-para-executar-tasks-com-esp32/
		//xTaskHandle * taskUploadSensorValue;
		//xTaskCreatePinnedToCore(&__uploadSensorValues, "UploadSensorValues", 2048, NULL, 1, taskUploadSensorValue, 1);
		// utilizar mensagem de debug =printf("{created task to upload: %d.", taskUploadSensorValue);
	}
}

void cremaClass::Restart()
{
	//IoT.publishHTTP(sensor, csLog, csLog);  // upload � feito pela fun��o uploadErrorLog()
	esp_restart();
}