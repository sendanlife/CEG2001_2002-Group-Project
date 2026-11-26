#ifndef LOADCELL_H
#define LOADCELL_H

#define HX711_DOUT_DIR  P3DIR
#define HX711_DOUT_IN   P3IN
#define HX711_DOUT_OUT  P3OUT
#define HX711_DOUT_REN  P3REN
#define HX711_DOUT_SEL  P3SEL
#define HX711_DOUT_BIT  BIT6   

#define HX711_SCK_DIR   P3DIR
#define HX711_SCK_OUT   P3OUT
#define HX711_SCK_SEL   P3SEL
#define HX711_SCK_BIT   BIT5     
#define HX711_SCALE_FACTOR   1096.50f

#define ZERO_DEADBAND_G      0.2f   
#define FILTER_READ_SAMPLES  5      
#define FILTER_SIZE          5      
#define NO_DATA_TICKS        10      

typedef enum {
    LC_OK = 0,
    LC_NO_DATA,        
    LC_STUCK_LOW,    
    LC_SATURATED    
} LoadCellStatus;

void LoadCell_Init(void);
void LoadCell_Tare(unsigned char samples);      
void LoadCell_Poll(void);                         
void LoadCell_Tick(void);                          
unsigned char LoadCell_HasWeight(void);             
float LoadCell_GetWeight(void);                   
long LoadCell_GetCounts(void);                 
long LoadCell_GetRaw(void);                   
LoadCellStatus LoadCell_GetStatus(void);
const char *LoadCell_StatusText(LoadCellStatus s);
void LoadCell_ResetFilter(void);                  

unsigned char LoadCell_IsReady(void);              
unsigned char LoadCell_IsConnected(void);          
long LoadCell_ReadRaw(void);                       
long LoadCell_ReadAverage(unsigned char samples);   
float LoadCell_RawToGrams(long raw);                
float LoadCell_GetGrams(unsigned char samples);    

#endif