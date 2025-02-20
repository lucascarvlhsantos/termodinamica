#include <WiFi.h>
#include "esp_eap_client.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

#define ONE_WIRE_BUS 4
#define pinoBomba 16
#define TAMANHO_JANELA 10

// DEFINIÇÕES DO SISTEMA OPERACIONAL
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// CONFIGURAÇÕES INICIAIS
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WebSocketsServer webSocket(81);

// VARIÁVEIS AUXILIARES
String redeEscolhida = "";
double Setpoint, Input, Output, SaidaBomba;
float bufferErro[TAMANHO_JANELA] = {0};
int indiceBuffer = 0;
float somaErros = 0.0;

// DEFINE CONSTANTES
double Kp_agress = 12, Ki_agress = 6, Kd_agress = 0.5;
double Kp_suav = 6, Ki_suav = 3, Kd_suav = 2;

// FUNÇÕES AUXULIARES
void limparBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

void imprimirLinhaDecorativa() {
  Serial.println("==============================================");
}

// GERENCIAR CONEXÃO À REDE

String obterTipoRede(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_OPEN: return "Rede aberta";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA-Personal";
    case WIFI_AUTH_WPA2_PSK: return "WPA2-Personal";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-Personal";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    default: return "Tipo desconhecido";
  }
}

int buscarRedes() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  WiFi.scanDelete();
  delay(500);

  return WiFi.scanNetworks();
}

void mostrarRedes(int numeroDeRedes) {
  if (numeroDeRedes == 0 || numeroDeRedes < 0) {
    Serial.println(">>> Nenhuma rede encontrada ou houve um erro durante a varredura. <<<");
  } else {
    Serial.println(">>> Redes encontradas: <<<");
    for (int i = 0; i < numeroDeRedes; i++) {
      Serial.print(">>> ");
      Serial.print(i + 1);
      Serial.print(". ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" | ");
      Serial.println(obterTipoRede(WiFi.encryptionType(i)));
    }
  }
  imprimirLinhaDecorativa();
}

void escolherRede(String& ssid, String& senha, String& usuario) {
  imprimirLinhaDecorativa();
  Serial.println(">>> Buscando redes disponíveis... <<<");

  int numeroDeRedes = buscarRedes();
  mostrarRedes(numeroDeRedes);

  int rede = -1;

  limparBuffer();
  while (rede < 1 || rede > numeroDeRedes) {
    Serial.println(">>> Digite o número da rede desejada: <<<");
    while (!Serial.available());
    rede = Serial.readString().toInt();
  }

  ssid = WiFi.SSID(rede - 1);
  wifi_auth_mode_t authMode = WiFi.encryptionType(rede - 1);

  if (authMode == WIFI_AUTH_OPEN) {
    senha = "";
    usuario = "";
    Serial.println(">>> Conectando a rede: " + ssid + " <<<");

  } else if (authMode == WIFI_AUTH_WPA2_ENTERPRISE) {
    Serial.println(">>> Digite o usuário para: " + ssid + " <<<");
    while (!Serial.available());
    usuario = Serial.readString();
    usuario.trim();

    Serial.println(">>> Digite a senha para: " + ssid + " <<<");
    while (!Serial.available());
    senha = Serial.readString();
    senha.trim();

  } else {
    usuario = "";
    Serial.println(">>> Digite a senha para: " + ssid + " <<<");
    while (!Serial.available());
    senha = Serial.readString();
    senha.trim();
  }

  // DELETAR CACHE
  WiFi.scanDelete();
  delay(500);
}

bool conectarRede(String nomeRede) {
  String ssid, senha, usuario;

  if (nomeRede.length() > 0) {
    ssid = nomeRede;
    senha = "";
    usuario = "";

    Serial.println(">>> Digite a senha para: " + nomeRede + " <<<");
    delay(1000);
    limparBuffer();

    while (!Serial.available());
    senha = Serial.readString();
    senha.trim();
  } else {
    escolherRede(ssid, senha, usuario);
  }

  redeEscolhida = ssid;
  if (usuario.length() > 0) {
    // Configuração para redes WPA2-Enterprise
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

    // Configuração do WPA2 Enterprise usando esp_eap_client.h
    esp_eap_client_set_identity((uint8_t*)usuario.c_str(), usuario.length());
    esp_eap_client_set_username((uint8_t*)usuario.c_str(), usuario.length());
    esp_eap_client_set_password((uint8_t*)senha.c_str(), senha.length());
    esp_wifi_sta_enterprise_enable();

    WiFi.begin(ssid.c_str());
  } else {
    WiFi.disconnect(true);
    // Configuração para redes comuns (WPA/WPA2)
    WiFi.begin(ssid.c_str(), senha.c_str());
  }

  Serial.println(">>> Tentando conectar à " + ssid + "... <<<");
  int tempo_conexao = 0;
  while (WiFi.status() != WL_CONNECTED && tempo_conexao < 30) {
    Serial.print(".");
    delay(500);
    tempo_conexao++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    imprimirLinhaDecorativa();
    Serial.println("\n>>> Conectado à rede: " + ssid + " <<<");
    Serial.println(">>> IP: " + WiFi.localIP().toString() + " <<<");
    imprimirLinhaDecorativa();
    return true;
  } else {
    Serial.println("\n\n>>> Falha na conexão com a rede: " + ssid + " <<<");
    return false;
  }
}

// FIM DA CONEXÃO À REDE

void setup() {
  Serial.begin(9600);
  delay(1000);

  // MANTER O FLUXO DO PROGRAMA TRAVADO ATÉ QUE A ESP SE CONECTE À ALGUMA REDE
  while (!conectarRede(redeEscolhida)) {
    while (true) {
      delay(1000);

      Serial.println(">>> Deseja tentar novamente? (1 = sim. | 2 = selecionar nova rede.) <<<");
      while (!Serial.available());

      // GARANTE QUE A ESCOLHA SERÁ UM ÚNICO CARACTER
      char input = Serial.read();
      if (!isDigit(input)) {
        Serial.println(">>> Opção inválida. Tente novamente. <<<");
        continue;
      }

      int escolha = input - '0';

      if (escolha == 1)
        break;
      else if (escolha == 2) {
        redeEscolhida = "";
        break;
      }

      Serial.println(">>> Opção inválida. Tente novamente. <<<");
    }
  }

  imprimirLinhaDecorativa();
  Serial.println(">>> Conectado com sucesso à rede: " + WiFi.SSID() + " <<<");
  imprimirLinhaDecorativa();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket iniciado!");

  // TAREFA WEBSOCKET
  xTaskCreatePinnedToCore(
      taskWebSocket,
      "TaskWebSocket",
      8192,
      NULL,
      1,
      NULL,
      app_cpu
  );

  sensors.begin();
  imprimirLinhaDecorativa();
  Serial.println(">>> Sensores iniciados e funcionando. <<<");
  imprimirLinhaDecorativa();
}

// TAREFA QUE GERENCIA O WEBSOCKET
void taskWebSocket(void *pvParameters) {
    while (true) {
        webSocket.loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// FUNÇÕES WEBSOCKET
void webSocketEvent(uint8_t client_id, WStype_t type, uint8_t *payload, size_t length) {
  Serial.print("Cliente: ");
  Serial.println(client_id);
  Serial.print("Tipo da conexão: ");
  Serial.println(type);
  Serial.print("Payload enviado: ");
  Serial.println(String((char *)payload));
  int temperaturaAmbiente = 30;
  switch (type) {
    case WStype_CONNECTED: {
      String message = "Bem-vindo ao WebSocket!";
      webSocket.sendTXT(client_id, message);
      StaticJsonDocument<200> doc;
      doc["temperatura"] = round(Input);
      doc["temperaturaAmbiente"] = temperaturaAmbiente;
      doc["potenciaBomba"] = round(((SaidaBomba - 90) / (255 - 90)) * 100);
      doc["setpoint"] = Setpoint;
      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);
      webSocket.sendTXT(client_id, jsonBuffer);
    } break;
    case WStype_DISCONNECTED:
        Serial.println("Cliente desconectado");
        break;
    case WStype_TEXT: {
      String message = String((char *)payload);
      Serial.println("Mensagem recebida: " + message);
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, message);
      if (error) {
        Serial.println("Erro ao desserializar JSON");
        return;
      }
      
      if (doc.containsKey("ping") && doc["ping"] == true) {
        Serial.println("Ping recebido. Enviando dados...");
        sendDataToClient(client_id, Input); // Responde ao ping com os dados
      }

      // Atualiza o setpoint se necessário
      if (doc.containsKey("setpoint")) {
        Serial.print("\nAntigo setpoint: ");
        Serial.println(Setpoint);
        Setpoint = doc["setpoint"];
        Serial.println("Novo setpoint: " + String(Setpoint));
      }
  } break;
  default:
    break;
  }
}

void sendDataToClient(uint8_t client_id, double temperatura) {
  StaticJsonDocument<200> doc;
  doc["temperatura"] = round(temperatura);
  doc["temperaturaAmbiente"] = 30;
  doc["potenciaBomba"] = round(((SaidaBomba - 90) / (255 - 90)) * 100);
  doc["setpoint"] = Setpoint;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Depuração: Imprime o JSON no Serial
  Serial.print("JSON enviado: ");
  serializeJson(doc, Serial);
  Serial.println();

  webSocket.sendTXT(client_id, jsonBuffer);
}

// FIM DO WEBSOCKET

// CONTROLADOR
float calcularPID(double temperatura, double Kp, double Ki, double Kd) {
  float erro = Setpoint - temperatura;
  somaErros = somaErros - bufferErro[indiceBuffer] + erro;
  bufferErro[indiceBuffer] = erro;
  indiceBuffer = (indiceBuffer + 1) % TAMANHO_JANELA;
  float mediaMovelErro = somaErros / TAMANHO_JANELA;

  static float erroAnterior = 0.0;
  static float integral = 0.0;
  integral += mediaMovelErro;
  float derivativo = mediaMovelErro - erroAnterior;
  erroAnterior = mediaMovelErro;
  float saidaPID = Kp * mediaMovelErro + Ki * integral + Kd * derivativo;

  return saidaPID;
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));

    sensors.requestTemperatures();
    Input = sensors.getTempCByIndex(0);

    double erro = Setpoint-Input;
    if (abs(erro) < 4) {
      Output = calcularPID(Input, Kp_agress, Ki_agress, Kd_agress);
    }
    else {
      Output = calcularPID(Input, Kp_suav, Ki_suav, Kd_suav);
    }

    if (erro < 0) {
      SaidaBomba = constrain((255 - Output), 95, 255);
      analogWrite(pinoBomba, SaidaBomba);
    }

    else {
      SaidaBomba = constrain(Output, 100, 255);
      analogWrite(pinoBomba, SaidaBomba);
    }
    Serial.print("Setpoint: ");
    Serial.print(Setpoint);
    Serial.print(" | Erro: ");
    Serial.print(erro);
    Serial.print(" | Temperatura: ");
    Serial.print(Input);
    Serial.print(" | Saída: ");
    Serial.println(SaidaBomba);

    delay(100);
}
