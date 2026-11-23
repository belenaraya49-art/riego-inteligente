#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <math.h>

// ======================================
// 1. WIFI - CAMBIA ESTOS DATOS
// ======================================

const char* ssid = "NOMBRE_DE_TU_WIFI";
const char* password = "CONTRASENA_DE_TU_WIFI";

// ======================================
// 2. PINES
// ======================================

const int PIN_HUMEDAD_SUELO = 34;
const int PIN_TEMPERATURA_SUELO = 4;
const int PIN_DHT11 = 18;
const int PIN_PIR = 27;
const int PIN_RELE = 26;

#define TIPO_DHT DHT11

// ======================================
// 3. CONFIGURACION
// ======================================

const int VALOR_SECO = 3000;
const int VALOR_MOJADO = 2140;

const int UMBRAL_ENCENDER = 35;
const int UMBRAL_APAGAR = 55;

const unsigned long INTERVALO_LECTURA = 30000UL;
const unsigned long TIEMPO_PIR = 60000UL;
const unsigned long ESPERA_MOVIMIENTO = 10000UL;

// ======================================
// 4. OBJETOS
// ======================================

OneWire oneWire(PIN_TEMPERATURA_SUELO);
DallasTemperature sensorSuelo(&oneWire);

DHT sensorAmbiente(PIN_DHT11, TIPO_DHT);

WebServer servidor(80);

// ======================================
// 5. VARIABLES
// ======================================

int humedadSuelo = 0;
int lecturaADC = 0;

float temperaturaSuelo = NAN;
float temperaturaAmbiente = NAN;
float humedadAmbiente = NAN;

int sensoresDS18B20 = 0;

bool dhtCorrecto = false;
bool movimiento = false;
bool bloqueoPIR = true;
bool releEncendido = false;

bool riegoAutomatico = false;
bool riegoManual = false;

String modo = "demo";

unsigned long ultimaLectura = 0;
unsigned long inicioPIR = 0;
unsigned long ultimoMovimiento = 0;

bool movimientoRegistrado = false;

// ======================================
// 6. CONTROL DEL RELE
// ======================================

void controlarRele(bool encender) {

  releEncendido = encender;

  // Rele activo en LOW
  digitalWrite(
    PIN_RELE,
    encender ? LOW : HIGH
  );
}

// ======================================
// 7. MOVIMIENTO
// ======================================

void revisarPIR() {

  unsigned long ahora = millis();

  movimiento =
    digitalRead(PIN_PIR) == HIGH;

  if (movimiento) {
    ultimoMovimiento = ahora;
    movimientoRegistrado = true;
  }

  bool calentamiento =
    ahora - inicioPIR < TIEMPO_PIR;

  bool espera =
    movimientoRegistrado &&
    ahora - ultimoMovimiento < ESPERA_MOVIMIENTO;

  bloqueoPIR =
    calentamiento || movimiento || espera;
}

// ======================================
// 8. CONTROL AUTOMATICO
// ======================================

void actualizarRiego() {

  bool encender = false;

  if (modo == "automatico") {
    encender = riegoAutomatico;
  }

  if (modo == "manual") {
    encender = riegoManual;
  }

  // Modo demo: rele siempre apagado
  if (modo == "demo") {
    encender = false;
  }

  // Proteccion por movimiento
  if (bloqueoPIR) {
    encender = false;
  }

  // No activar con sensor de suelo invalido
  if (lecturaADC < 100 ||
      lecturaADC > 4000) {
    encender = false;
  }

  controlarRele(encender);
}

// ======================================
// 9. LECTURA DE SENSORES
// ======================================

const char* ARCHIVO_HISTORIAL = "/historial.csv";
bool historialDisponible = false;

void guardarLectura() {
  if (!historialDisponible) {
    return;
  }

  File archivo = LittleFS.open(ARCHIVO_HISTORIAL, "a");
  if (!archivo) {
    Serial.println("No se pudo abrir el historial");
    return;
  }

  if (archivo.size() == 0) {
    archivo.println(
      "segundos_desde_encendido,humedad_suelo_pct,adc,"
      "temperatura_suelo_c,temperatura_ambiente_c,humedad_ambiente_pct"
    );
  }

  String fila = String(millis() / 1000UL);
  fila += ',';
  fila += String(humedadSuelo);
  fila += ',';
  fila += String(lecturaADC);
  fila += ',';
  if (!isnan(temperaturaSuelo)) {
    fila += String(temperaturaSuelo, 1);
  }
  fila += ',';
  if (!isnan(temperaturaAmbiente)) {
    fila += String(temperaturaAmbiente, 1);
  }
  fila += ',';
  if (!isnan(humedadAmbiente)) {
    fila += String(humedadAmbiente, 1);
  }
  fila += '\n';

  if (archivo.print(fila) != fila.length()) {
    Serial.println("Escritura incompleta en el historial");
  }
  archivo.close();
}

void leerSensores() {

  Serial.println();
  Serial.println("==========================");
  Serial.println("DIAGNOSTICO DE SENSORES");
  Serial.println("==========================");

  // HUMEDAD DEL SUELO

  long suma = 0;

  for (int i = 0; i < 10; i++) {
    suma += analogRead(PIN_HUMEDAD_SUELO);
    delay(3);
  }

  lecturaADC = suma / 10;

  humedadSuelo = constrain(
    map(
      lecturaADC,
      VALOR_SECO,
      VALOR_MOJADO,
      0,
      100
    ),
    0,
    100
  );

  Serial.print("Humedad del suelo: ");
  Serial.print(humedadSuelo);
  Serial.println("%");

  Serial.print("ADC: ");
  Serial.println(lecturaADC);

  // DS18B20

  sensoresDS18B20 =
    sensorSuelo.getDeviceCount();

  Serial.print(
    "Sensores DS18B20 encontrados: "
  );
  Serial.println(sensoresDS18B20);

  sensorSuelo.requestTemperatures();

  float lecturaTemperatura =
    sensorSuelo.getTempCByIndex(0);

  if (lecturaTemperatura ==
      DEVICE_DISCONNECTED_C ||
      lecturaTemperatura < -55 ||
      lecturaTemperatura > 125) {

    temperaturaSuelo = NAN;

    Serial.println(
      "ERROR: DS18B20 sin lectura"
    );

  } else {

    temperaturaSuelo =
      lecturaTemperatura;

    Serial.print(
      "Temperatura del suelo: "
    );

    Serial.print(temperaturaSuelo);
    Serial.println(" C");
  }

  // DHT11

  float nuevaTemperatura =
    sensorAmbiente.readTemperature();

  float nuevaHumedad =
    sensorAmbiente.readHumidity();

  dhtCorrecto =
    !isnan(nuevaTemperatura) &&
    !isnan(nuevaHumedad);

  if (dhtCorrecto) {

    temperaturaAmbiente =
      nuevaTemperatura;

    humedadAmbiente =
      nuevaHumedad;

    Serial.print(
      "Temperatura ambiental: "
    );

    Serial.print(temperaturaAmbiente);
    Serial.println(" C");

    Serial.print(
      "Humedad ambiental: "
    );

    Serial.print(humedadAmbiente);
    Serial.println("%");

  } else {

    temperaturaAmbiente = NAN;
    humedadAmbiente = NAN;

    Serial.println(
      "ERROR: DHT11 sin lectura"
    );
  }

  // DECISION AUTOMATICA

  if (lecturaADC < 100 ||
      lecturaADC > 4000) {

    riegoAutomatico = false;

  } else if (
    humedadSuelo < UMBRAL_ENCENDER
  ) {

    riegoAutomatico = true;

  } else if (
    humedadSuelo > UMBRAL_APAGAR
  ) {

    riegoAutomatico = false;
  }

  ultimaLectura = millis();
  guardarLectura();

  Serial.println("==========================");
}

// ======================================
// 10. FORMATO JSON
// ======================================

String numeroJSON(float valor) {

  if (isnan(valor)) {
    return "null";
  }

  return String(valor, 1);
}

// ======================================
// 11. DATOS PARA LA WEB
// ======================================

void enviarDatos() {

  String datos = "{";

  datos += "\"suelo\":" +
    String(humedadSuelo) + ",";

  datos += "\"adc\":" +
    String(lecturaADC) + ",";

  datos += "\"temperaturaSuelo\":" +
    numeroJSON(temperaturaSuelo) + ",";

  datos += "\"temperaturaAmbiente\":" +
    numeroJSON(temperaturaAmbiente) + ",";

  datos += "\"humedadAmbiente\":" +
    numeroJSON(humedadAmbiente) + ",";

  datos += "\"dsEncontrados\":" +
    String(sensoresDS18B20) + ",";

  datos += "\"dhtCorrecto\":" +
    String(dhtCorrecto ? "true" : "false") + ",";

  datos += "\"movimiento\":" +
    String(movimiento ? "true" : "false") + ",";

  datos += "\"bloqueo\":" +
    String(bloqueoPIR ? "true" : "false") + ",";

  datos += "\"rele\":" +
    String(releEncendido ? "true" : "false") + ",";

  datos += "\"modo\":\"" + modo + "\"";

  datos += "}";

  servidor.send(
    200,
    "application/json",
    datos
  );
}

// ======================================
// 12. PAGINA WEB
// ======================================

const char PAGINA[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es">

<head>
<meta charset="UTF-8">
<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>Riego Inteligente</title>

<style>

body {
  font-family: Arial, sans-serif;
  background: #eef5f1;
  color: #234337;
  text-align: center;
  padding: 15px;
}

h1 {
  margin-bottom: 5px;
}

.card {
  background: white;
  padding: 20px;
  margin: 14px auto;
  border-radius: 15px;
  max-width: 500px;
  box-shadow: 0 3px 10px #0001;
}

.valor {
  font-size: 27px;
  font-weight: bold;
}

button {
  padding: 12px;
  margin: 5px;
  border: none;
  border-radius: 8px;
  background: #d7e9de;
  cursor: pointer;
}

button:disabled {
  opacity: 0.45;
}

.verde {
  background: #2f9369;
  color: white;
}

.error {
  color: #b34436;
}

.aviso {
  color: #b56b22;
}

</style>
</head>

<body>

<h1>🌱 Riego Inteligente</h1>
<p>Monitoreo y control ESP32</p>

<div class="card">
<h3>🌱 Humedad del suelo</h3>
<div class="valor" id="suelo">--</div>
<p id="adc"></p>
</div>

<div class="card">
<h3>🌡️ Temperatura del suelo</h3>
<div class="valor" id="ts">--</div>
<p id="ds"></p>
</div>

<div class="card">
<h3>☀️ Ambiente</h3>
<p>Temperatura:
<b id="ta">--</b></p>
<p>Humedad:
<b id="ha">--</b></p>
<p id="dht"></p>
</div>

<div class="card">
<h3>🚶 Movimiento</h3>
<p id="mov">--</p>
<p class="aviso" id="bloqueo"></p>
</div>

<div class="card">
<h3>⚙️ Control de riego</h3>

<p>Modo: <b id="modo">--</b></p>

<button onclick="accion('/modo?valor=automatico')">
Automático
</button>

<button onclick="accion('/modo?valor=manual')">
Manual
</button>

<button onclick="accion('/modo?valor=demo')">
Demostración
</button>

<p>Relé: <b id="rele">--</b></p>

<button class="verde" id="on"
onclick="accion('/rele?valor=on')">
Encender
</button>

<button id="off"
onclick="accion('/rele?valor=off')">
Apagar
</button>

<p>Modo demostración: relé apagado.</p>

</div>

<div class="card">
<h3>🔍 Diagnóstico</h3>

<button onclick="accion('/leer')">
Medir ahora
</button>

<p>Lecturas automáticas cada 30 segundos.</p>
<p><a href="/historial.csv" download>Descargar historial CSV</a></p>

</div>

<script>

function formato(valor, unidad) {
  return valor === null ?
    "Sin lectura" :
    valor + unidad;
}

async function actualizar() {

  try {

    const respuesta = await fetch('/datos');
    const d = await respuesta.json();

    document.getElementById('suelo').textContent =
      d.suelo + "%";

    document.getElementById('adc').textContent =
      "ADC: " + d.adc;

    document.getElementById('ts').textContent =
      formato(d.temperaturaSuelo, " °C");

    document.getElementById('ta').textContent =
      formato(d.temperaturaAmbiente, " °C");

    document.getElementById('ha').textContent =
      formato(d.humedadAmbiente, "%");

    document.getElementById('ds').textContent =
      "DS18B20 encontrados: " + d.dsEncontrados;

    document.getElementById('dht').textContent =
      d.dhtCorrecto ?
      "DHT11 funcionando" :
      "DHT11 sin respuesta";

    document.getElementById('mov').textContent =
      d.movimiento ?
      "Movimiento detectado" :
      "Sin movimiento actual";

    document.getElementById('bloqueo').textContent =
      d.bloqueo ?
      "Protección PIR activa" : "";

    document.getElementById('modo').textContent =
      d.modo;

    document.getElementById('rele').textContent =
      d.rele ? "ENCENDIDO" : "APAGADO";

    document.getElementById('on').disabled =
      d.modo !== "manual";

    document.getElementById('off').disabled =
      d.modo !== "manual";

  } catch (e) {
    console.log(e);
  }
}

async function accion(ruta) {
  await fetch(ruta);
  actualizar();
}

setInterval(actualizar, 1000);
actualizar();

</script>

</body>
</html>
)HTML";

// ======================================
// 13. SETUP
// ======================================

void setup() {

  Serial.begin(115200);

  digitalWrite(PIN_RELE, HIGH);
  pinMode(PIN_RELE, OUTPUT);

  pinMode(PIN_PIR, INPUT);

  controlarRele(false);

  analogReadResolution(12);

  sensorSuelo.begin();
  sensorSuelo.setResolution(10);

  sensorAmbiente.begin();

  Serial.println();
  Serial.println("INICIANDO RIEGO INTELIGENTE");

  sensoresDS18B20 =
    sensorSuelo.getDeviceCount();

  Serial.print(
    "Sensores DS18B20 encontrados: "
  );
  Serial.println(sensoresDS18B20);

  inicioPIR = millis();

  revisarPIR();

  // Dar tiempo al DHT11 para iniciar
  delay(2500);

  // Conserva el historial existente si falla el montaje.
  historialDisponible = LittleFS.begin(false);
  if (!historialDisponible) {
    Serial.println("LittleFS no disponible");
  }

  leerSensores();
  actualizarRiego();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  Serial.print("Abre la pagina: http://");
  Serial.println(WiFi.localIP());

  servidor.on("/", []() {
    servidor.send_P(
      200,
      "text/html; charset=utf-8",
      PAGINA
    );
  });

  servidor.on("/datos", enviarDatos);

  servidor.on("/historial.csv", []() {
    if (!historialDisponible) {
      servidor.send(503, "text/plain", "LittleFS no disponible");
      return;
    }

    File archivo = LittleFS.open(ARCHIVO_HISTORIAL, "r");
    if (!archivo) {
      servidor.send(404, "text/plain", "Todavia no hay historial");
      return;
    }

    servidor.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"historial.csv\""
    );
    servidor.streamFile(archivo, "text/csv");
    archivo.close();
  });

  servidor.on("/leer", []() {

    leerSensores();
    revisarPIR();
    actualizarRiego();

    servidor.send(
      200,
      "text/plain",
      "OK"
    );
  });

  servidor.on("/modo", []() {

    String nuevo = servidor.arg("valor");

    if (
      nuevo == "demo" ||
      nuevo == "manual" ||
      nuevo == "automatico"
    ) {

      modo = nuevo;
      riegoManual = false;

      revisarPIR();
      actualizarRiego();
    }

    servidor.send(
      200,
      "text/plain",
      "OK"
    );
  });

  servidor.on("/rele", []() {

    if (modo == "manual") {

      riegoManual =
        servidor.arg("valor") == "on";
    }

    revisarPIR();
    actualizarRiego();

    servidor.send(
      200,
      "text/plain",
      "OK"
    );
  });

  servidor.begin();

  Serial.println("Servidor web iniciado");
}

// ======================================
// 14. LOOP
// ======================================

void loop() {

  servidor.handleClient();

  revisarPIR();

  if (
    millis() - ultimaLectura >=
    INTERVALO_LECTURA
  ) {

    leerSensores();
  }

  actualizarRiego();

  static bool anteriorMovimiento = false;
  static bool anteriorRele = false;

  if (
    movimiento != anteriorMovimiento ||
    releEncendido != anteriorRele
  ) {

    Serial.print("PIR: ");

    Serial.print(
      movimiento ?
      "MOVIMIENTO" :
      "SIN MOVIMIENTO"
    );

    Serial.print(" | RELE: ");

    Serial.println(
      releEncendido ?
      "ENCENDIDO" :
      "APAGADO"
    );

    anteriorMovimiento = movimiento;
    anteriorRele = releEncendido;
  }

  delay(30);
}
