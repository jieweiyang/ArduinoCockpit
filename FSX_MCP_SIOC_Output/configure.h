
#define MAX7219_Count 3
//#define DIN_Seg 7
//#define CLK_Seg 9
#define CS_Seg 8 // 42 //8


//#define DIN_Annun  4
//#define CLK_Annun 6
#define CS_Annun 6 //22 //5

#define DebugSerialTX 3
#define DebugSerialRX 2

#define FlashInterval 500

// LED Position  7 6 5 4 3 2 1 0

#define CourseL_ic 0  // Which MAX IC
#define CourseL_pos 5  //Rightest Position
#define CourseL_len 3 // Length

#define IAS_ic 0 
#define IAS_pos 1
#define IAS_len 3

#define IAS_Flash_ic 0
#define IAS_Flash_pos 4
#define IAS_Flash_len 1 


#define HDG_ic 1
#define HDG_pos 5
#define HDG_len 3

#define ALT_ic 1
#define ALT_pos 0
#define ALT_len 5

#define VSPD_ic 2
#define VSPD_pos 0
#define VSPD_len 5

#define CourseR_ic 2
#define CourseR_pos 5
#define CourseR_len 3

// Row,Col
// Row reverse calc


int LEDCordAnnunATArm[]={4,0};
int LEDCordAnnunFDL[]={6,0};
int LEDCordAnnunFDR[]={7,0};
int LEDCordAnnunN1[]={5,0};
int LEDCordAnnunSPEED[]={7,1};
int LEDCordAnnunVNAV[]={4,1};
int LEDCordAnnunLVL_CHG[]={5,1};
int LEDCordAnnunHDG_SEL[]={4,3};
int LEDCordAnnunLNAV[]={5,3};
int LEDCordAnnunVOR_LOC[]={6,3};
int LEDCordAnnunAPP[]={6,1};
int LEDCordAnnunALT_HOLD[]={4,2};
int LEDCordAnnunVS[]={7,3};
int LEDCordAnnunCMD_A[]={5,2};
int LEDCordAnnunCMD_B[]={7,2};
int LEDCordAnnunCWS_A[]={7,7};
int LEDCordAnnunCWS_B[]={6,3};

