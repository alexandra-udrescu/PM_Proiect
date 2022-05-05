#include <dht.h> //DTHlib

dht DHT;

#define dhtPin A2   // pinul la care este conectat senzorul de temperatura si umiditate
#define mq2Pin A0   // Vout senzor MQ-2 este conectat pe pinul A0

#define R_L 5.0 // tensiunea Rl in kohm

void setup() {
  pinMode (mq2Pin, INPUT);      // se configureaza pinul A0 ca intrare
  pinMode (dhtPin, INPUT);      // se configureaza pinul A2 ca intrare
  Serial.begin(9600);           // comunicatia seriala cu baud rate = 9600bps
}

 

void loop() {

  //Serial.println("\n\n--------------");
  // senzor gaz MQ2
  float tensiune_mq2 = (analogRead(mq2Pin)/1023.0)*5.0;
  Serial.print("tensiune mq2= ");
  Serial.print(tensiune_mq2);
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

  float R_t_h_crt_mq2 = R_mq2_from_V_mq2(tensiune_mq2);
  float R_20_65_mq2 = convert_R_t_h_to_R_20_65(temp, humid, R_t_h_crt_mq2);
  float R0_20_65_mq2 = get_R0_from_R_20_65(R_20_65_mq2);
  Serial.println("Rezistenta senzorului la temperatura si umiditatea curente:");
  Serial.println(R_t_h_crt_mq2, 4);
  Serial.println("Rezistenta senzorului la temperatura 20*C si umiditatea 65%:");
  Serial.println(R_20_65_mq2, 4);
  Serial.println("Rezistenta R0_20_65:");
  Serial.println(R0_20_65_mq2, 4);
  delay(100);

}

float get_R0_from_R_20_65(float R_20_65) {
  return R_20_65 * extrapolat(20, 65) / 9.26;
}

float convert_R_t_h_to_R_20_65(int t, int h, float R_t_h) {
  return R_t_h / extrapolat(t,h);
}

float R_mq2_from_V_mq2(float V_mq2) {
  return R_L * (5.0 / V_mq2 - 1);
}

float extrapolat(int t, int h) { // temperatura si umiditatea
  // valori t: 0 - 50
  // valori h: 0 - 100
  // valorile obtinute prin interpolare in matlab pentru humidity = 33% si 85%
  float all_h85[] = {
                1.253,1.2288,1.2046,1.1804,1.1562,1.132,1.1126,1.0932,1.0738,
                1.0544,1.035,1.0188,1.0026,0.9864,0.9702,0.954,0.946,0.938,
                0.93,0.922,0.914,0.9082,0.9024,0.8966,0.8908,0.885,0.8826,
                0.8802,0.8778,0.8754,0.873,0.8708,0.8686,0.8664,0.8642,0.862,
                0.8596,0.8572,0.8548,0.8524,0.85,0.8466,0.8432,0.8398,0.8364,
                0.833,0.8318,0.8306,0.8294,0.8282,0.827
              };
  float all_h33[] = {
                1.363,1.341,1.319,1.297,1.275,1.253,1.23,1.207,1.184,1.161,
                1.138,1.1196,1.1012,1.0828,1.0644,1.046,1.0368,1.0276,1.0184,
                1.0092,1,0.9942,0.9884,0.9826,0.9768,0.971,0.9676,0.9642,0.9608,
                0.9574,0.954,0.9516,0.9492,0.9468,0.9444,0.942,0.9432,0.9444,
                0.9456,0.9468,0.948,0.9446,0.9412,0.9378,0.9344,0.931,0.9264,
                0.9218,0.9172,0.9126,0.908
              };  
  return (all_h85[t] - all_h33[t]) / (85 - 33) * (h - 33) + all_h33[t];
}
