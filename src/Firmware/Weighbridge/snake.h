#include "config.h"
#include <ArduinoJson.h>
#include "logging.h"
#include "mode.h"
#include "mqtt.h"

#include "text_utils.h"
#include "buttons.h"

extern Adafruit_SSD1306 display;

namespace SNAKE_MODE {


  struct coord{
    int x;
    int y;

    coord() : x(0), y(0){}
    coord(int _x, int _y) : x(_x), y(_y){}

    bool operator==(const coord &rhs) const {
      return x == rhs.x &&
             y == rhs.y;
    }

    bool operator!=(const coord &rhs) const {
      return !(rhs == *this);
    }
  };


  template <typename T>
  struct ring{

    int first;
    int last;
    int maxSize;
    int size;
    T* content;

    ring(int size) : first(0), last(0), maxSize(size), size(0){
      content = new T[size];
    }

    ~ring(){
      delete[] this->content;
    }

    void push(T element){
      if(size + 1 == maxSize){
        DEBUG_PRINTLN("RING IS FULL!");
      }
      content[last] = element;
      last = (last +1) % maxSize;
      size++;
    }

    T pop(){
      if(size == 0){
        DEBUG_PRINTLN("RING IS EMPTY");
      }
      size--;
      int out = first;
      first = (first + 1)%maxSize;
      return content[out];
    }

    T get(int i){
      if(i >= size){
        DEBUG_PRINTLN("OUT OF BOUNDS!");
        return T();
      }

      return content[(i + first)%maxSize];

    }

    bool next(int &c){
      if(c == last) return false;
      int next = (c + 1) % size;
      if(next == first) return false;
      c = next;
      return true;
    }

  };

  coord get_bounty_point(coord* head, ring<coord>* tail, int width, int height){
    coord bounty;
    do{
      again:
      bounty = coord(random(width), random(height));
      if(bounty == *head) continue;
      for(int i = 0; i < tail->size; ++i){
        if(tail->get(i) == bounty) goto again;
      }
      break;
    }while(true);

    return bounty;
  }

  byte gamestate, dir = 0;
  coord head, bounty;
  ring<coord> *tail;

  unsigned long gameTime;
  unsigned long lastTime;

  void game(){
    int width = OLED_WIDTH / 4;
    int height = OLED_HEIGHT / 4;
    
    if(gamestate == 10){
      head = coord(width/2 +2, height/2);

      DEBUG_PRINTLN("!");
      tail = new ring<coord>(width*height);

      DEBUG_PRINTLN("RING CREATE");

      tail->push(coord(width/2 -1, height/2));
      tail->push(coord(width/2 +0, height/2));
      tail->push(coord(width/2 +1, height/2));
      tail->push(coord(width/2 +2, height/2));

      bounty = get_bounty_point(&head, tail, width, height); // coord(random(width), random(height));

      lastTime = gameTime = millis();
      dir = 0;
      DEBUG_PRINTLN("GAME START");
      gamestate = 12;
    }

    if(gamestate == 18){
      gameover:
      DEBUG_PRINTLN("GAME OVER!");
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("GameOver");
      display.setCursor(16, 16);
      display.printf("Score: %d", tail->size);
      display.display();
      
      StaticJsonBuffer<JSON_OBJECT_SIZE(4)> jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();

      json["time_played"] = millis() - gameTime;
      json["score"] = tail->size;

      char message[json.measureLength() + 1];
      json.printTo(message, sizeof(message));
      publish(MQTT_SNAKE_STATE, message);
      
      delay(1000);

      gamestate = 20;
      delete tail;
      return;
    }

    if(getButtonUpState() == HIGH && dir % 2 == 0) dir = 1;
    if(getButtonDownState() == HIGH && dir % 2 == 0) dir = 3;
    if(getButtonRightState() == HIGH && dir % 2 == 1) dir = 0;
    if(getButtonLeftState() == HIGH && dir % 2 == 1) dir = 2;

    if(millis() - lastTime < 200){
      return;
    }


    lastTime = millis();

    coord next = dir % 2 == 0 ? coord(head.x + (1 - dir), head.y)
                                : coord(head.x, head.y - (2 - dir));

    if(next.x < 0 || next.x >= width){
      // game over
      gamestate = 18;
      return;
    }
    if(next.y < 0 || next.y >= height){
      gamestate = 18;
      return;
    }

    // check if "next" is bounty
    if(next != bounty){
      tail->pop();
    }else{
      bounty = get_bounty_point(&head, tail, width, height);
    }

    // check if bitten in tail:
    for(int i = 0; i < tail->size-1; ++i){ // ignore last
      if(tail->get(i) == next){
        gamestate = 18;
        return;
      }
    }

    //tail.insert(tail.begin(), head);
    tail->push(next);
    head = next;

    // RENDER:
    display.clearDisplay();
    for(int i = 0; i < tail->size; ++i){
      coord c = tail->get(i);

      display.fillRect(c.x*4, c.y*4, 3, 3, WHITE);
    }


    display.drawCircle(bounty.x*4+1, bounty.y*4+1, 1, WHITE);
    display.display();
  }

  

  void start_mode() {
    gamestate = 0;
  }

  void loop() {
    if(gamestate == 0){

      display.clearDisplay();
      print_text_centered_in_box(0, 0, OLED_WIDTH, OLED_HEIGHT, "SNAKE");
      print_text_centered_in_box(0, 50, OLED_WIDTH, 14, ">> Play >>", true);
      display.display();

      if (getButtonRightState() == HIGH){
        gamestate = 10;
      }
      return;
    }

    if(gamestate >= 10 && gamestate < 20){
      game();
      return;
    }
    if(gamestate == 20 ){
      delay(3000);
      switchMode(0);
    }
  }

  void setup() {
    addMode(&start_mode, &loop, (char *) "snake");
  }

}
