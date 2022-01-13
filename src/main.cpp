#include <Arduino.h>

#define AOE 11

#define MWE 10
#define MOE 9
#define MCS 8

#define Q7 7
#define DOE 6
#define S0 5
#define S1 4
#define CP 3
#define DSR 2

void writeToROM( uint8_t data, uint16_t address );
uint8_t readFromROM(uint16_t address);

void writeAddressAndDataToRegisters( uint8_t data, uint16_t address);

void writeTo299(uint8_t data);
uint8_t readFrom299();

uint8_t shiftIn299(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
void sClock();
void latch();
void hold();


void setup() {
  Serial.begin(115200);
  
  pinMode(LED_BUILTIN, OUTPUT);

  // Get these HIGH ASAP to avoid any accidental single byte write
  pinMode(MCS, OUTPUT);
  digitalWrite(MCS,HIGH);

  pinMode(AOE, OUTPUT);
  digitalWrite(AOE, HIGH);

  pinMode(MWE, OUTPUT);
  digitalWrite(MWE, HIGH);

  pinMode(MOE, OUTPUT);
  digitalWrite(MOE, HIGH);

  pinMode(DOE, OUTPUT);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(CP, OUTPUT);
  pinMode(DSR, OUTPUT);

  pinMode(Q7, INPUT);

  digitalWrite(CP, LOW);
  digitalWrite(DOE, HIGH);

  // Chip select for RAM/ROM
  digitalWrite(MCS,LOW);
  delayMicroseconds(1);

  Serial.println("Setup done.");
}

void writeToROM( uint8_t data, uint16_t address )
{
  digitalWrite(MOE, HIGH);

  writeAddressAndDataToRegisters( data, address );

  digitalWrite(AOE, LOW);
  
  digitalWrite(MWE, LOW);

  digitalWrite(DOE, LOW);

  // Min WE pulse width for AT28C256 is 100ns
  delayMicroseconds(0.15);

  digitalWrite(MWE, HIGH);

  digitalWrite(DOE, HIGH);

  digitalWrite(AOE, HIGH);

  // Poll I/O 7 to see when write is completed
  // it will be the complement of data[7] until
  // then.
  bool finished = false;
  unsigned long timeout = millis()+500;
  uint8_t bit7 = data & 0x7f;
  while(!finished && (millis() < timeout)) {
    digitalWrite(MOE, LOW);
    latch();
    uint8_t ret = readFrom299();
    digitalWrite(MOE, HIGH);
    if ((ret & 0x7f) == bit7)
      finished = true;
  }
   
}

uint8_t readFromROM(uint16_t address)
{
  writeAddressAndDataToRegisters(0, address);
  digitalWrite(AOE, LOW);
  digitalWrite(MOE, LOW);
  latch();
  digitalWrite(MOE, HIGH);
  digitalWrite(AOE, HIGH);
  return readFrom299();
}

void writeAddressAndDataToRegisters( uint8_t data, uint16_t address)
{
  uint8_t hi = address >> 8;
  uint8_t lo = address & 0xFF;

  writeTo299(hi);
  writeTo299(lo);
  writeTo299(data);
  hold();
  sClock();
}

void writeTo299(uint8_t data)
{
  // Set mode to right shift
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  
  // Shift
  shiftOut(DSR, CP, LSBFIRST, data );
}

uint8_t readFrom299()
{
  // Set mode to right shift
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Shift
  return shiftIn299(Q7, CP, LSBFIRST );
}

uint8_t shiftIn299(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
  uint8_t value = 0;
  uint8_t i;

  // Read first bit *before* sending first clock LOW->HIGH edge.
  
  for (i = 0; i < 8; ++i) {
    
    if (bitOrder == LSBFIRST)
      value |= digitalRead(dataPin) << i;
    else
      value |= digitalRead(dataPin) << (7 - i);

    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
  
  return value;
}

void sClock()
{
  digitalWrite(CP, HIGH);
  digitalWrite(CP, LOW);
}

void latch()
{
  // Set mode to parallel in
  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);
  // Clock
  sClock();
}

void hold()
{
  // Set mode to hold
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
}

bool blinker;
unsigned long previousBlink;

void statusBlinkLED()
{
  if (millis()-previousBlink > 1000) {
    digitalWrite(LED_BUILTIN, blinker);
    blinker = !blinker;
    previousBlink = millis();
  }  
}


char getch_nb()
{
  if (Serial.available())
    return Serial.read();

  return -1;
}

uint8_t hexToInt2(const char *ptr)
{
  char buffer[] = {*ptr++, *ptr, 0};
  return (uint8_t)strtol(buffer, nullptr, 16);
}

uint16_t hexToInt4(const char *ptr)
{
  char buffer[] = {*ptr++, *ptr++, *ptr++, *ptr, 0};
  return (uint16_t)strtol(buffer, nullptr, 16);
}

char commandBuffer[256];
uint8_t commandBufferNdx;
uint16_t address;
uint16_t nBytes = 32;

void outputAddress()
{
  char zero = '0';

  if (address < 0x1000)
      Serial.print(zero);
  if (address < 0x100)
      Serial.print(zero);
  if (address < 0x10)
      Serial.print(zero);

  Serial.print(address, HEX);
}

void write()
{
  uint8_t value;
  for(uint16_t n = 0; n < nBytes; n++)
  {
    value  = hexToInt2(&commandBuffer[1 + n*2]);
    
    Serial.print(F("Writing "));
    Serial.print(value, HEX);
    Serial.print(F(" to "));
    Serial.print(F("address: "));
    outputAddress();
    Serial.println();

    writeToROM(value, address++);
    if (address > 0xFFFF)
      break;
  }
  Serial.println(F("Wrote data"));
}

void read()
{
  uint8_t value;

  for(uint16_t n = 0; n < nBytes; n++)
  {
    if (n % 32 == 0 || nBytes <= 0x10) {
      Serial.println();
      outputAddress();
      Serial.print(F(":"));
    }
    value = readFromROM(address++);
    if (value < 0x10)
      Serial.print(F("0"));
    Serial.print(value, HEX);

    if (address > 0xFFFF)
      break;

  }

  Serial.println();
}

void setAddress()
{
  address = hexToInt4(&commandBuffer[1]);
  Serial.print(F("Set address to: "));
  Serial.println(address, HEX);
}

void setNBtytes()
{
  nBytes = hexToInt4(&commandBuffer[1]);
  Serial.print(F("Set N to: "));
  Serial.println(nBytes, HEX);
}

void clear()
{
  uint8_t value = hexToInt2(&commandBuffer[1]);
  Serial.print(F("Clearing to value: "));
  Serial.print(value, HEX);
  address = 0;
  while(address < 0x8000)
  {
    if (address % 0x100 == 0)
    {
      Serial.println(address, HEX);
      blinker = !blinker;
      digitalWrite(LED_BUILTIN, blinker);
    }
    writeToROM(value, address++);
  }

  address = 0;
  Serial.println(F(" done."));
}

void testPattern()
{
  Serial.println(F("Writting test pattern"));
  address = 0;
  bool flip = false;
  while(address < 0x8000)
  {
    if (address % 0x100 == 0)
    {
      Serial.println(address, HEX);
      blinker = !blinker;
      digitalWrite(LED_BUILTIN, blinker);
    }
    if( flip ) {
      writeToROM(0xAA, address++);
    }
    else
    {
      writeToROM(0x55, address++);
    }

    flip = ! flip;
  }

  address = 0;
  Serial.println(F(" done."));

}

void help()
{
  Serial.println(F("Help"));
  Serial.println(F("wXXYYZZ.... - Write N bytes"));
  Serial.println(F("r           - read N bytes"));
  Serial.println(F("aAAAA       - set address"));
  Serial.println(F("sXXXX       - set N"));
  //Serial.println(F("cNN         - clear all addresses to NN"));
} 

void executeCommand()
{
  switch(commandBuffer[0])
  {
    case 'w':
      write();
      break;
    case 'r':
      read();
      break;
    case 'a':
      setAddress();
      break;
    case 's':
      setNBtytes();
      break;
    case 'h':
      help();
      break;
    // Commented out so I don't accidentally cause pointless EEPROM wear 
    //case 'c':
    //  clear();
    //  break;
    //case 't':
    //  testPattern();    
    //  break;
  }
  
  Serial.print(F("Address: "));
  outputAddress();
  Serial.print(F(" - nBytes: "));
  Serial.println(nBytes, HEX);
  Serial.println(F("completed."));

  memset(commandBuffer, 0, sizeof(commandBuffer)*sizeof(char));

}

void handleCommands()
{
  char ch = getch_nb();

  if (ch < 0)
    return; // No input

  Serial.print(ch);

  if (ch == '\n') 
  {
    commandBuffer[commandBufferNdx] = 0;
    commandBufferNdx = 0;
    executeCommand();
    return;
  }

  commandBuffer[commandBufferNdx++] = ch;

  if (commandBufferNdx>255)
  {
    commandBufferNdx = 0;
    Serial.print(F("ERROR:Too long"));
  }

  return;
}

void loop() 
{
  statusBlinkLED();
  handleCommands();
}
