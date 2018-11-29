#ifdef DEBUG
  #define DEBUG_PRINT(x)            Serial.print(x)
  #define DEBUG_PRINTLN(x)          Serial.println(x)
  #define DEBUG_PRINTF(x, args...)  Serial.printf(x, args)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, args...)
#endif
