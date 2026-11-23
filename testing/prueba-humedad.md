# Prueba de humedad del suelo y registro de datos

## Objetivo

Comprobar que el prototipo distingue entre tierra seca y húmeda y que
las mediciones siguen disponibles después de reiniciar el ESP32.

## Materiales

ESP32 con el firmware del proyecto, sensor capacitivo de humedad,
dos muestras de tierra (una seca y una húmeda) y un dispositivo
con acceso al dashboard.

## Procedimiento

1. Abrir el dashboard y dejar el sistema en modo Demostración.
2. Insertar el sensor en tierra seca y pulsar «Medir ahora».
   Registrar el ADC y el porcentaje que muestra el dashboard.
3. Repetir con tierra húmeda, insertando el sensor a la misma profundidad.
4. Descargar el historial CSV desde la sección Diagnóstico.
5. Reiniciar el ESP32, volver a abrir el dashboard y descargar el CSV.
6. Comprobar que las dos mediciones anteriores siguen en el archivo.

## Criterios de éxito

- El porcentaje mostrado para la tierra húmeda es mayor que el de la seca.
- Las dos mediciones siguen disponibles en el CSV tras reiniciar.

## Resultados

| Condición | Lectura ADC | Humedad mostrada (%) |
| --- | --- | --- |
| Tierra seca | Pendiente | Pendiente |
| Tierra húmeda | Pendiente | Pendiente |

- Persistencia del historial después del reinicio: pendiente.
- Evidencia: se adjuntará una captura del dashboard y el CSV de la prueba.

## Relación con el proyecto

Distinguir entre suelo seco y húmedo permite evaluar cuándo hace falta
regar. El historial permite revisar las mediciones que respaldan esa decisión.
