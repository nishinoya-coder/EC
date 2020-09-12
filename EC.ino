#include <SPI.h>

#include <ETH.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiSTA.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t




/**************wifi define ***************/
const char *ssid = "EC";
const char *password = "EC666666";
WiFiServer server(8090);


/**************SPI define***************/ 
static const int spiClk = 1000000;    // 1MHz
SPIClass * vspi = NULL;
SPIClass * hspi = NULL;


/**************ad define***************/  
//ad pin  (num=GPIO_num)
#define ad_start 22
#define ad_drdy 17

//ad command
#define NOP 0x00
#define WAKEUP 0x02
#define POWERDOWN 0x04
#define RESET 0x06
#define START 0x08
#define STOP 0x0a
#define RDATA 0x12
#define RREG(i) (0x20 | i)
#define WREG(i) (0x40 | i)

//ad address
#define ID 0x00
#define STATUS 0x01
#define INPMUX 0x02
#define PGA 0x03
#define DATARATE 0x04
#define REF 0x05
#define IDACMAG 0x06
#define IDACMUX 0x07
#define VBIAS 0x08
#define SYS 0x09
#define OFCAL0 0x0b
#define OFCAL1 0x0c
#define FSCAL0 0x0e
#define FSCAL1 0x0f

//ad reg
#define PGA_(i) (1<<3 | i)
    //gain list: 0-1, 1-2, 2-4, 3-8, 4-16, 5-32, 6-64, 7-128
#define MUX_(i) (i<<4 | (0x2)) 
    //[7:4]-i for single-end port to positive 
    //[3:0]-0010 for negative port connect to AIN2
#define DATARATE_ (0x3e) 
    //8'b00111110; [6]-0 for internal clock, 
    //[5]-1 for single-shot mode,
    //[4]-1 reserved for Low-latency filter
    //[3:0]-1110 for 4000SPS
#define REF_ (0x0a)
    //[5:4]-00 for enable reference buffer bypass
    //[3:2]-10 for internal 2.5v reference
    //[1:0]-10 for internal always on
#define IDACMAG_(i) (i)
#define IDACMUX_(i,j) (i<<4 | j)
#define VBIAS_(i) (i<<6)        //connect vbias to AINCOM
#define SYS_(i) (i<<5 || 1<<4)  //[7:5]-i for system monitor,
                                //default 8 samples, default disable timeout
#define MON_DISABLE 0
#define MON_VBIAS 1
#define MON_TEMP 2
#define MON_AVDDtoAVSSdiv4 3
#define MON_DVDDdiv4 4
#define MON_p2uA 5
#define MON_1uA 6
#define MON_10uA 7
//rest regs are useless or reserved



/**************da define***************/
//da pin define     (num=GPIO_num)
#define HSPI_MISO   12
#define HSPI_MOSI   13
#define HSPI_SCLK   14
#define HSPI_SS     15  




/**************ad function***************/
void wait()
{     while (digitalRead(ad_drdy));   }

void write(u8 cmd)
{     vspi->transfer(cmd);    }

u8 read(void)
{     return vspi->transfer(0x0);   }

u8 read_reg(u8 address)     //read 1 register for once
{
  //wait();
  write(1<<5 | address);
  write(0x00);
  return read();
}
                  
void write_reg(u8 address, u8 reg)
{
  write(1<<6 | address);
  write(0x00);
  write(reg);
}

void set_pga(u8 g)
{     write_reg(PGA,PGA_(g));   }
                  
void set_mux(u8 i)
{     write_reg(INPMUX,MUX_(i));    }
                  
s16 read_data(void)
{
  s16 a;
  digitalWrite(ad_start, 1);
  wait();
  write(RDATA);
  a= (read()<<8);
  a|=read();
  digitalWrite(ad_start, 0);
  return a;
}

void ad_init(void)
{
  pinMode(ad_start, OUTPUT);
  pinMode(ad_drdy, INPUT);
  digitalWrite(ad_start, 0);
  vspi = new SPIClass(VSPI);
  vspi->begin();
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));
  write(RESET);
  delay(2);
  write_reg(REF,REF_);
  write_reg(DATARATE,DATARATE_);
}




/**************da function***************/
u16 write_da(u16 da_data)
{
  digitalWrite(HSPI_SS, LOW);
  hspi->transfer16(da_data);
  digitalWrite(HSPI_SS, HIGH);
  Serial.println("da transfer done");
}

void da_init(void)
{
  hspi = new SPIClass(HSPI);
  hspi->begin();
  pinMode(HSPI_SS, OUTPUT);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  //  1.MSBFIRST/LSBFIRST  2.SPI_MODE0/1/2/3
}




/**************measure function***************/
float cal_volt(u8 gain, s16 code)
{
  return  ((float)(code*2500)/(float)(1<<(15+gain)));
}

s32 dat[2];
int gain[2];
float I_data[12];
void sweep_read()
{
  s16 temp;
  s8 i,j;
  for(i=1;i<2;i++)
  {
    set_mux(i);
    for(j=7;j>=0;j--)
    {
      set_pga(j);
      temp = read_data();
      if (temp <= 32764 && temp >= -32764)
      {
//        Serial.print("i=");
//        Serial.print(i);
//        Serial.print(" j=");
//        Serial.print(j);
//        Serial.print(" temp=");
//        Serial.println(temp);
        dat[i]=temp;
        gain[i]=j;
        I_data[i] = -cal_volt(j, temp);
        break;
      }                
        dat[i]=temp;
        gain[i]=j;
        I_data[i] = -cal_volt(j, temp);
    }
  }
}



/**************PROGRAM START***************/
void setup() {
  //Must be If you want to use serial monitor. 
  Serial.begin(115200);
  Serial.println();
  delay(5000);

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  Serial.print("ssid: ");
  Serial.println(ssid);  
  Serial.print("password: ");
  Serial.println(password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");

  // da init
  da_init();
  Serial.println("da init done");

  //ad init
  ad_init();
  Serial.println("ad init done");
//  Serial.println(read_reg(0x00));
}




char flag=0;
s8 volt;
s8 volt_data[5];
s16 voltout=0;
u16 voltoutput;
u8 i;

float ftemp1;
s16 *p;
s16 *ps;

void loop() {
  delay(2000);
  while(1)
  {
    WiFiClient client = server.available();   // listen for incoming clients
    //Serial.println("cylce");

    while (client.connected()) 
    {            
      //loop while the client's connected
      sweep_read();
      ftemp1 = I_data[1];
      
      if (client.available()) 
      {
//        sweep_read();
//        ftemp1 = I_data[1];

        //connect 'voltout' to 'I_data', make transfer for once 
        p = (s16*)&ftemp1;
        ps = (p+2);
        *ps = voltout;
        
        Serial.println();
        //printf("%f\n\r", ftemp1);

        //send 'voltout' and 'I_data' to upper
        client.write((uint8_t*)&ftemp1, 6);
        
        // if there's bytes to read from the client
        i=0;
        Serial.println("read once");
        do{
          Serial.print(i);
          Serial.print(" ");
          volt = client.read();
//          Serial.print(volt);
//          Serial.print(" ");
          volt_data[i] = volt - 48;
          Serial.println(volt_data[i]);
          i+=1;
        }while((volt!=47)&(volt!=-1));

        if(volt_data[0] == -3)
        {
          switch(i){
            case 6: voltout = (-1)*(volt_data[1]*1000 + volt_data[2]*100 + volt_data[3]*10 + volt_data[4]);  break;
            case 5: voltout = (-1)*(volt_data[1]*100 + volt_data[2]*10 + volt_data[3]);  break;
            case 4: voltout = (-1)*(volt_data[1]*10 + volt_data[2]);  break;
            case 3: voltout = (-1)*(volt_data[1]);  break;
            case 2: break;
            case 1: break;
            default: break;
          }
          Serial.print("voltout = ");
          Serial.println(voltout);
        }
        else
        {
          switch(i){
            case 5: voltout = volt_data[0]*1000 + volt_data[1]*100 + volt_data[2]*10 + volt_data[3];  break;
            case 4: voltout = volt_data[0]*100 + volt_data[1]*10 + volt_data[2];  break;
            case 3: voltout = volt_data[0]*10 + volt_data[1];  break;
            case 2: voltout = volt_data[0];  break;
            case 1: break;
            default: break;
          }
          Serial.print("voltout = ");
          Serial.println(voltout);
        }
        voltoutput = voltout*32768/2500 + 32768;
        Serial.print("voltout* = ");
        Serial.println(voltoutput);
        write_da(voltoutput);
      delay(20);
      }
    }
  }
}
