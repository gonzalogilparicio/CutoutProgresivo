# Proyecto Control de Válvula Cutout Automotriz

## Descripción
Este proyecto consiste en un sistema electrónico basado en Arduino Uno R3 para controlar una válvula cutout de escape en un automóvil (en este caso instalado en un Golf GTI 2018). El sistema permite ajustar la apertura de la válvula proporcionalmente mediante un potenciómetro, recuerda la última posición al apagar el vehículo (usando memoria EEPROM), y cuenta con un botón para cerrar la válvula forzosamente. Está diseñado para integrarse al sistema eléctrico del auto, usando un positivo de contacto para encendido/apagado.

## Características
- Control proporcional de la válvula cutout con un potenciómetro (0% cerrado, 100% abierto).
- Memoria EEPROM para guardar la última posición y valores calibrados.
- Botón para cerrar la válvula forzosamente.
- Diseño robusto para vibraciones automotrices con fijación en gabinete plástico.
- Compatible con alimentación de batería de auto regulada a 5V con step-down.

## Hardware Utilizado
### Componentes Principales
- **Arduino Uno R3**: Microcontrolador central para procesar entradas y controlar el motor.
- **XL4015**: Regulador de voltaje (ajustado a 5V) para alimentar el Arduino desde la batería del auto.
- **BTS7960**: Puente H para controlar el motor DC de la válvula con PWM.
- **Potenciómetro Lineal 10kΩ**: Sensor de entrada para ajustar la posición de la válvula.
- **Botón Momentáneo**: Interruptor para activar el cierre forzado.
- **Tecla Encendido**: Interruptor manual para encender/apagar el sistema.
- **Fusible 5A con Portafusible**: Protección contra sobrecorriente desde la batería.
- **Cables**: AWG 18-20 para alimentación (B+ y B- del BTS7960), AWG 22-24 para señales.

## Conexiones
### Arduino Uno R3
- **5V**: Conectado al VOUT del XL4015 (5V).
- **GND**: Conectado al GND del XL4015 y B- del BTS7960 (tierra común).
- **A0**: Pin central del potenciómetro (izquierda a 5V, derecha a GND).
- **D9 (RPWM)**: Conectado a RPWM del BTS7960 (apertura).
- **D10 (LPWM)**: Conectado a LPWM del BTS7960 (cierre).
- **D2**: Conectado a un lado del botón (otro lado a GND, pull-up interno).

### XL4015 Step-Down
- **VIN+**: Conectado al positivo de la batería a través de un fusible de 5A y la tecla encendido.
- **VIN-**: Conectado al negativo de la batería del auto.
- **VOUT+**: Ajustado a 5V con un multímetro, conectado al pin 5V del Arduino.
- **VOUT-**: Conectado al GND del Arduino.

### BTS7960
- **B+**: Conectado al positivo de la batería vía fusible 5A y tecla encendido.
- **B-**: Conectado al negativo de la batería.
- **M+**: Conectado al cable positivo del motor de la válvula cutout.
- **M-**: Conectado al cable negativo del motor de la válvula cutout.
- **RPWM**: Conectado al pin D9 del Arduino.
- **LPWM**: Conectado al pin D10 del Arduino.
- **R_EN y L_EN**: Ambos conectados al pin 5V del Arduino.
- **R_IS y L_IS**: No conectados.

### Botón
- Un lado a D2 del Arduino, el otro a GND.

### Potenciómetro
- Pin izquierdo a 5V del Arduino.
- Pin derecho a GND del Arduino.
- Pin central a A0 del Arduino.

## Instrucciones de Uso
1. **Calibración Inicial**:
   - Conecta el Arduino por USB a una PC y abre el Monitor Serial (115200 baudios).
   - Gira el potenciómetro a la izquierda hasta que la válvula esté cerrada, presiona Enter.
   - Gira a la derecha hasta que esté abierta, presiona Enter.
   - La calibración se guarda en EEPROM.
2. **Operación Normal**:
   - Enciende el sistema con la tecla.
   - Ajusta la válvula girando el potenciómetro (0-100%).
   - La posición se recuerda al apagar.
3. **Cierre Forzado**:
   - Presiona el botón; la válvula se cerrará por 2 segundos (12V) e ignorará el potenciómetro por 5 segundos.

## Notas Adicionales
- **Depuración**: Usa el Monitor Serial para verificar `targetPosition` y `currentPosition`.
- **Ajustes**: Si la válvula no se mueve a bajos %, aumenta el PWM mínimo en `map()` (ej. de 100 a 150).

## Licencia
Este proyecto se comparte bajo licencia MIT.

## Contribuciones
Sugerencias o mejoras son bienvenidas. Abrí un issue o enviá un pull request.