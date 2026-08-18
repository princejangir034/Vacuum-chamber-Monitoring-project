#include <Wire.h>
#include <SparkFun_MS5637_Arduino_Library.h>
#include <DHT.h>

// =========================
// PIN CONFIGURATION
// =========================
#define DHTPIN 2
#define DHTTYPE DHT22
#define RELAY_PIN 8

// =========================
// CONTROL SETTINGS
// =========================

// Maximum allowed temperature
#define MAX_TEMPERATURE 50.0

// Maximum allowed humidity
#define MAX_HUMIDITY 80.0

// Vacuum target pressure
// MS5637 gives absolute pressure.
// Pump will continue until pressure reaches this value.
#define TARGET_PRESSURE_PSI 5.0

// Hysteresis for pressure control
// Pump ON again only when pressure rises above this value.
#define PRESSURE_HYSTERESIS 0.5

// Therefore:
// Pump OFF at <= 5.0 PSI
// Pump can turn ON again at >= 5.5 PSI

// Number of pressure samples used for averaging
#define PRESSURE_SAMPLES 10

// Reading interval
#define SENSOR_INTERVAL 2000


// =========================
// SENSOR OBJECTS
// =========================

DHT dht(DHTPIN, DHTTYPE);
MS5637 pressureSensor;


// =========================
// VARIABLES
// =========================

float temperature = 0.0;
float humidity = 0.0;

float pressure_mbar = 0.0;
float pressure_psi = 0.0;

bool pumpRunning = false;

unsigned long lastSensorRead = 0;


// =========================
// PRESSURE READING
// =========================

float getAveragePressure()
{
  float total = 0.0;

  for (int i = 0; i < PRESSURE_SAMPLES; i++)
  {
    total += pressureSensor.getPressure();
    delay(20);
  }

  return total / PRESSURE_SAMPLES;
}


// =========================
// PUMP CONTROL
// =========================

void pumpON()
{
  digitalWrite(RELAY_PIN, LOW);
  pumpRunning = true;
}

void pumpOFF()
{
  digitalWrite(RELAY_PIN, HIGH);
  pumpRunning = false;
}


// =========================
// SETUP
// =========================

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);

  // Start with pump OFF
  pumpOFF();

  Serial.println();
  Serial.println("====================================");
  Serial.println("      VACUUM CONTROLLER STARTED");
  Serial.println("====================================");

  Serial.println("Initializing sensors...");

  // Initialize MS5637
  if (pressureSensor.begin() == false)
  {
    Serial.println("ERROR: MS5637 sensor not found!");
    Serial.println("Pump disabled for safety.");
    
    pumpOFF();

    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("MS5637 OK");
  Serial.println("DHT22 OK");

  Serial.println();
  Serial.println("Control Settings:");
  Serial.print("Maximum Temperature : ");
  Serial.print(MAX_TEMPERATURE);
  Serial.println(" C");

  Serial.print("Maximum Humidity    : ");
  Serial.print(MAX_HUMIDITY);
  Serial.println(" %");

  Serial.print("Target Pressure     : ");
  Serial.print(TARGET_PRESSURE_PSI);
  Serial.println(" PSI");

  Serial.print("Pressure ON Limit   : ");
  Serial.print(TARGET_PRESSURE_PSI + PRESSURE_HYSTERESIS);
  Serial.println(" PSI");

  Serial.println();
  Serial.println("System Ready.");
  Serial.println("------------------------------------");
}


// =========================
// MAIN LOOP
// =========================

void loop()
{
  // Read sensors every 2 seconds
  if (millis() - lastSensorRead < SENSOR_INTERVAL)
  {
    return;
  }

  lastSensorRead = millis();


  // =========================
  // READ DHT22
  // =========================

  float newTemperature = dht.readTemperature();
  float newHumidity = dht.readHumidity();


  // Check DHT22 readings
  if (isnan(newTemperature) || isnan(newHumidity))
  {
    Serial.println();
    Serial.println("ERROR: DHT22 reading failed!");
    Serial.println("Safety Action: Pump OFF");

    pumpOFF();

    return;
  }

  temperature = newTemperature;
  humidity = newHumidity;


  // =========================
  // READ PRESSURE
  // =========================

  pressure_mbar = getAveragePressure();

  // Convert mbar to PSI
  // 1 PSI = 68.9476 mbar

  pressure_psi = pressure_mbar / 68.9476;


  // =========================
  // SERIAL MONITOR DISPLAY
  // =========================

  Serial.println();

  Serial.println("----------- SENSOR DATA -----------");

  Serial.print("Temperature : ");
  Serial.print(temperature, 2);
  Serial.println(" C");

  Serial.print("Humidity    : ");
  Serial.print(humidity, 2);
  Serial.println(" %");

  Serial.print("Pressure    : ");
  Serial.print(pressure_psi, 2);
  Serial.println(" PSI");

  Serial.print("Pressure    : ");
  Serial.print(pressure_mbar, 2);
  Serial.println(" mbar");

  Serial.print("Pump Status : ");

  if (pumpRunning)
  {
    Serial.println("ON");
  }
  else
  {
    Serial.println("OFF");
  }


  // =========================
  // SAFETY: TEMPERATURE
  // =========================

  if (temperature >= MAX_TEMPERATURE)
  {
    pumpOFF();

    Serial.println();
    Serial.println("!!! TEMPERATURE LIMIT !!!");
    Serial.println("Temperature >= 50 C");
    Serial.println("ACTION: PUMP OFF");

    return;
  }


  // =========================
  // SAFETY: HUMIDITY
  // =========================

  if (humidity >= MAX_HUMIDITY)
  {
    pumpOFF();

    Serial.println();
    Serial.println("!!! HUMIDITY LIMIT !!!");
    Serial.println("Humidity >= 80%");
    Serial.println("ACTION: PUMP OFF");

    return;
  }


  // =========================
  // PRESSURE CONTROL
  // =========================

  // If pressure is at or below target,
  // required vacuum has been achieved.

  if (pressure_psi <= TARGET_PRESSURE_PSI)
  {
    pumpOFF();

    Serial.println();
    Serial.println("TARGET VACUUM REACHED");
    Serial.print("Pressure = ");
    Serial.print(pressure_psi, 2);
    Serial.println(" PSI");

    Serial.println("ACTION: PUMP OFF");

    return;
  }


  // =========================
  // START PUMP
  // =========================

  // Pump starts when pressure is above
  // target + hysteresis.

  if (pressure_psi >= (TARGET_PRESSURE_PSI + PRESSURE_HYSTERESIS))
  {
    pumpON();

    Serial.println();
    Serial.println("VACUUM REQUIRED");
    Serial.print("Pressure = ");
    Serial.print(pressure_psi, 2);
    Serial.println(" PSI");

    Serial.println("ACTION: PUMP ON");

    return;
  }


  // =========================
  // HYSTERESIS REGION
  // =========================

  Serial.println();
  Serial.println("PRESSURE HYSTERESIS REGION");

  if (pumpRunning)
  {
    Serial.println("ACTION: Pump remains ON");
  }
  else
  {
    Serial.println("ACTION: Pump remains OFF");
  }

  Serial.println("------------------------------------");
}