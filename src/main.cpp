#include <Arduino.h>
#include <EEPROM.h>  // Incluye la biblioteca EEPROM para almacenar datos persistentes

// Prototipos de funciones para que el compilador las reconozca antes de usarlas
void calibrateSystem();
void checkForEEPROMClear();
void forceCloseValve();

// Definición de constantes para pines y parámetros
const int potPin = A0;    // Pin analógico A0 para leer el valor del potenciómetro
const int RPWM = 9;       // Pin 9 como salida PWM para controlar la apertura (RPWM del BTS7960)
const int LPWM = 10;      // Pin 10 como salida PWM para controlar el cierre (LPWM del BTS7960)
const int buttonPin = 2;  // Pin 2 como entrada para el botón con función de cerrar válvula
int potValue = 0;         // Variable para almacenar el valor crudo del potenciómetro (0-1023)
int lastPotValue = 0;     // Variable para almacenar el último valor leído del potenciómetro
int targetPosition = 0;   // Variable para la posición objetivo de la válvula (0-100%)
int currentPosition = 0;  // Variable para la posición actual estimada de la válvula
int minPotValue = 0;      // Valor mínimo del potenciómetro calibrado (cerrado, 0%)
int maxPotValue = 1023;   // Valor máximo del potenciómetro calibrado (abierto, 100%)
const int rampStep = 5;   // Incremento gradual para mover la válvula suavemente
unsigned long lastMoveTime = 0; // Tiempo del último movimiento del potenciómetro
const unsigned long idleThreshold = 500; // Tiempo de inactividad (ms) para apagar el motor
bool ignorePot = false;   // Bandera para indicar si se debe ignorar el potenciómetro
unsigned long ignoreStartTime = 0; // Tiempo de inicio cuando se ignora el potenciómetro

// Direcciones en la EEPROM para almacenar datos
#define POS_ADDR 0     // Dirección para guardar currentPosition
#define MIN_ADDR 4     // Dirección para guardar minPotValue
#define MAX_ADDR 8     // Dirección para guardar maxPotValue

// Función de configuración inicial
void setup() {
  Serial.begin(115200);  // Inicia la comunicación serial a 115200 baudios para depuración
  pinMode(RPWM, OUTPUT); // Configura el pin RPWM como salida
  pinMode(LPWM, OUTPUT); // Configura el pin LPWM como salida
  pinMode(buttonPin, INPUT_PULLUP); // Configura el pin del botón como entrada con pull-up interno
  digitalWrite(RPWM, LOW); // Inicializa RPWM en bajo (motor apagado)
  digitalWrite(LPWM, LOW); // Inicializa LPWM en bajo (motor apagado)

  checkForEEPROMClear();  // Verifica si hay un comando para borrar la EEPROM antes de leerla

  // Lee los valores guardados en la EEPROM
  EEPROM.get(POS_ADDR, currentPosition);  // Recupera la última posición guardada
  EEPROM.get(MIN_ADDR, minPotValue);      // Recupera el valor mínimo calibrado
  EEPROM.get(MAX_ADDR, maxPotValue);      // Recupera el valor máximo calibrado

  // Verifica si la calibración es válida, si no, inicia el proceso
  if (minPotValue < 0 || maxPotValue > 1023 || minPotValue >= maxPotValue) {
    calibrateSystem();  // Llama a la función de calibración si los valores son inválidos
  }
}

// Función para calibrar los valores mínimo y máximo del potenciómetro
void calibrateSystem() {
  Serial.println("Iniciando calibración...");  // Mensaje de inicio en el Monitor Serial
  Serial.println("Girar el potenciómetro a la izquierda (cerrado). Presionar Enter.");  // Instrucción al usuario
  while (!Serial.available()) delay(100);  // Espera hasta que se presione Enter
  Serial.readString();  // Limpia el buffer del Serial
  minPotValue = analogRead(potPin);  // Lee el valor del potenciómetro en posición cerrada
  Serial.print("Valor mínimo: "); Serial.println(minPotValue);  // Muestra el valor leído

  Serial.println("Girar el potenciómetro a la derecha (abierto). Presionar Enter.");  // Nueva instrucción
  while (!Serial.available()) delay(100);  // Espera otro Enter
  Serial.readString();  // Limpia el buffer nuevamente
  maxPotValue = analogRead(potPin);  // Lee el valor en posición abierta
  Serial.print("Valor máximo: "); Serial.println(maxPotValue);  // Muestra el valor

  // Guarda los valores calibrados en la EEPROM
  EEPROM.put(MIN_ADDR, minPotValue);  // Almacena el mínimo
  EEPROM.put(MAX_ADDR, maxPotValue);  // Almacena el máximo
  EEPROM.put(POS_ADDR, 0);  // Resetea la posición a 0
  Serial.println("Calibración completada.");  // Confirma el fin de la calibración
}

// Bucle principal que controla el sistema
void loop() {
  // Detecta si se presiona el botón
  if (digitalRead(buttonPin) == LOW) {  // LOW indica que el botón está presionado (pull-up)
    delay(50);  // Retardo para debounce y evitar falsos positivos
    if (digitalRead(buttonPin) == LOW) {  // Confirma que sigue presionado
      forceCloseValve();  // Ejecuta la función para cerrar la válvula
      while (digitalRead(buttonPin) == LOW) delay(10);  // Espera a que se suelte el botón
    }
  }

  // Controla la lectura del potenciómetro según el estado de ignorePot
  if (!ignorePot || (millis() - ignoreStartTime >= 5000)) {  // Si no ignora o pasaron 5 segundos
    ignorePot = false;  // Reanuda la lectura del potenciómetro
    potValue = analogRead(potPin);  // Lee el valor analógico del potenciómetro
    targetPosition = map(potValue, minPotValue, maxPotValue, 0, 100);  // Mapea a 0-100%
    if (targetPosition > 100) targetPosition = 100;  // Limita targetPosition a 100% si se excede

    // Actualiza currentPosition solo si la diferencia con targetPosition es significativa o está cerca de un extremo
    if (abs(targetPosition - currentPosition) > 2 || (targetPosition == 0 && currentPosition > 0) || (targetPosition == 100 && currentPosition < 100)) {
      lastMoveTime = millis();  // Actualiza el tiempo del último movimiento

      if (targetPosition > currentPosition) {  // Si debe abrirse más
        int pwmValue = map(targetPosition - currentPosition, 0, 100, 100, 255);  // PWM de 100 a 255
        analogWrite(RPWM, pwmValue);  // Envía señal PWM para abrir
        analogWrite(LPWM, 0);  // Apaga el cierre
        currentPosition += rampStep;  // Incrementa la posición actual
        if (currentPosition > targetPosition) currentPosition = targetPosition;  // Limita al objetivo
      } else if (targetPosition < currentPosition) {  // Si debe cerrarse más
        int pwmValue = map(currentPosition - targetPosition, 0, 100, 100, 255);  // PWM de 100 a 255
        analogWrite(RPWM, 0);  // Apaga la apertura
        analogWrite(LPWM, pwmValue);  // Envía señal PWM para cerrar
        currentPosition -= rampStep;  // Decrementa la posición actual
        if (currentPosition < targetPosition) currentPosition = targetPosition;  // Limita al objetivo
      }

      // Forzar límites exactos en 0% y 100% después del ajuste
      if (targetPosition == 0 && currentPosition > 0) currentPosition = 0;  // Fuerza 0% si es el objetivo
      if (targetPosition == 100 && currentPosition < 100) currentPosition = 100;  // Fuerza 100% si es el objetivo

      EEPROM.put(POS_ADDR, currentPosition);  // Guarda la nueva posición en EEPROM
    }
  }

  // Apaga el motor si no hay actividad por 500 ms
  if (millis() - lastMoveTime > idleThreshold) {
    analogWrite(RPWM, 0);  // Detiene la apertura
    analogWrite(LPWM, 0);  // Detiene el cierre
  }

  lastPotValue = potValue;  // Actualiza el último valor leído

  // Muestra datos en el Monitor Serial para depuración
  Serial.print("Pot: "); Serial.print(targetPosition);  // Muestra la posición objetivo
  Serial.print("% - Posición actual: "); Serial.println(currentPosition);  // Muestra la posición actual
  delay(50);  // Pequeño retardo para estabilidad
}

// Función para verificar si se recibe el comando "borrar-eeprom" por el Monitor Serial
void checkForEEPROMClear() {
  Serial.println("Escribe 'BE' (Borrar EEPROM) para limpiar la memoria EEPROM (10 segundos restantes para escribir)...");  // Instrucción al usuario
  unsigned long startTime = millis();  // Registra el tiempo de inicio
  String input = "";  // Variable para almacenar el texto recibido

  while (millis() - startTime < 10000) {  // Espera 10 segundos para recibir un comando
    if (Serial.available() > 0) {  // Si hay datos disponibles en el puerto Serial
      char c = Serial.read();  // Lee un carácter
      if (c == '\n' || c == '\r') {  // Si es un salto de línea o retorno de carro
        input.trim();  // Elimina espacios o caracteres extra
        if (input == "BE") {  // Compara el texto recibido con el comando
          Serial.println("Borrando EEPROM...");  // Confirma la acción
          for (unsigned int i = 0; i < EEPROM.length(); i++) {  // Recorre toda la memoria EEPROM (usamos unsigned int para evitar la advertencia)
            EEPROM.write(i, 0);  // Escribe 0 en cada posición
          }
          Serial.println("EEPROM borrada. Reinicia el Arduino para calibrar de nuevo.");  // Mensaje de confirmación
          while (true);  // Detiene el programa hasta que reinicies
        }
        input = "";  // Resetea el buffer de entrada
      } else {
        input += c;  // Agrega el carácter al buffer
      }
    }
    delay(10);  // Pequeño retardo para no saturar el procesador
  }
  Serial.println("Tiempo de espera terminado. Continuando...");  // Mensaje si no se recibe el comando
}

// Función para cerrar la válvula forzosamente
void forceCloseValve() {
  ignorePot = true;         // Activa la bandera para ignorar el potenciómetro
  ignoreStartTime = millis(); // Registra el tiempo de inicio de la ignorancia
  analogWrite(LPWM, 255);   // Aplica 12V completo (PWM 255) para cerrar
  delay(2000);              // Mantiene el cierre por 2 segundos
  analogWrite(LPWM, 0);     // Apaga el motor después de 2 segundos
  currentPosition = 0;      // Actualiza la posición a 0 (cerrado)
  EEPROM.put(POS_ADDR, currentPosition);  // Guarda la posición en EEPROM
  // Ignora el potenciómetro por 5 segundos desde ignoreStartTime (controlado en loop)
}