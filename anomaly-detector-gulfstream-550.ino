#include <DHT.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(5, 6, 10, 11, 12, 9);

#define DHTPIN 2
#define LEDPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// AI Parameters
const float alpha = 0.5;
const float thresholdMultiplier = 5.0;

// Aviation Limits
const float TEMP_MIN = 15.0, TEMP_MAX = 35.0;
const float HUM_MIN = 20.0, HUM_MAX = 80.0;

// Dynamic baselines
float smoothedTemp = 25.0, smoothedHum = 40.0;
float tempVariance = 1.0, humVariance = 1.0;
float prevHum = 40.0; // Tracks previous raw humidity

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  dht.begin();
  pinMode(LEDPIN, OUTPUT);
  
  lcd.print("Aviation Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  static unsigned long lastUpdate = 0;
  const float MAX_HUM_DELTA = 15.0;

  if(millis() - lastUpdate >= 2000) {
    lastUpdate = millis();
    
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    
    if(!isnan(newTemp) && !isnan(newHum)) {
      // Calculate rate-of-change
      float humDelta = abs(newHum - prevHum);
      
      // Update adaptive baselines
      smoothedTemp = alpha*newTemp + (1-alpha)*smoothedTemp;
      smoothedHum = alpha*newHum + (1-alpha)*smoothedHum;
      
      // Update variances
      tempVariance = alpha*pow(newTemp - smoothedTemp, 2) + (1-alpha)*tempVariance;
      humVariance = alpha*pow(newHum - smoothedHum, 2) + (1-alpha)*humVariance;
      
      // Calculate thresholds
      float tempThreshold = thresholdMultiplier * sqrt(tempVariance);
      float humThreshold = thresholdMultiplier * sqrt(humVariance);
      
      // Anomaly detection
      bool tempAnomaly = (abs(newTemp - smoothedTemp) > tempThreshold) || 
                        (newTemp < TEMP_MIN) || (newTemp > TEMP_MAX);
                        
      bool humAnomaly = (abs(newHum - smoothedHum) > humThreshold) ||
                        (newHum < HUM_MIN) || (newHum > HUM_MAX) ||
                        (humDelta > MAX_HUM_DELTA); // Use pre-update prevHum
      
      bool anomaly = tempAnomaly || humAnomaly;
      digitalWrite(LEDPIN, anomaly);
      updateDisplay(newTemp, newHum, anomaly);
      
      prevHum = newHum;
    }
  }
}

void updateDisplay(float temp, float hum, bool anomaly) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temp,1); lcd.write(223); lcd.print("C");
  lcd.setCursor(8, 0);
  lcd.print("H:"); lcd.print(hum,0); lcd.print("%");
  
  lcd.setCursor(0, 1);
  lcd.print(anomaly ? "ALERT! Check Env" : "All Systems OK");
}
