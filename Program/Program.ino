#include <Wire.h> // Biblioteca pentru LCD
#include <LiquidCrystal_I2C.h> // Biblioteca pentru comunicatia I2C

#include <dht.h> // Biblioteca pentru DTH

#define R_L_mq2 4.7 // rezistenta RL_MQ-2 in kohm
#define R_L_mq135 20.0 // rezistenta RL_MQ-135 in kohm

#define R_0_mq2 0.4 // rezistenta R0_MQ-2 determinata la calibrare in aer curat
#define R_0_mq135 1.5 // rezistenta R0_MQ-135 determinata la calibrare in aer curat

#define BIG_SENS 3
#define NORMAL_SENS 2
#define SMALL_SENS 1

#define SENS_BTN 2
#define ON_OFF_BTN 7

/*
Senzor de fum:
Tigara langa senzorii MQ-2, MQ-135 si Sharp_GP2Y1010AU0F
Pragurile de alerta alese:
    ->  1000ppm    - fum    - MQ-2 
    ->  0.45 mg/m3 - fum    - Sharp_GP2Y1010AU0F
    ->  0.7ppm     - toluen - MQ-135
    ->  5ppm       - CO     - MQ-135
    ->  1000ppm    - CO2    - MQ-135
Daca minim 2 din acestea sunt peste prag => dau alarma
*/

dht DHT;

LiquidCrystal_I2C lcd(0x27, 16, 4); // 0x27 adresa I2C, LCD1604 (16coloane & 4linii)
LiquidCrystal_I2C lcd_mic(0x26, 16, 2); // 0x26 adresa I2C, LCD1602 (16coloane & 2linii)

#define dhtPin A2 // pinul pentru Date senzor de temperatura si umiditate - DTH11 

#define sharpPin A3 // pinul pentru Vout senzor de fum - Sharp_GP2Y1010AU0F
#define sharpLed 11 // pinul pentru control LED IR senzor de fum - Sharp_GP2Y10

#define mq135Pin A1 // pinul pentru Vout senzor gaze MQ-135
#define mq2Pin A0 // pinul pentru Vout senzor gaze si fum MQ-2

#define ledAlert 6 // pinul la care este conectat ledul de avertizare
#define buzzAlert 3 // pinul la care este conectat buzzerul de avertizare

int lastDebounce; // folosit pentru a elimina problema de bounce a butonului apasat
int sensitive; // retine senzitivitatea senzorilor 
// => cati senzori semnalizeaza pericol simultan pentru a da alarma
int on_off; // spune daca alarma este pornita sau nu
int lastDebounce_on_off;

void change_sensitivity() {
  if(millis() - lastDebounce > 500) {
    lastDebounce = millis();
    sensitive ++;
    if(sensitive == BIG_SENS + 1)
      sensitive = 1;
    Serial.print("CHANGED SENSITIVITY!");
    Serial.println(sensitive);
    }
}

void on_off_modif() {
  if(millis() - lastDebounce > 500) {
    if(on_off == 0) {
      Serial.println();
      Serial.println("ON");
      on_off = 1;
    } else {
      on_off = 0;
      Serial.println();
      Serial.println("OFF");
    }
  }
}


void setup() {
  pinMode (buzzAlert, OUTPUT); // iesire ce comanda buzzer-ul
  pinMode (ledAlert, OUTPUT); // iesire ce comanda aprindere/stingerea ledului
  pinMode (mq2Pin, INPUT); // se configureaza pinul A0 ca intrare (MQ-2)
  pinMode (mq135Pin, INPUT); // se configureaza pinul A1 ca intrare (MQ-135)
  pinMode (dhtPin, INPUT); // se configureaza pinul A2 ca intrare (DTH11)
  pinMode (sharpPin, INPUT); // se configureaza pinul A4 ca intrare (Sharp)
  pinMode(sharpLed, OUTPUT); // iesirea pentru control LED IR senzor de fum (Sharp)
  digitalWrite (sharpLed, HIGH); // (HIGH --> led IR initial oprit)
  digitalWrite (ledAlert, LOW); // led de alerta initial stins
  digitalWrite (buzzAlert, HIGH); // buzzer de alerta initial oprit

  lcd.init(); // initializeaza LCD1604
  lcd.backlight();
  lcd.clear();

  lcd_mic.init(); // initializeaza LCD1604
  lcd_mic.backlight();
  lcd_mic.clear();

  Serial.begin(9600); // comunicatia seriala cu baud rate = 9600bps

  Serial.println("\t tensiune mq2 \t | \t tensiune mq135 \t | \t temperatura \t\t | \t\t umiditate \t | \t concentratie FUM \t | \t densitate FUM");

  pinMode(SENS_BTN, INPUT_PULLUP);
  pinMode(ON_OFF_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENS_BTN), change_sensitivity, FALLING);
  attachInterrupt(digitalPinToInterrupt(ON_OFF_BTN), on_off_modif, RISING);

  sensitive = NORMAL_SENS;
  lastDebounce = millis();
  on_off = 1;
  lastDebounce_on_off = millis();
}

void loop() {
  // THE LINE MUST START WITH > FOR PYTHON
  Serial.print(">\t");
  
  // senzor gaz si fum MQ-2
  float tensiune_mq2 = (analogRead(mq2Pin)/1023.0)*5.0;
  Serial.print(tensiune_mq2);     // Vout MQ-2 [V]
  Serial.print(" \t\t | \t\t");

  // senzor gaz MQ-135
  float tensiune_mq135 = (analogRead(mq135Pin)/1023.0)*5.0;
  Serial.print(tensiune_mq135);  // Vout MQ-135 [V]
  Serial.print(" \t\t | \t\t");
    
  // senzor temperatura si umiditate DHT11
  int info = DHT.read11(dhtPin); // info de la senzorul de umiditate si temperatura
  int temp = DHT.temperature;
  int humid = DHT.humidity;
  Serial.print(temp); // temperatura curenta [grade C]
  Serial.print(" \t\t | \t\t ");
  Serial.print(humid); // umiditatea relativa curenta [%]
  Serial.print(" \t\t | \t\t ");

  // senzor fum SHARP GP2Y10
  digitalWrite(sharpLed, LOW); // (LOW --> led IR pornit)
  delayMicroseconds(280); // asteptare totala de 32 milisecunde
  float tensiune_sharp = analogRead(sharpPin) * 5.0 / 1023.0;
  delayMicroseconds(40);
  digitalWrite(sharpLed, HIGH);
  delayMicroseconds(9680);
  float dens_fum; // mg/m^3
  if(tensiune_sharp <= 0.59) {
    dens_fum = 0;
  } 
  else 
  if(tensiune_sharp > 0.59 && tensiune_sharp < 3.58) {
    dens_fum = tensiune_sharp * 0.174 - 0.102;
  } 
  else 
  if(tensiune_sharp <= 3.58 && tensiune_sharp <= 5) {
    dens_fum = tensiune_sharp * 3.5 - 12.01;
  }
  
  Serial.print(tensiune_sharp);
  Serial.print(" \t\t | \t\t ");
  Serial.print(dens_fum, 4); // densitate fum [mg/m3]
  Serial.println("");

  // afisare info pe lcd
  // temperatura – DTH11
  lcd_mic.setCursor(0,0); // misca cursorul la pozitia (0, 0)
  lcd_mic.print("T="); // afiseaza mesaj la pozitia (0, 0)
  lcd_mic.print(temp); // afiseaza mesaj la pozitia (2, 0)
  lcd_mic.print("*C"); // afiseaza mesaj la pozitia (4, 0)
  // umiditate
  lcd_mic.print(" UMID="); // afiseaza mesaj la pozitia (6, 0)
  lcd_mic.print(humid); // afiseaza mesaj la pozitia (13,0)
  lcd_mic.print("% "); // afiseaza mesaj la pozitia (15, 0)

  lcd_mic.setCursor(0,1); // misca cursorul la pozitia (0, 1)
  // FUM ppm - MQ-2
  lcd_mic.print("FUM:");
  float smoke_conc = concentratie_mq2(temp, humid, tensiune_mq2, 4955.53, -2.58);
  lcd_mic.print(smoke_conc, 1);
  lcd_mic.print(" ");
  
  // FUM densitate - Sharp
  lcd_mic.print(dens_fum, 4);

  lcd.setCursor(0,0);
  if(on_off == 0)
    // on/off alarma
    lcd.print("OFF");
  else
  {
    lcd.print("Sensitiv:       ");
    lcd.setCursor(10,0);
    if(sensitive == BIG_SENS)
      lcd.print("BIG");
    else if(sensitive == NORMAL_SENS)
      lcd.print("NORMAL");
    else if(sensitive == SMALL_SENS)
      lcd.print("SMALL");
  }
  
  lcd.setCursor(0,1);
  //Toluen - MQ-135
  lcd.print("Toluen:");
  float toluen_conc = concentratie_mq135(temp, humid, tensiune_mq135, 45.004, -3.499);
  lcd.print(toluen_conc, 2);

  lcd.setCursor(-4,2);
  //CO – MQ-135
  lcd.print("CO:");
  float co_conc = concentratie_mq135(temp, humid, tensiune_mq135, 605, -3.936);
  lcd.print(co_conc, 2);
  lcd.print(" ");
  
  //CO2 – MQ-2
  lcd.setCursor(-4,3);
  lcd.print("CO2:");
  float co2_conc = concentratie_mq135(temp, humid, tensiune_mq135, 112.351, -2.898) + 400;
  lcd.print(co2_conc, 2);
  
  if(ciggarete_check(smoke_conc, dens_fum, toluen_conc, co_conc, co2_conc, temp) >= sensitive) {
    digitalWrite (ledAlert, HIGH);
    buzz(100);
  }
  else
    digitalWrite (ledAlert, LOW);
  delay(400);
}

float concentratie_mq135(int t, int h, float V_mq135, float a, float b) {
  float z = extrapolat_mq135(t, h);
  float f = z / extrapolat_mq135(20, 65);
  float Rs_t_h = R_L_mq135 * (5.0 / V_mq135 - 1);
  if(Rs_t_h < 0)
    Rs_t_h = 0;
  float Rs_20_65 = Rs_t_h / f;
  float ratio = Rs_20_65 / R_0_mq135;
  float conc = a * pow(ratio, b);
  return conc;
}

float concentratie_mq2(int t, int h, float V_mq2, float a, float b) {
  float z = extrapolat_mq2(t, h);
  float f = z / extrapolat_mq2(20, 65);
  float Rs_t_h = ((R_L_mq2 * 5.0) / V_mq2) - R_L_mq2;
  if(Rs_t_h < 0)
    Rs_t_h = 0;
  float Rs_20_65 = Rs_t_h / f;
  float ratio = Rs_20_65 / R_0_mq2;
  float conc = a * pow(ratio, b);
  return conc;
}

float extrapolat_mq2(int t, int h) { // temperatura si umiditatea
  // valori Temperatura: 0 – 50 grade C  & valori RH: 0 – 100 %
  // valorile obtinute prin interpolare in Matlab pentru umiditate = 33% si 85%
  float all_h85[] = { 
                1.253,1.2288,1.2046,1.1804,1.1562,1.132,1.1126,1.0932,1.0738,1.0544,
                1.035,1.0188,1.0026,0.9864,0.9702,0.954,0.946,0.938,0.93,0.922,0.914,
                0.9082,0.9024,0.8966,0.8908,0.885,0.8826,0.8802,0.8778,0.8754,0.873,
                0.8708,0.8686,0.8664,0.8642,0.862,0.8596,0.8572,0.8548,0.8524,0.85,
                0.8466,0.8432,0.8398,0.8364,0.833,0.8318,0.8306,0.8294,0.8282,0.827
              }; 
  float all_h33[] = {
                1.363,1.341,1.319,1.297,1.275,1.253,1.23,1.207,1.184,1.161,1.138,1.1196,
                1.1012,1.0828,1.0644,1.046,1.0368,1.0276,1.0184,1.0092,1,0.9942,0.9884,
                0.9826,0.9768,0.971,0.9676,0.9642,0.9608,0.9574,0.954,0.9516,0.9492,
                0.9468,0.9444,0.942,0.9432,0.9444,0.9456,0.9468,0.948,0.9446,0.9412,
                0.9378,0.9344,0.931,0.9264,0.9218,0.9172,0.9126,0.908                
              };  
  return (all_h85[t] - all_h33[t]) / (85 - 33) * (h - 33) + all_h33[t];
}

float extrapolat_mq135(int t, int h) { // temperatura si umiditatea
  // valori Temperatura 0 – 50 grade C  & valori RH: 0 – 100 %
  // valorile obtinute prin interpolare in matlab pentru humidity = 33% si 85%
  float all_h85[] = { 
                1.238,1.2156,1.1932,1.1708,1.1484,1.126,1.1068,1.0876,1.0684,1.0492,
                1.03,1.0132,0.9964,0.9796,0.9628,0.946,0.9392,0.9324,0.9256,0.9188,
                0.912,0.9052,0.8984,0.8916,0.8848,0.878,0.8758,0.8736,0.8714,0.8692,
                0.867,0.8636,0.8602,0.8568,0.8534,0.85,0.8478,0.8456,0.8434,0.8412,
                0.839,0.8356,0.8322,0.8288,0.8254,0.822,0.8208,0.8196,0.8184,0.8172,0.816                
              }; 
  float all_h33[] = {
                1.378,1.35,1.322,1.294,1.266,1.238,1.2144,1.1908,1.1672,1.1436,1.12,
                1.102,1.084,1.066,1.048,1.03,1.024,1.018,1.012,1.006,1,0.9902,0.9804,
                0.9706,0.9608,0.951,0.9488,0.9466,0.9444,0.9422,0.94,0.9366,0.9332,
                0.9298,0.9264,0.923,0.9242,0.9254,0.9266,0.9278,0.929,0.9256,0.9222,
                0.9188,0.9154,0.912,0.9098,0.9076,0.9054,0.9032,0.901                
              };  
  return (all_h85[t] - all_h33[t]) / (85 - 33) * (h - 33) + all_h33[t];
}

int ciggarete_check(
  float smoke_conc, float smoke_density, float toluen_conc, 
  float co_conc, float co2_conc, float temp) {
  int count = 0;
  
  if(smoke_conc >= 17 || co2_conc >= 500) {
    count ++;
    Serial.print("MQ2 ");
  }

  if(smoke_density >= 0.45) {
    count ++;
    Serial.print("SharpLed ");
  }
  
  if(toluen_conc >= 4 || co_conc >= 5){
    count ++;
    Serial.print("MQ135 ");
  }
  
  if(temp >= 45) {
    Serial.print("DHT_Temp ");
    count ++;
  }

  if(count > 0)
    Serial.println();

  return count;
}

void buzz(int delay_ms) {
  if(on_off == 0)
    return;
  digitalWrite (buzzAlert, LOW);
  delay(10);
  digitalWrite (buzzAlert, HIGH);
}
