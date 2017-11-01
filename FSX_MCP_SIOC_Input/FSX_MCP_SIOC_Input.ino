/*

   Over/Under speed blinking
   IAS from Long to Float

*/



#include "PMDGEvents.h"
#include "PMDGEvents_Action.h"
#include "PMDGData.h"
#include "configure.h"
#include "LedControl.h"
#include <SPI.h>

byte LedAnnunRowStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};
//byte bitLedCol[] = {1, 2, 4, 8, 16, 32, 64, 128};
byte bitLedCol[] = {128, 64, 32, 16, 8, 4, 2, 1};

boolean MCP_IASOverspeed = false;
boolean MCP_IASOverspeedCurrent = false;
boolean MCP_IASUnderspeed = false;
boolean MCP_IASUnderspeedCurrent = false;
unsigned long previousMillis = 0;

// ButtonState / Encoder Config
byte curInState[BUTTON_GROUP_SIZE];
byte preInState[BUTTON_GROUP_SIZE];
byte EncoderConfig[BUTTON_GROUP_SIZE];

// Prototypes - Handling different action.
void HandleInput();  // processing Serial/COM incoming.
void HandleVaiableReceived();  //Process with income command.
void RegisterVariables();

// Input/Output buffer
char line[64]; // Output buffer
const byte InputBufferSize = 32;
char InputBuffer[InputBufferSize];

// Variable for input prase.
int ivar = 0;
long ivalue = 0; // Input value as Int
String svalue; // Input value as String
char cvalue[8]; // Input value as Char arrary
int valuelength = 0;   // Lenght of C value


// Union for converting LONG to FLOAT
union equiv {
  float f;
  unsigned long l;
} equiv ;


// Declear MAX7219 Control
//LedControl lcSeg = LedControl(DIN_Seg, CLK_Seg, CS_Seg, MAX7219_Count);
//LedControl lcAnnun = LedControl(DIN_Annun, CLK_Annun, CS_Annun, 1);



void setup()
{

  Serial.begin(115200);
  Serial.println("Started");

  SPI.begin ();
  pinMode (CS_Button, OUTPUT);
  digitalWrite (CS_Button, HIGH);

  // Setup Encoder PIN
  EncoderConfig[EncoderCrsL_IC] |= 1 << EncoderCrsL_B1;
  EncoderConfig[EncoderCrsL_IC] |= 1 << EncoderCrsL_B2;

  EncoderConfig[EncoderCrsR_IC] |= 1 << EncoderCrsR_B1;
  EncoderConfig[EncoderCrsR_IC] |= 1 << EncoderCrsR_B2;

  EncoderConfig[EncoderIAS_IC] |= 1 << EncoderIAS_B1;
  EncoderConfig[EncoderIAS_IC] |= 1 << EncoderIAS_B2;

  EncoderConfig[EncoderHDG_IC] |= 1 << EncoderHDG_B1;
  EncoderConfig[EncoderHDG_IC] |= 1 << EncoderHDG_B2;

  EncoderConfig[EncoderALT_IC] |= 1 << EncoderALT_B1;
  EncoderConfig[EncoderALT_IC] |= 1 << EncoderALT_B2;

  EncoderConfig[EncoderVSPD_IC] |= 1 << EncoderVSPD_B1;
  EncoderConfig[EncoderVSPD_IC] |= 1 << EncoderVSPD_B2;
  //
  // Register the variables we need with DataEventServer

  RegisterVariables();
}

void loop()
{
  
  ButtonCheck();
  /*
      Here the main loop discovers that input was received, and will dispatch to handle it.
      If input is was parced by 'HandleInput' and a variable was received we find ivar non zero
  */

    //
  char rc;
  static byte ndx;

  /*
        Collect Serial Input NON-BLocking.
        Continue receiving characters until '\n' (linefeed) is received.
        Start the handling of the line (command) received in HandleInput.
  */
  if (Serial.available() > 0)
  {
    rc = Serial.read();
    InputBuffer[ndx++] = rc;  // Character just received to input buffer
    if (rc == '\n')
    {
      HandleInput();
      ndx = 0;
      /*
         As this code is not multy thread we can safely reset the serial input pointer to begin
         of buffer. As no further characters can be received while we are handling the input
      */
    }
    if (ndx > InputBufferSize) ndx = InputBufferSize; // Avoid buffer overflow
  }

  /*
      Here the main loop discovers that input was received, and will dispatch to handle it.
      If input is was parced by 'HandleInput' and a variable was received we find ivar non zero
  */

  /*
     Next codes run based on interval
  */
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= FlashInterval)
  {
    //LEDSPDFlashing();
    previousMillis = currentMillis;
  }



} //    End Main loop



// Routines

void HandleInput()
{
  // This function parces the input line that has the format of  "nnn=nnn\n"
  // the result is left in:
  // ivar and ivalue

  char var[8];
  byte varIndex = 0;  // Index pointer to temp var string
  char digit;
  byte index = 0;   // Index pointer to input chars

  // PMDGDataEventServer will send "Ready" if the serial port is opened.
  //
  if (InputBuffer[0] == 'R')
  {
    /*
            We assume 'Ready' is received,
            The server sends this message after it (re)connected this arduino
            Or (re)connected SIOC
    */
    RegisterVariables();   //We should register the variables we want te receive from SIOC
    return;
  }
  /*
       If this line was not "Ready" it must be a variable,
       Get the first number (variable number)
  */
  do
  {
    // Everything before the '=' sign
    var[varIndex++] = InputBuffer[index++];
    if (index > InputBufferSize) return; // To avoid crash if input is wrong
  } while (InputBuffer[index] != '=');
  var[varIndex] = 0;
  //
  index++; // skip =
  ivar = atoi(var);

  varIndex = 0;
  // Get the second number is the value
  do
  {
    // Everything after the '=' sign
    cvalue[varIndex++] = InputBuffer[index++];
    if (index > InputBufferSize) return; // To avoid crash if input is wrong
  } while (InputBuffer[index] != '\n');
  cvalue[varIndex] = 0;
  ivalue = atol(cvalue);
  valuelength = varIndex - 1;
  /*
     Now ivar and ivalue contain the 2 number from the vvv=nnn message
     The main loop will see that ivar is not 000 and react on that.
  */
}

void RegisterVariables()
{
  /* This function sends the variable numbers of the variables we are interested in to
    the Server, the server will then send it to SIOC.
    The function will be called by startuo and when a "ready" message from the server is received.
    Registration messages can be sent at any time. Registrations do not need to be in order of in
    single message.
    As a test we request variables 100-104
  */
  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          EVT_MCP_COURSE_SELECTOR_L,
          EVT_MCP_FD_SWITCH_L,
          EVT_MCP_AT_ARM_SWITCH,
          EVT_MCP_N1_SWITCH,
          EVT_MCP_SPEED_SWITCH);
  Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          EVT_MCP_CO_SWITCH,
          EVT_MCP_SPEED_SELECTOR,
          EVT_MCP_VNAV_SWITCH,
          EVT_MCP_SPD_INTV_SWITCH,
          EVT_MCP_BANK_ANGLE_SELECTOR);
  Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          EVT_MCP_HEADING_SELECTOR,
          EVT_MCP_LVL_CHG_SWITCH,
          EVT_MCP_HDG_SEL_SWITCH,
          EVT_MCP_APP_SWITCH,
          EVT_MCP_ALT_HOLD_SWITCH);
  Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          EVT_MCP_VS_SWITCH,
          EVT_MCP_VOR_LOC_SWITCH,
          EVT_MCP_LNAV_SWITCH,
          EVT_MCP_ALTITUDE_SELECTOR,
          EVT_MCP_VS_SELECTOR);
  Serial.println(line);
  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          EVT_MCP_CMD_A_SWITCH,
          EVT_MCP_CMD_B_SWITCH,
          EVT_MCP_CWS_A_SWITCH,
          EVT_MCP_CWS_B_SWITCH,
          EVT_MCP_DISENGAGE_BAR),
          Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          EVT_MCP_FD_SWITCH_R,
          EVT_MCP_COURSE_SELECTOR_R,
          EVT_MCP_ALT_INTV_SWITCH),
          0,0
          ;
  Serial.println(line);
  //
  //  LCD reset code can be here.
}

void ButtonHandle(byte Button, byte ONOF) {
  //SendEvent(Button, ONOF);
  switch (Button)
  {
    case 14:
      if (ONOF == 1) SendEvent(EVT_MCP_COURSE_SELECTOR_L,  MOUSE_FLAG_RIGHTSINGLE);
      if (ONOF == 0) SendEvent(EVT_MCP_COURSE_SELECTOR_L,  MOUSE_FLAG_LEFTSINGLE);
      break;

    case 6:
      if (ONOF == 1) SendEvent(EVT_MCP_SPEED_SELECTOR,  MOUSE_FLAG_RIGHTSINGLE);
      if (ONOF == 0) SendEvent(EVT_MCP_SPEED_SELECTOR,  MOUSE_FLAG_LEFTSINGLE);
      break;

    case 4:
      if (ONOF == 1) SendEvent(EVT_MCP_HEADING_SELECTOR,  MOUSE_FLAG_RIGHTSINGLE);
      if (ONOF == 0) SendEvent(EVT_MCP_HEADING_SELECTOR,  MOUSE_FLAG_LEFTSINGLE);
      break;

    case 16:
      if (ONOF == 1) SendEvent(EVT_MCP_ALTITUDE_SELECTOR,  MOUSE_FLAG_RIGHTSINGLE);
      if (ONOF == 0) SendEvent(EVT_MCP_ALTITUDE_SELECTOR,  MOUSE_FLAG_LEFTSINGLE);
      break;

    case 22:
      if (ONOF == 1) SendEvent(EVT_MCP_VS_SELECTOR,  MOUSE_FLAG_RIGHTSINGLE);
      if (ONOF == 0) SendEvent(EVT_MCP_VS_SELECTOR,  MOUSE_FLAG_LEFTSINGLE);
      break;

    case 30:
      if (ONOF == 1) SendEvent(EVT_MCP_COURSE_SELECTOR_R,  MOUSE_FLAG_RIGHTSINGLE);
      if (ONOF == 0) SendEvent(EVT_MCP_COURSE_SELECTOR_R,  MOUSE_FLAG_LEFTSINGLE);
      break;

    case 11:
      SendEvent(EVT_MCP_FD_SWITCH_L,  ONOF);
      break;

    case 24:
      SendEvent(EVT_MCP_FD_SWITCH_R,  ONOF);
      break;

    case 12:
      if (ONOF == 0 ) SendEvent(EVT_MCP_AT_ARM_SWITCH, 0);
      break;
    case 13:
      if (ONOF == 0 ) SendEvent(EVT_MCP_AT_ARM_SWITCH,  1);
      break;

    case 1:
      if (ONOF == 0 ) SendEvent(EVT_MCP_N1_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 2:
      if (ONOF == 0 ) SendEvent(EVT_MCP_SPEED_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 8:
      if (ONOF == 0 ) SendEvent(EVT_MCP_CO_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 3:
      if (ONOF == 0 ) SendEvent(EVT_MCP_VNAV_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 10:
      if (ONOF == 0 ) SendEvent(EVT_MCP_SPD_INTV_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 0:
      if (ONOF == 0 ) SendEvent( EVT_MCP_LVL_CHG_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 9:
      if (ONOF == 0 ) SendEvent(EVT_MCP_HDG_SEL_SWITCH , MOUSE_FLAG_LEFTSINGLE);
      break;

    case 25:
      if (ONOF == 0 ) SendEvent(EVT_MCP_APP_SWITCH , MOUSE_FLAG_LEFTSINGLE);
      break;

    case 26:
      if (ONOF == 0 ) SendEvent( EVT_MCP_ALT_HOLD_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 29:
      if (ONOF == 0 ) SendEvent(EVT_MCP_VS_SWITCH , MOUSE_FLAG_LEFTSINGLE);
      break;

    case 19:
      if (ONOF == 0 ) SendEvent(EVT_MCP_VOR_LOC_SWITCH , MOUSE_FLAG_LEFTSINGLE);
      break;

    case 18:
      if (ONOF == 0 ) SendEvent( EVT_MCP_LNAV_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 20:
      if (ONOF == 0 ) SendEvent(EVT_MCP_CMD_A_SWITCH , MOUSE_FLAG_LEFTSINGLE);
      break;

    case 27:
      if (ONOF == 0 ) SendEvent( EVT_MCP_CMD_B_SWITCH, MOUSE_FLAG_LEFTSINGLE);
      break;

    case 28:
      if (ONOF == 0 ) {
        SendEvent(EVT_MCP_CWS_A_SWITCH , MOUSE_FLAG_LEFTSINGLE);
        RegisterVariables();
      }
      break;

    case 21:
      if (ONOF == 0 ) SendEvent(EVT_MCP_ALT_INTV_SWITCH , MOUSE_FLAG_LEFTSINGLE);
      break;

      /*
        case :
        break;
        case :
        break;
      */

      //default:
      //SendEvent(Button, ONOF);

  }

}


void ButtonCheck()
{
  digitalWrite (CS_Button, LOW);    // pulse the parallel load csButton
  digitalWrite (CS_Button, HIGH);
  //byte tmpState=0;
  byte BitMask;
  byte ChgMask = 0;
  byte now, pre;   // Variable for Rotery Encoder Routine

  for (int i = 0; i < BUTTON_GROUP_SIZE; i++) {
    preInState[i] = curInState[i];
    curInState[i] = SPI.transfer (0);
  }

  for (int i = 0; i < BUTTON_GROUP_SIZE; i++) {
    BitMask = 1;
    ChgMask = preInState[i] ^ curInState[i];
    if (ChgMask != 0 )
    {
      for (int j = 0; j < 8; j++)   // Check for bit's that changed
      {
        if ((EncoderConfig[i]& BitMask) != 0) // Check if it's an encoder.
        {
          BitMask = BitMask << 1;
          now = (curInState[i] >> j ) & B11; //0x03; Get current two pin state..
          pre = (preInState[i] >> j ) & B11; //0x03; Get previous two pin state..

          if (((now ^ pre) & 0X03) != 0) // Did one of the pin change
          {
            if (now == 0)       // Yes, is it middle (AB) position ?
            {
              if (pre == 1 ) {
                sprintf(line, "IC %d Encoder %d ", i, j);
                //Serial.print(line);
                //Serial.println(" Turn Right"); // Send to Buffer
                ButtonHandle((i * 8) + j, 1);
              }
              if (pre == 2 ) {
                sprintf(line, "IC %d Encoder %d ", i, j);
                //Serial.print(line);
                //Serial.println(" Turn Left"); // Send to Buffer
                ButtonHandle((i * 8) + j, 0);
              }
              ButtonHandle((i * 8) + j, 3); // Button Release
              
            } // End If - Encoder direction..
          } // End If - Encoder Changed

          j++;  // Skip next pin as Encoder used two pins.

        } // End If - IsEncoder?
        else
        { // Button Change
          if ((ChgMask & BitMask) != 0 )         // did this bit change?
          {
            sprintf(line, "IC %d Button %d ", i, j);
            //Serial.print(line);
            //Serial.println(curInState[i] & BitMask, BIN);
            if ((curInState[i] & BitMask) == 0)
              ButtonHandle((i * 8) + j, 0);
            else
              ButtonHandle((i * 8) + j, 1);
          }
        }
        BitMask = BitMask << 1;
      }
    }
  }

  delay (10);   // debounce
}  // end of loop

void SendEvent(int Event, int EventValue ) {
  sprintf(line, "%d=%d", Event, EventValue);
  Serial.println(line);
  sprintf(line, "%d=%d", Event, MOUSE_FLAG_LEFTRELEASE);
  Serial.println(line);

}

