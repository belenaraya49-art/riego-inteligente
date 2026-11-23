# Riego Inteligente

Prototipo IoT para monitorear la humedad del suelo y apoyar la decisión
de cuándo regar áreas verdes. El ESP32 recibe datos de los sensores y
los muestra en una página web que se abre desde la misma red Wi-Fi.

## Integrantes

- Belén Araya 
- Martín Cardenas 

## Cómo funciona

- Un sensor capacitivo mide la humedad del suelo.
- El DS18B20 mide la temperatura del suelo y el DHT11 mide la temperatura
  y humedad del ambiente.
- El ESP32 toma una lectura cada 30 segundos. También se puede solicitar
  una lectura desde el botón «Medir ahora» del dashboard.
- En modo automático, el código solicita encender el relé cuando la humedad
  baja del 35 % y apagarlo cuando supera el 55 %. El sensor PIR bloquea
  la activación si detecta movimiento.
- El código incluye un historial CSV descargable desde la sección Diagnóstico.

## Cómo probarlo

1. Abre `RiegoInteligente.ino` en Arduino IDE.
2. Instala el soporte para ESP32 y las librerías OneWire,
   DallasTemperature y DHT.
3. Configura el nombre y la contraseña del Wi-Fi en tu copia local
   del archivo y carga el programa al ESP32.
4. Abre el Monitor serie a 115200 baudios y copia la dirección IP
   que imprime la placa.
5. Abre esa dirección en un navegador conectado a la misma red.
6. Para probar el historial, pulsa «Medir ahora» y descarga el CSV
   desde Diagnóstico.

## Estado de las pruebas

Se comprobaron las lecturas en el dashboard y la conmutación del relé.
Queda pendiente verificar en la placa que el historial CSV se conserve
después de un reinicio y comprobar el bombeo de agua.

## Fuentes

Las librerías y el apoyo de inteligencia artificial se documentan en
[FUENTES.md](FUENTES.md).
