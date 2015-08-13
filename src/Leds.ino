//Handles flashing LEDs

void initLeds()
{
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  
  pinMode(GRN_LED_PIN, OUTPUT);
  digitalWrite(GRN_LED_PIN, LOW);
  
}

//----------------------------------------------------------------------
void serviceLeds()
{
  if (!ledCounter)
  {
    ledCounter = 4 * LED_DELAY;
    ledState = ledMode;
  }
  ledCounter--;

  digitalWrite(RED_LED_PIN, (ledState & 2) ? 1 : 0);
  digitalWrite(GRN_LED_PIN, (ledState & 1) ? 1 : 0);
    
  if (ledCounter % LED_DELAY == 0)
    ledState = ledState >> 2;
  
}

