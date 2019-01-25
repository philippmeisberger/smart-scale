//
// Created by oliver on 05.12.18.
//

#ifndef SMART_SCALE_TEXT_UTILS_H
#define SMART_SCALE_TEXT_UTILS_H

extern Adafruit_SSD1306 display;

// each chracter is 5x8 px with 1px horizontal spacing

/**
 * @param x - horizontal offset for the cursor
 * @param y - vertical offset for the cursor
 * @param width - text-box width
 * @param height - text-box height
 * @param text - the text to be printed
 * @param verticalScale - weather the scale should use width or height for size
 */
void print_text_centered_in_box(int x, int y, int width, int height, String text, bool verticalScale = false){
  int dx = 0, dy = 0, s = 1;
  if(! verticalScale){
    int l = text.length() * (5+1) - 1;
    s = width / l;
    dx = (width - (l * s)) * 0.5f;
    dy = (height - (8)*s) * 0.5f;
  }else{
    s = height / 8;
    int l = text.length() * (5+1) - 1;
    dx = (width - (l * s)) * 0.5f;
    dy = (height - (8)*s) * 0.5f;
  }
  display.setCursor(x + dx, y + dy);
  display.setTextSize(s);
  display.println(text);
}

#endif //SMART_SCALE_TEXT_UTILS_H
