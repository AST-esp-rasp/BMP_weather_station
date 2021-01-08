
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
#define TFT_GREY 0x5AEB
#define M_SIZE 1.3333
float ltx = 0;
float ltx1[6] = {0, 0, 0, 0, 0, 0};    // Saved x coord of bottom of needle
uint16_t osx = M_SIZE*120, osy = M_SIZE*120; // Saved x & y coords
uint16_t osx1[6] = {0, 0, 0, 0, 0, 0}, osy1[6] = {0, 0, 0, 0, 0, 0}; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update
int Y_offset = 168;

int old_analog =  -999; // Value last displayed
int old_analog1[6] =  {0, 0, 0, 0, 0, 0}; // Value last displayed
int value[6] = {0, 0, 0, 0, 0, 0};
int value1[6] = {0, 0, 0, 0, 0, 0};
int value2;
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int old_value1[6] = { -1, -1, -1, -1, -1, -1};
int d = 0;
void draw_scale(float r_max,int xoffset,int yoffset){
   
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    uint16_t y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;
    uint16_t x1 = sx * M_SIZE*100 + M_SIZE*120;
    uint16_t y1 = sy * M_SIZE*100 + M_SIZE*140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (M_SIZE*100 + tl) + M_SIZE*120;
    int y2 = sy2 * (M_SIZE*100 + tl) + M_SIZE*140;
    int x3 = sx2 * M_SIZE*100 + M_SIZE*120;
    int y3 = sy2 * M_SIZE*100 + M_SIZE*140;

    // Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
    //  tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    //}

    // Green zone limits
    if (i >= -50 && i < -40) {
      tft.fillTriangle(x0 + xoffset, y0 + yoffset, x1 + xoffset, y1 + yoffset, x2 + xoffset, y2 + yoffset, TFT_RED);
      tft.fillTriangle(x1 + xoffset, y1 + yoffset, x2 + xoffset, y2 + yoffset, x3 + xoffset, y3 + yoffset, TFT_RED);
    }
    if (i >= -40 && i < -10) {
      tft.fillTriangle(x0 + xoffset, y0 + yoffset, x1 + xoffset, y1 + yoffset, x2 + xoffset, y2 + yoffset, TFT_YELLOW);
      tft.fillTriangle(x1 + xoffset, y1 + yoffset, x2 + xoffset, y2 + yoffset, x3 + xoffset, y3 + yoffset, TFT_YELLOW);
    }
    // Orange zone limits
    if (i >= -10 && i < 50) {
      tft.fillTriangle(x0 + xoffset, y0 + yoffset, x1 + xoffset, y1 + yoffset, x2 + xoffset, y2 + yoffset, TFT_GREEN);
      tft.fillTriangle(x1 + xoffset, y1 + yoffset, x2 + xoffset, y2 + yoffset, x3 + xoffset, y3 + yoffset, TFT_GREEN);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;
    x1 = sx * M_SIZE*100 + M_SIZE*120;
    y1 = sy * M_SIZE*100 + M_SIZE*140;

    // Draw tick
    tft.drawLine(x0 + xoffset, y0 + yoffset, x1 + xoffset, y1 + yoffset, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE*100 + tl + 10) + M_SIZE*120;
      y0 = sy * (M_SIZE*100 + tl + 10) + M_SIZE*140;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0 + xoffset, y0 - 12 + yoffset, 2); break;
        case -1: tft.drawCentreString(String(r_max/4,1), x0 + xoffset, y0 - 9 + yoffset, 2); break;
        case 0: tft.drawCentreString(String(r_max/2,1), x0 + xoffset, y0 - 7 + yoffset, 2); break;
        case 1: tft.drawCentreString(String(r_max/4*3,1), x0 + xoffset, y0 - 9 + yoffset, 2); break;
        case 2: tft.drawCentreString(String(r_max,1), x0 + xoffset, y0 - 12 + yoffset, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * M_SIZE*100 + M_SIZE*120;
    y0 = sy * M_SIZE*100 + M_SIZE*140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0 + xoffset, y0 + yoffset, x1 + xoffset, y1 + yoffset, TFT_BLACK);
  }
}
// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter(int x_offset,int y_offset,float range_max,String unit_bt_right, String Name_of_Gauge)
{

  // Meter outline

  tft.fillRect(0+x_offset, 0+y_offset, M_SIZE*239, M_SIZE*126, TFT_GREY);
  tft.fillRect(5+x_offset, 3+y_offset, M_SIZE*230, M_SIZE*119, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
draw_scale(range_max,x_offset,y_offset);

  tft.drawString(unit_bt_right, M_SIZE*(5 + 230 - 40) + x_offset, M_SIZE*(119 - 20) + y_offset, 4); // Units at bottom right
  tft.drawCentreString(Name_of_Gauge, M_SIZE*120 + x_offset, M_SIZE*70 + y_offset, 4); // Comment out to avoid font 4
  tft.drawRect(5 + x_offset, 3 + y_offset, M_SIZE*230, M_SIZE*119, TFT_BLACK); // Draw bezel line


}



// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################


void plotNeedle1( byte ms_delay, int xoffset, int yoffset, String NameofGauge, int ch)
{
//  float sx1[6]= {0, 0, 0, 0, 0, 0};
  float sx1;
  float sy1;
  float tx1;
  float sdeg1;
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8];
  if ((ch==0)||(ch==2)) value2=value1[ch]*60;
  if (ch==1)value2=value1[ch];
  dtostrf(value2, 4, 0, buf);
  
  tft.drawRightString(buf, M_SIZE*40+ xoffset, M_SIZE*(119 - 20) + yoffset, 4);
  if (value1[ch] < -10) value1[ch] = -10; // Limit value to emulate needle end stops
  if (value1[ch] > 110) value1[ch] = 110;

  // Move the needle until new value reached
//  while (!(value1[ch] == old_analog1[ch])) {
    if (old_analog1[ch] < value1[ch]) old_analog1[ch]++;
    else old_analog1[ch]--;

    if (ms_delay == 0) old_analog1[ch] = value1[ch]; // Update immediately if delay is 0

    sdeg1 = map(old_analog1[ch], -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    sx1 = cos(sdeg1 * 0.0174532925);
    sy1 = sin(sdeg1 * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    tx1 = tan((sdeg1 + 90) * 0.0174532925);

    // Erase old needle image
    
    
    tft.drawLine(M_SIZE*(120 + 20 * ltx1[ch] - 1)+xoffset, M_SIZE*(140 - 20) + yoffset, osx1[ch] - 1 + xoffset, osy1[ch] + yoffset, TFT_WHITE);
    tft.drawLine(M_SIZE*(120 + 20 * ltx1[ch]) + xoffset, M_SIZE*(140 - 20) + yoffset, osx1[ch] + xoffset, osy1[ch] + yoffset, TFT_WHITE);
    tft.drawLine(M_SIZE*(120 + 20 * ltx1[ch] + 1) + xoffset, M_SIZE*(140 - 20) + yoffset, osx1[ch] + 1 + xoffset, osy1[ch] + yoffset, TFT_WHITE);
    // Re-plot text under needle
    tft.setTextColor(TFT_BLACK);
    
    tft.drawCentreString(NameofGauge, M_SIZE*120 + xoffset, M_SIZE*70 + yoffset, 4); // // Comment out to avoid font 4
    // Store new needle end coords for next erase
    ltx1[ch] = tx1;
    osx1[ch] = M_SIZE*(sx1 * 98 + 120);
    osy1[ch] = M_SIZE*(sy1 * 98 + 140);

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    
    tft.drawLine(M_SIZE*(120 + 20 * ltx1[ch] - 1) + xoffset, M_SIZE*(140 - 20) + yoffset, osx1[ch] - 1 + xoffset, osy1[ch] + yoffset, TFT_RED);
    tft.drawLine(M_SIZE*(120 + 20 * ltx1[ch]) + xoffset, M_SIZE*(140 - 20) + yoffset, osx1[ch]+xoffset, osy1[ch] + yoffset, TFT_MAGENTA);
    tft.drawLine(M_SIZE*(120 + 20 * ltx1[ch] + 1) + xoffset, M_SIZE*(140 - 20) + yoffset, osx1[ch] + 1 + xoffset, osy1[ch] + yoffset, TFT_RED);
    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog1[ch] - value1[ch]) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
//    delay(ms_delay);
//  }
}
