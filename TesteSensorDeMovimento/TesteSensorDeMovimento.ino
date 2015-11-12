// Programa : Sensor de presenca com modulo PIR
// Autor : Arduino e Cia

int temgente = 5; //Pino ligado ao led vermelho
int temninguem = 3; //Pino ligado ao led azul
int sensor = 6;  //Pino ligado ao sensor PIR
int acionamento;  //Variavel para guardar valor do sensor

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(50);

  pinMode(temgente, OUTPUT); //Define pino como saida
  pinMode(temninguem, OUTPUT); //Define pino como saida
  pinMode(sensor, INPUT);   //Define pino sensor como entrada
  
  

}

void loop()
{
 acionamento = digitalRead(sensor); //Le o valor do sensor PIR
 if (acionamento == LOW)  //Sem movimento, mantem led azul ligado
 {
    digitalWrite(temgente, LOW);
    digitalWrite(temninguem, HIGH);
    Serial.println("no one");
 }
 else  //Caso seja detectado um movimento, aciona o led vermelho
 {
    digitalWrite(temgente, HIGH);
    digitalWrite(temninguem, LOW);
    Serial.println("moviment");
 }
 delay(1000);

}
