/*
 Huge thank you to Uspizig for offering this library that I have modified for TFT_eSPI: https://github.com/Uspizig/MLX90640
 Has to be Library, TFT_eSPI Rev 2.5.43
 The latest does not work

*/

#include <TFT_eSPI.h> 
#include <SPI.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "CST816T.h"

#define TA_SHIFT 8 //Default shift for MLX90640 in open air
#define MLX_VDD  11
#define MLX_SDA  41
#define MLX_SCL  42
#define SCREEN_ROTATION 1
#define BAT_ADC  10
#define SCREEN_BL_PIN 9

#define _SCALE 9
#define LONG_PUSH_T 1000
const int buttonPin1 = 0;  
const int buttonPin2 = 21; 
bool buttonState1 = 1;  
bool buttonState2 = 0;  

const byte MLX90640_address = 0x33;
static float mlx90640To[768];
paramsMLX90640 mlx90640;


uint16_t test_points[10][2];
int R_colour, G_colour, B_colour;            
int i, j;                                    
float T_max, T_min, T_avg;                            
float T_center;  
int max_x, max_y, min_x, min_y;

float bat_v;

unsigned long previousMillis = 0; 
const long interval = 1000;
bool  lock = false;
bool power_on = true;

TFT_eSPI tft = TFT_eSPI();  
CST816T touch(7, 6, 5, -1);	// sda, scl, rst, irq



// ===============================
// ===== determine the colour ====
// ===============================
uint8_t hashmap[182][3];


void getColour(int j)
   {
    if (j >= 0 && j < 30)
       {
        R_colour = 0;
        G_colour = 0;
        B_colour = 20 + (120.0/30.0) * j;
       }
    
    if (j >= 30 && j < 60)
       {
        R_colour = (120.0 / 30) * (j - 30.0);
        G_colour = 0;
        B_colour = 140 - (60.0/30.0) * (j - 30.0);
       }

    if (j >= 60 && j < 90)
       {
        R_colour = 120 + (135.0/30.0) * (j - 60.0);
        G_colour = 0;
        B_colour = 80 - (70.0/30.0) * (j - 60.0);
       }

    if (j >= 90 && j < 120)
       {
        R_colour = 255;
        G_colour = 0 + (60.0/30.0) * (j - 90.0);
        B_colour = 10 - (10.0/30.0) * (j - 90.0);
       }

    if (j >= 120 && j < 150)
       {
        R_colour = 255;
        G_colour = 60 + (175.0/30.0) * (j - 120.0);
        B_colour = 0;
       }

    if (j >= 150 && j <= 180)
       {
        R_colour = 255;
        G_colour = 235 + (20.0/30.0) * (j - 150.0);
        B_colour = 0 + 255.0/30.0 * (j - 150.0);
       }
}

//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected()
   {
    Wire.beginTransmission((uint8_t)MLX90640_address);
    if (Wire.endTransmission() != 0){return (false);}
    return (true);
   }   

// 绘制十字
void draw_cross(int x, int y, int len){
   tft.drawLine(x - len/2, y, x + len/2, y, tft.color565(255, 255, 255));
   tft.drawLine(x, y-len/2, x, y+len/2,  tft.color565(255, 255, 255));
}

// 热成像读取多任务
void task_mlx(void * ptr){
   Wire.begin(MLX_SDA, MLX_SCL); 
   pinMode(MLX_VDD, OUTPUT);
   digitalWrite(MLX_VDD, LOW);
   vTaskDelay(1000);
   Wire.setClock(800000); //Increase I2C clock speed to 400kHz

   Serial.println("MLX90640 IR Array Example");

   if (isConnected() == false){
      while (1){
         Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
         delay(1000);
         };
   }Serial.println("MLX90640 online!");
   int status;
   uint16_t eeMLX90640[832];
      
   status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
   if (status != 0)
         Serial.println("Failed to load system parameters");

   status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
   if (status != 0)
   {
      Serial.println("Parameter extraction failed");
      Serial.print(" status = ");
      Serial.println(status);
   }
   //Once params are extracted, we can release eeMLX90640 array
      MLX90640_I2CWrite(0x33, 0x800D, 6401);    // writes the value 1901 (HEX) = 6401 (DEC) in the register at position 0x800D to enable reading out the temperatures!!!
   // ===============================================================================================================================================================
   //MLX90640_SetRefreshRate(MLX90640_address, 0x00); //Set rate to 0.25Hz effective - Works
   //MLX90640_SetRefreshRate(MLX90640_address, 0x01); //Set rate to 0.5Hz effective - Works
   //MLX90640_SetRefreshRate(MLX90640_address, 0x02); //Set rate to 1Hz effective - Works
   //MLX90640_SetRefreshRate(MLX90640_address, 0x03); //Set rate to 2Hz effective - Works
   //MLX90640_SetRefreshRate(MLX90640_address, 0x04); //Set rate to 4Hz effective - Works
   MLX90640_SetRefreshRate(MLX90640_address, 0x05); //Set rate to 8Hz effective - Works at 800kHz
   //MLX90640_SetRefreshRate(MLX90640_address, 0x06); //Set rate to 16Hz effective - fails
   //MLX90640_SetRefreshRate(MLX90640_address, 0x07); //Set rate to 32Hz effective - fails

   // MLX主循环
   for(;power_on==true;){
      lock = true;
      for (byte x = 0 ; x < 2 ; x++){
         uint16_t mlx90640Frame[834];
         int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
      
         if (status < 0){
            Serial.print("GetFrame Error: ");
            Serial.println(status);
            }
         float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
         float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

         float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
         float emissivity = 0.95;

         MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
      }
         // determine T_min and T_max and eliminate error pixels
         // ====================================================

      // mlx90640To[1*32 + 21] = 0.5 * (mlx90640To[1*32 + 20] + mlx90640To[1*32 + 22]);    // eliminate the error-pixels
      // mlx90640To[4*32 + 30] = 0.5 * (mlx90640To[4*32 + 29] + mlx90640To[4*32 + 31]);    // eliminate the error-pixels
      
      lock = false;
      T_min = mlx90640To[0];
      T_max = mlx90640To[0];
      T_avg = mlx90640To[0];
      for (i = 1; i < 768; i++){
         if((mlx90640To[i] > -41) && (mlx90640To[i] < 301))
            {
               if(mlx90640To[i] < T_min)
                  {
                  T_min = mlx90640To[i];
                  }

               if(mlx90640To[i] > T_max)
                  {
                  T_max = mlx90640To[i];
                  max_x = i / 32;
                  max_y = i % 32;
                  }
            }
         else if(i > 0)   // temperature out of range
            {
               mlx90640To[i] = mlx90640To[i-1];
            }
         else
            {
               mlx90640To[i] = mlx90640To[i+1];
            }
            T_avg = T_avg + mlx90640To[i];
         }
      T_avg = T_avg / 768;
      vTaskDelay(20);
   }
   vTaskDelete(NULL); 
}

// 关机
void power_off(){
   esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1);
   for(int i=512; i>0; i--){
      ledcWrite(0, i);
      vTaskDelay(2);
   }
   
   power_on = false;
   digitalWrite(MLX_VDD, LOW);
   ledcWrite(0, 0);
   digitalWrite(SCREEN_BL_PIN, LOW);
   vTaskDelay(1000);
   // esp_deep_sleep_start();
   vTaskDelete(NULL); 
}

// 背光调节
void set_brightness(uint16_t brightness){
   if (brightness < 1024){
      ledcWrite(0, brightness);
   }
}


// 平滑的开机
void task_smooth_on(void * ptr){
   ledcSetup(0, 3000, 10);
   ledcAttachPin(SCREEN_BL_PIN, 0);
   ledcWrite(0, 0);
   vTaskDelay(100);
   for(int i=0; i<512; i++){
      ledcWrite(0, i);
      vTaskDelay(2);
   }
   vTaskDelete(NULL); 
}


// 电池管理多任务
void task_bat(void * ptr){
   pinMode(BAT_ADC, INPUT);
   for(;power_on==true;){
      int adc_value = analogRead(BAT_ADC);
      bat_v = (float)adc_value / 4096 * 3.3 * 1.05;
      vTaskDelay(1000);
   }
   vTaskDelete(NULL);
}

void task_button(void * ptr){
   pinMode(buttonPin1, INPUT_PULLUP);
   pinMode(buttonPin2, INPUT);
   unsigned long start_time =  0;
   for(;power_on==true;){
      if (digitalRead(buttonPin1) == LOW){
         // if(buttonState1 == HIGH){
         //    // 检测下降沿
         //    vTaskDelay(5);
         //    if (digitalRead(buttonPin1) == LOW){start_time = millis();}
         // }
         if (millis() - start_time >= LONG_PUSH_T){
            power_off();
         }
      }else{
         start_time = millis();
      }
      buttonState1 = digitalRead(buttonPin1);
      buttonState2 = digitalRead(buttonPin2);
      vTaskDelay(100); 
   }
   vTaskDelete(NULL);
}

void task_screen_draw(void * ptr){
   for(;power_on==true;){
      touch.update();
      bool touched = touch.tp.touching;
      if( touched )
      {
         test_points[0][0] = touch.tp.x;
         test_points[0][1] = touch.tp.y;
      }
   // read the state of the pushbutton value:
   buttonState1 = digitalRead(buttonPin1);
   buttonState2 = digitalRead(buttonPin2);

      tft.setRotation(3);
      // drawing the picture
      if (!lock){
         for (i = 0 ; i < 24 ; i++){
            for (j = 0; j < 32; j++){
               mlx90640To[i*32 + j] = 180.0 * (mlx90640To[i*32 + j] - T_min) / (T_max - T_min);         
               getColour(mlx90640To[i*32 + j]);
               tft.fillRect(280 - j * _SCALE, (240 - _SCALE * 24) + i * _SCALE, _SCALE, _SCALE, tft.color565(R_colour, G_colour, B_colour));  
         }
      }
      }
      
      tft.setRotation(SCREEN_ROTATION);
      //  tft.setCursor(60, 220);
      //  tft.print(T_center, 1);

      tft.setCursor(60, 220);
      tft.printf("max: %.2f  ", T_max);
      tft.setCursor(60, 230);
      tft.printf("min: %.2f  ", T_min);
      
      tft.setCursor(150, 220);
      tft.printf("avg: %.2f  ", T_avg);
      tft.setCursor(150, 230);
      tft.printf("bat: %.2f v ", bat_v);

      tft.setTextColor(TFT_WHITE, TFT_BLACK); 
      tft.setCursor(155, 220);
      //  tft.print("C");
      
      //  tft.setCursor(80, 200);
      //  tft.printf("max_pos: %d, %d ", max_x, max_y);
      //  tft.setCursor(80, 210);
      //  tft.printf("touch_state: %d, %d     ",test_points[0][0], test_points[0][1]);
      tft.setCursor(80, 210);
      tft.printf("button: %d, %d     ",buttonState1, buttonState2);
      tft.drawCircle(test_points[0][1], 240 - test_points[0][0], 15, tft.color565(255, 255, 255));
      draw_cross(test_points[0][1], 240 - test_points[0][0], 10);
      delay(20);
   }
   vTaskDelete(NULL);
}

void setup(void)
 {
   Serial.begin(115200);
   touch.begin();
   // 按钮启用


   pinMode(SCREEN_BL_PIN, OUTPUT);
   
   xTaskCreate(task_mlx, "MLX_FLASHING", 1024 * 6, NULL, 1, NULL);
   xTaskCreate(task_bat, "BAT_MANAGER", 1024, NULL, 2, NULL);

   
   tft.init();
   tft.setRotation(SCREEN_ROTATION);
   tft.fillScreen(TFT_BLACK);
   xTaskCreate(task_screen_draw, "SCREEN", 1024 * 8, NULL, 2, NULL);
   xTaskCreate(task_smooth_on, "SMOOTH_ON", 1024, NULL, 2, NULL);
   xTaskCreate(task_button,    "BUTTON", 1024, NULL, 2, NULL);
   // draw the colour-scale
}

void loop() 
{
 vTaskDelay(3000);
}
