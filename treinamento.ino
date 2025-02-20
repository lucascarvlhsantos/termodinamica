#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <PID_v1.h>
#include <FS.h>
#include <SPIFFS.h>

#define ONE_WIRE_BUS 4
#define pinoBomba 16
#define TAMANHO_ARRAY 6  // Para registrar 6 intervalos de 10 segundos em 1 minuto
#define TEMPO_MAXIMO 60000  // 1 minuto

struct DadosPID {
  float Kp;
  float setpoint;
  float resposta[TAMANHO_ARRAY]; 
  float tempo[TAMANHO_ARRAY];
  float pwm[TAMANHO_ARRAY]; 
  bool sucesso;
  int contador_dados;
};

DadosPID dataset[10];
int indice_dataset = 0;

// CONFIGURAÇÃO SENSOR TEMPERATURA
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// VARIÁVEIS AUXILIARES
float variacaoMaximaDeTemperatura;
float Kp_Max;
double Kp;
double setpoint;
float temperaturaC;
float erro, erro_anterior;
unsigned long tempo_inicial;
unsigned long tempo_passado;

// Variáveis do PID
double input, output;
double Ki = 0.0, Kd = 0.0;
PID myPID(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

void setup() {
  Serial.begin(9600);
  sensors.begin();
  
  // Inicializa o sistema de arquivos SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar o sistema de arquivos SPIFFS");
    return;
  }

  Serial.println("Digite a variação máxima de temperatura:");
  while (Serial.available() == 0);
  variacaoMaximaDeTemperatura = Serial.parseFloat();
  Kp_Max = 255.0 / variacaoMaximaDeTemperatura;
  
  randomSeed(analogRead(0)); // Garante valores aleatórios
}

void loop() {
  while (indice_dataset < 10) {
    iniciarNovaIteracao();
    executarControle();
  }

  // Após a execução de todas as iterações, exporta os dados para o arquivo .txt
  salvarDatasetEmTxt();
}

void iniciarNovaIteracao() {
  setpoint = random(13, 26);
  Kp = random(5, (int)Kp_Max + 1);
  myPID.SetTunings(Kp, Ki, Kd);

  erro_anterior = 9999; // Valor alto para garantir atualização inicial
  tempo_inicial = millis();
  tempo_passado = tempo_inicial;
  sensors.requestTemperatures();
  temperaturaC = sensors.getTempCByIndex(0);
  erro = setpoint - temperaturaC;
  input = temperaturaC;
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(95, 255);
}

void executarControle() {
  unsigned long tempo_atual;
  int contador = 0;
  bool sucesso = false;

  // Verifica continuamente por 1 minuto
  while (millis() - tempo_inicial < TEMPO_MAXIMO) {  
    tempo_atual = millis();

    // A cada 10 segundos
    if (tempo_atual - tempo_passado >= 10000 && contador < TAMANHO_ARRAY) {  
      tempo_passado = tempo_atual;

      sensors.requestTemperatures();
      temperaturaC = sensors.getTempCByIndex(0);
      erro = setpoint - temperaturaC;
      input = temperaturaC;
      myPID.Compute();
      float pwm = constrain((255 - output), 95, 255);
      analogWrite(pinoBomba, pwm);

      // Armazena os dados a cada 10 segundos
      DadosPID &dados = dataset[indice_dataset];
      dados.Kp = Kp;
      dados.setpoint = setpoint;
      dados.resposta[contador] = temperaturaC;
      dados.tempo[contador] = (millis() - tempo_inicial) / 1000.0;
      dados.pwm[contador] = pwm;
      dados.sucesso = sucesso;
      dados.contador_dados++;
      contador++;

      // Imprime os dados no monitor serial
      Serial.print("Kp: "); Serial.println(dados.Kp);
      Serial.print("Setpoint: "); Serial.println(dados.setpoint);
      Serial.print("Última temperatura: "); Serial.println(dados.resposta[contador-1]);
      Serial.print("Último tempo: "); Serial.println(dados.tempo[contador-1]);
      Serial.print("Último PWM: "); Serial.println(dados.pwm[contador-1]);
      Serial.print("Sucesso: "); Serial.println(dados.sucesso ? "Sim" : "Não");
      Serial.print("Contador de dados: "); Serial.println(dados.contador_dados);
    }

    erro = setpoint - temperaturaC;

    // Condição de sucesso se erro for pequeno o suficiente
    if (abs(erro) <= 0.5) {
      sucesso = true;
      break;
    }

    // Se passou o tempo máximo ou erro muito grande, a iteração termina
    if ((millis() - tempo_inicial >= TEMPO_MAXIMO) && (erro - erro_anterior > 2)) {
      break;
    }

    // Atualiza o erro anterior
    if ((erro_anterior - erro) >= 2) {
      erro_anterior = erro;
      tempo_inicial = millis(); // Reinicia o tempo para nova avaliação
    }

    delay(2000);
  }

  dataset[indice_dataset].sucesso = sucesso;
  indice_dataset++;
}

void salvarDatasetEmTxt() {
  // Verifica espaço disponível no SPIFFS antes de gravar
  uint32_t totalBytes = SPIFFS.totalBytes();
  uint32_t usedBytes = SPIFFS.usedBytes();
  Serial.print("Espaço total: "); Serial.println(totalBytes);
  Serial.print("Espaço usado: "); Serial.println(usedBytes);

  // Abre o arquivo para escrita
  File file = SPIFFS.open("/dataset.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Erro ao abrir o arquivo para escrita");
    return;
  }

  // Adiciona timestamp de quando os dados começaram a ser registrados
  String timestamp = String(millis());
  file.println("Timestamp de coleta de dados: " + timestamp);

  // Grava os dados no arquivo
  for (int i = 0; i < 10; i++) {
    DadosPID &dados = dataset[i];
    file.print("Iteração "); file.println(i + 1);
    file.print("Kp: "); file.println(dados.Kp);
    file.print("Setpoint: "); file.println(dados.setpoint);
    
    for (int j = 0; j < TAMANHO_ARRAY; j++) {
      file.print("Tempo: "); file.print(dados.tempo[j]); file.print("s | ");
      file.print("Temperatura: "); file.print(dados.resposta[j]); file.print("C | ");
      file.print("PWM: "); file.println(dados.pwm[j]);
    }

    file.print("Sucesso: "); file.println(dados.sucesso ? "Sim" : "Não");
    file.println();
  }

  file.close();
  Serial.println("Dados exportados para o arquivo dataset.txt");
}
