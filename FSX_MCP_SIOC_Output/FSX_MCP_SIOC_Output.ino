/*

   Over/Under speed blinking
   IAS from Long to Float

*/



#include "PMDGEvents.h"
#include "PMDGEvents_Action.h"
#include "PMDGData.h"
//#include "Common.h"
#include "configure.h"
#include <LedControl.h>
#include <SoftwareSerial.h>

//boolean LedAnnunUpdate = false;
//boolean LedSegUpdate = false;

byte LedAnnunRowStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};
//byte bitLedCol[] = {1, 2, 4, 8, 16, 32, 64, 128};
byte bitLedCol[] = {128, 64, 32, 16, 8, 4, 2, 1};

boolean MCP_IASOverspeed = false;
boolean MCP_IASOverspeedCurrent = false;
boolean MCP_IASUnderspeed = false;
boolean MCP_IASUnderspeedCurrent = false;
unsigned long previousMillis = 0;



// Prototypes - Handling different action.
void HandleInput();  // processing Serial/COM incoming.
void HandleVaiableReceived();  //Process with income command.
void RegisterVariables();




//
//
//
// Input/Output buffer
char line[64]; // Output buffer
const byte InputBufferSize = 32;
char InputBuffer[InputBufferSize];
//
//
// Variable for input prase.
int ivar = 0;
int ivar_p;
long ivalue = 0; // Input value as Int
long ivalue_p;
String svalue; // Input value as String
char cvalue[8]; // Input value as Char arrary
int valuelength = 0;   // Lenght of C value


// Union for converting LONG to FLOAT
union equiv {
  float f;
  unsigned long l;
} equiv ;


// Declear MAX7219 Control // Using Hardware SPI LedControl Lib.
//LedControl lcSeg = LedControl(DIN_Seg, CLK_Seg, CS_Seg, MAX7219_Count);
LedControl lcSeg = LedControl(CS_Seg, MAX7219_Count);

//LedControl lcAnnun = LedControl(DIN_Annun, CLK_Annun, CS_Annun, 1);
LedControl lcAnnun = LedControl(CS_Annun, 1);

SoftwareSerial DbgSerial(DebugSerialRX, DebugSerialTX); // RX, TX


void setup()
{

  Serial.begin(115200);
  //Serial1.begin(115200);
  Serial.println("Started");
  DbgSerial.begin(115200);
  DbgSerial.println("Started");
  

  for (int c = 0; c <= MAX7219_Count - 1; c++) {
    lcSeg.shutdown(c, false);
    /* Set the brightness to a medium values */
    lcSeg.setIntensity(c, 1);
    /* and clear the display */
    lcSeg.clearDisplay(c);
  }

  lcAnnun.shutdown(0, false);
  lcAnnun.setIntensity(0, 15);
  lcAnnun.clearDisplay(0);
  //
  // Register the variables we need with DataEventServer
  RegisterVariables();




}

void loop()
{
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
      DbgSerial.print(ndx, DEC);
      DbgSerial.println(String(InputBuffer));
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
  if (ivar != 0)
  {
    HandleVaiableReceived();
    // the handling routine will have to clear ivar to indicate handling done.
  }

  /*
     Next codes run based on interval
  */
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= FlashInterval)
  {
    LEDSPDFlashing();
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

void HandleVaiableReceived()
{
  if ((ivar == ivar_p) && ( ivalue == ivalue_p)) return;
  ivar_p = ivar;
  ivalue_p = ivalue;
  //Serial1.print(ivar);
  //Serial1.print("=");
  //Serial1.print(ivalue);
  switch (ivar)
  {
    //void LEDSegOut(LedControl lc, int ic, int startPos, int hwlen, char value[], boolean padding) {
    case MCP_Course_L:
      LEDSegOut(lcSeg, CourseL_ic, CourseL_pos, CourseL_len,  cvalue, true);
      break;
    case MCP_Course_R:
      LEDSegOut(lcSeg, CourseR_ic, CourseR_pos, CourseR_len,  cvalue, true);
      break;
    case MCP_IASMach:
      equiv.l = ivalue;
      svalue = String(equiv.f);
      if (svalue.indexOf('.') < 3) svalue.remove(0, 2); // if value <0 in form of 0.xx
      svalue.toCharArray(cvalue, 4);
      LEDSegOut(lcSeg, IAS_ic, IAS_pos, IAS_len, cvalue, false);
      break;
    case MCP_IASBlank:
      // Ias is blank
      if (ivalue == 1) {
        LEDSegOut(lcSeg, IAS_ic, IAS_pos, IAS_len, ' ', false);
      }
      break;
    case MCP_IASOverspeedFlash: // IAS Flashing
      if (ivalue == 1) {
        MCP_IASOverspeed = true;
      }
      else {
        MCP_IASOverspeed = false;
        lcSeg.setChar(IAS_Flash_ic, IAS_Flash_pos, ' ', false);
      }
      break;
    case MCP_IASUnderspeedFlash: // IAS Flashing

      if (ivalue == 1) {
        MCP_IASUnderspeed = true;
      }
      else {
        MCP_IASUnderspeed = false;
        lcSeg.setChar(IAS_Flash_ic, IAS_Flash_pos, ' ', false);
      }

      break;
    case MCP_Heading:
      LEDSegOut(lcSeg, HDG_ic, HDG_pos, HDG_len,  cvalue, true);
      break;
    case MCP_Altitude:
      switch (cvalue[0]) {  // when Altitute set to 0
        case '0':
          LEDSegOut(lcSeg, ALT_ic, ALT_pos, ALT_len,  cvalue, true);
          break;
        default:
          LEDSegOut(lcSeg, ALT_ic, ALT_pos, ALT_len,  cvalue, false);
      }
      break;
    case MCP_VertSpeed:
      if (ivalue > 30000) {
        ivalue = 65535 - ivalue + 1;
        sprintf(cvalue, "-%d", ivalue);
      }
      switch (cvalue[0]) {  // when Altitute set to 0
        case '0':
          LEDSegOut(lcSeg, VSPD_ic, VSPD_pos, VSPD_len, cvalue, true);
          break;
        default:
          LEDSegOut(lcSeg, VSPD_ic, VSPD_pos, VSPD_len, cvalue, false);
          break;
      }
    case MCP_VertSpeedBlank:
      //VSPD is blank
      if (ivalue == 1) {
        LEDSegOut(lcSeg, VSPD_ic, VSPD_pos, VSPD_len, ' ', false);
      }
      break;
    case MCP_annunFD_L:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunFDL, ivalue);
      break;
    case MCP_annunFD_R:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunFDR, ivalue);
      break;
    case MCP_annunATArm:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunATArm, ivalue);
      break;
    case MCP_annunN1:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunN1, ivalue);
      break;
    case MCP_annunSPEED:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunSPEED, ivalue);
      break;
    case MCP_annunVNAV:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunVNAV, ivalue);
      break;
    case MCP_annunLVL_CHG:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunLVL_CHG, ivalue);
      break;
    case MCP_annunHDG_SEL:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunHDG_SEL, ivalue);
      break;
    case MCP_annunLNAV:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunLNAV, ivalue);
      break;
    case MCP_annunVOR_LOC:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunVOR_LOC, ivalue);
      break;
    case MCP_annunAPP:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunAPP, ivalue);
      break;
    case MCP_annunALT_HOLD:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunALT_HOLD, ivalue);
      break;
    case MCP_annunVS:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunVS, ivalue);
      break;
    case MCP_annunCMD_A:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunCMD_A, ivalue);
      break;
    case MCP_annunCWS_A:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunCWS_A, ivalue);
      break;
    case MCP_annunCMD_B:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunCMD_B, ivalue);
      break;
    case MCP_annunCWS_B:
      LEDLitOut(lcAnnun, 0, LEDCordAnnunCWS_B, ivalue);
      break;
    default:
      break;
  }
  ivar = 0; // Clear ivar to indicate to main look we handled the variable
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
          MCP_Course_L,
          MCP_Course_R,
          MCP_IASMach,
          MCP_IASBlank,
          MCP_IASOverspeedFlash);
  Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          MCP_IASUnderspeedFlash,
          MCP_Heading,
          MCP_Altitude,
          MCP_VertSpeed,
          MCP_VertSpeedBlank);
  Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          MCP_annunFD_L,
          MCP_annunFD_R,
          MCP_annunATArm,
          MCP_annunN1,
          MCP_annunSPEED);
  Serial.println(line);

  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          MCP_Course_L,
          MCP_Course_R,
          MCP_IASMach,
          MCP_IASBlank,
          MCP_IASOverspeedFlash);
  Serial.println(line);
  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          MCP_annunVNAV,
          MCP_annunLVL_CHG,
          MCP_annunHDG_SEL,
          MCP_annunLNAV,
          MCP_annunVOR_LOC);
  Serial.println(line);
  sprintf(line, "Reg:%d:%d:%d:%d:%d",
          MCP_annunAPP,
          MCP_annunALT_HOLD,
          MCP_annunVS,
          MCP_annunCMD_A,
          MCP_annunCWS_A,
          MCP_annunCMD_B,
          MCP_annunCWS_B);
  Serial.println(line);
  //
  //  LCD reset code can be here.
}


// LED Output Control

void LEDSegOut(LedControl lc, int ic, int startPos, int hwlen, char value[], boolean padding) {
  char outval[20];
  String outvalS;

  switch (padding) {
    case true:
      outvalS = strPadding(value, hwlen, '0');
      break;
    case false:
      outvalS = strPadding(value, hwlen, ' ');
      break;
  }
  outvalS.toCharArray(outval, hwlen + 1);

  for (int i = hwlen; i > 0; i--) {
    // OUt from padding
    lc.setChar(ic, startPos + i - 1, outval[hwlen - i], false);
    delay(2);

    // out from original
    //lc.setChar(ic, startPos + i - 1, value[i], false);
  }
}

void LEDLitOut(LedControl lc, int ic, int LEDCord[], int LEDLit) {

  switch (LEDLit) {
    case 1:
      LedAnnunRowStatus[LEDCord[0]] |= bitLedCol[LEDCord[1]];
      break;
    case 0:
      LedAnnunRowStatus[LEDCord[0]] &= ~(bitLedCol[LEDCord[1]]);
      break;
  }
  lc.setRow(ic, LEDCord[0], LedAnnunRowStatus[LEDCord[0]]);
}

String strPadding(char value[], int padLength, char charPad) {

  String strForPad = String (value);
  if (strForPad.length() >= padLength) {
    return strForPad;
  }
  String strOut = strForPad;
  for (int i = 0; i < (padLength - (strForPad.length() )); i ++) {
    strOut = String(charPad) + strOut;
  }
  return strOut;

}

void LEDSPDFlashing() {
  // Overspeed flashing.
  if (MCP_IASOverspeed == true) {
    //Serial1.println("Do Blink Over");
    MCP_IASOverspeedCurrent = !MCP_IASOverspeedCurrent;
    if (MCP_IASOverspeedCurrent == false) lcSeg.setChar(IAS_Flash_ic, IAS_Flash_pos, ' ', false);
    if (MCP_IASOverspeedCurrent == true) lcSeg.setChar(IAS_Flash_ic, IAS_Flash_pos, '8', false);
  }
  // Underspeed Flashing
  if (MCP_IASUnderspeed == true ) {
    //Serial1.println("Do Blink Under");
    MCP_IASUnderspeedCurrent = !MCP_IASUnderspeedCurrent;
    if (MCP_IASUnderspeedCurrent == false) lcSeg.setChar(IAS_Flash_ic, IAS_Flash_pos, ' ', false);
    if (MCP_IASUnderspeedCurrent == true) lcSeg.setChar(IAS_Flash_ic, IAS_Flash_pos, 'A', false);
  }

}

