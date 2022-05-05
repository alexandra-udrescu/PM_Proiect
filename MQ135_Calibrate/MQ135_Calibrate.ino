#include <dht.h> //DTHlib

dht DHT;

#define dhtPin A2   // pinul la care este conectat senzorul de temperatura si umiditate
#define mq135Pin A1   // Vout senzor MQ-135 este conectat pe pinul A0

#define R_L 20.0 // tensiunea Rl in kohm

void setup() {
  pinMode (mq135Pin, INPUT);      // se configureaza pinul A0 ca intrare
  pinMode (dhtPin, INPUT);      // se configureaza pinul A2 ca intrare
  Serial.begin(9600);           // comunicatia seriala cu baud rate = 9600bps
}

 

void loop() {

  //Serial.println("\n\n--------------");
  // senzor gaz mq135
  float tensiune_mq135 = (analogRead(mq135Pin)/1023.0)*5.0;
  Serial.print("tensiune mq135= ");
  Serial.print(tensiune_mq135);
  Serial.println("V");

  // senzor temperatura si umiditate DHT11
  int info = DHT.read11(dhtPin); // info de la senzorul de umiditate&temp
  int temp = DHT.temperature;
  int humid = DHT.humidity;
  Serial.print(temp);
  Serial.print("*C   ");
  Serial.print(humid);
  Serial.print("%   ");
  Serial.println(extrapolat(temp, humid), 4);
  Serial.println("--------------");

  float R_t_h_crt_mq135 = R_mq135_from_V_mq135(tensiune_mq135);
  float R_20_65_mq135 = convert_R_t_h_to_R_20_65(temp, humid, R_t_h_crt_mq135);
  float R0_20_65_mq135 = get_R0_from_R_20_65(R_20_65_mq135);
  Serial.println("Rezistenta senzorului la temperatura si umiditatea curente:");
  Serial.println(R_t_h_crt_mq135, 4);
  Serial.println("Rezistenta senzorului la temperatura 20*C si umiditatea 65%:");
  Serial.println(R_20_65_mq135, 4);
  Serial.println("Rezistenta R0_20_65:");
  Serial.println(R0_20_65_mq135, 4);
  delay(100);

}

float get_R0_from_R_20_65(float R_20_65) {
  return R_20_65 * extrapolat(20, 65) / 3.6;
}

float convert_R_t_h_to_R_20_65(int t, int h, float R_t_h) {
  return R_t_h / extrapolat(t,h);
}

float R_mq135_from_V_mq135(float V_mq135) {
  return R_L * (5.0 / V_mq135 - 1);
}

float extrapolat(int t, int h) { // temperatura si umiditatea
  // valori t: 0 - 50
  // valori h: 0 - 100
  // valorile obtinute prin interpolare in matlab pentru humidity = 33% si 85%
  float all_h85[] = {
                1.238,1.2156,1.1932,1.1708,1.1484,1.126,1.1068,1.0876,1.0684,
                1.0492,1.03,1.0132,0.9964,0.9796,0.9628,0.946,0.9392,0.9324,
                0.9256,0.9188,0.912,0.9052,0.8984,0.8916,0.8848,0.878,0.8758,
                0.8736,0.8714,0.8692,0.867,0.8636,0.8602,0.8568,0.8534,0.85,
                0.8478,0.8456,0.8434,0.8412,0.839,0.8356,0.8322,0.8288,0.8254,
                0.822,0.8208,0.8196,0.8184,0.8172,0.816
              };
  float all_h33[] = {
                1.378,1.35,1.322,1.294,1.266,1.238,1.2144,1.1908,1.1672,1.1436,
                1.12,1.102,1.084,1.066,1.048,1.03,1.024,1.018,1.012,1.006,1,
                0.9902,0.9804,0.9706,0.9608,0.951,0.9488,0.9466,0.9444,0.9422,
                0.94,0.9366,0.9332,0.9298,0.9264,0.923,0.9242,0.9254,0.9266,
                0.9278,0.929,0.9256,0.9222,0.9188,0.9154,0.912,0.9098,0.9076,
                0.9054,0.9032,0.901
              };  
  return (all_h85[t] - all_h33[t]) / (85 - 33) * (h - 33) + all_h33[t];
}
