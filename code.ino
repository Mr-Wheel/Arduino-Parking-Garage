#include "SR04.h"                 //für Ultrasonic Proximity Sensor
#include <Servo.h>                //für Servo = Schranke
#include <LiquidCrystal.h>        //für LCD

//Ultrasonic Proximity Sensor HC-SR04
int trig_pin=4;
int echo_pin=3;
SR04 proximitySensor = SR04(echo_pin,trig_pin); 

//PIR Motion Sensor HC-SR501 (in H-Modus=Hold/Repeat/Retriggering-->HIGH-Output solange Bewegung wahrgenommen wird)
int pirPin=13; 


//Schranke
Servo myServo;
int servoDelay=3000;               //3s
int manServoDelay = 9000;          //9s bei Betätigen des Interrupt-Buttons
int servoMax=90;                   //Schranke öffnet sich zu 90°
int servoMin=0;                    //Schranke zu

//LCD
LiquidCrystal lcd(7,8,9,10,11,12); //initialize the library with the numbers of the interface pins

//Interrupt-Button
int interruptButton = 2;

//Button zum manuellen Überschreiben der freien Plätze
int manCapacityButton = 6;

//Variabeln
long distance;                     //von Proximity Sensor gemessene Distanz
int pirValue;                      //Wert des Motion Sensors: 1=Bewegung wahrgenommen, 0=keine Bewegung wahrgenommen
int capacity;                      //Freie Plätze im Parkhaus
int manCapacity;                   //manuell festgelegte Anzahl an freien Plätzen
volatile int interruptFlag;        //Flag, die auf 1 gesetzt wird, wenn Interrupt-Button gedrückt wird



/******************************************************************************************************
************************************************SETUP**************************************************
******************************************************************************************************/
void setup() {

  //Serial.begin(9600);            //bei Bedarf: Ausgabe der gemessenen Distanz des Ultrasonic Sensors im seriellen Monitor zum Debugging (s.u.)

  //PIR Motion Sensor
  pinMode(pirPin, INPUT);
  
  //Schranke
  myServo.attach(5);               //Servo an Pin5
  myServo.write(servoMin);         //Schranke schließen

  //LCD
  lcd.begin(16, 2);                //set up the LCD's number of columns and rows
  lcd.print("Freie Plaetze:");
  capacity = 5;                    //Freie Plätze auf 5 initialisieren
  lcd.setCursor(0, 1);             //2. Zeile
  lcd.print(capacity);            

  //Interrupt
  pinMode(2,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptButton),schrankeAuf,RISING); //Button an Pin2 ist als Interrupt Button definiert, ruft schrankeAuf() auf,
                                                                              //wenn sich state von LOW zu HIGH verändert
  interruptFlag = 0;               //interruptFlag auf 0 initialisieren

  //Button zum manuellen Anpassen der freien Plätze
  pinMode(manCapacityButton, INPUT_PULLUP);
}

/******************************************************************************************************
*************************************************LOOP**************************************************
******************************************************************************************************/
void loop() {
  
  /**************************************
   EINFAHRT (Ultrasonic Proximity Sensor)
  **************************************/
  distance = proximitySensor.Distance();     //Distanz messen 
  //Serial.println(distance);                //bei Bedarf: Ausgabe der gemessenen Distanz des Ultrasonic Sensors im seriellen Monitor zum Debugging

  if (distance<5){                           //steht ein Auto da?
    if (capacity>0){                         //wenn noch Platz frei ist
      while (distance<5){                    //während das Auto noch vor der Schranke steht
        myServo.write(servoMax);             //Schranke öffnen bzw. offen lassen
        delay(servoDelay);                   //warten bis Auto eingefahren ist
        distance=proximitySensor.Distance(); //erneut Distanz messen
        //Serial.println(distance);            //zur Kontrolle im seriellen Monitor ausgeben
      }
      capacity = capacity - 1;               //Counter runterzählen
      myServo.write(servoMin);               //Schranke schließen
    } 
  }

  
  /**************************************
   AUSFAHRT (PIR Motion Sensor)
  **************************************/
  pirValue = digitalRead(pirPin);            //erkennt der Sensor eine Bewegung? 1=ja,0=nein
  
  if(pirValue == 1){                         //steht ein Auto da?
    while(pirValue == 1){                    //während das Auto noch vor der Schranke steht
      myServo.write(servoMax);               //Schranke öffnen bzw. offen lassen
      delay(servoDelay);                     //warten bis Auto eingefahren ist
      pirValue = digitalRead(pirPin);        //erkennt der Sensor immer noch eine Bewegung?
    }
    if(capacity<5){                          //wenn z.Zt. weniger als 5 Plätze frei sind
      capacity = capacity + 1;               //Counter erhöhen
    }
    myServo.write(servoMin);                 //Schranke schließen
  }


  /**************************************
   LCD-Anzeige
  **************************************/
  lcd.setCursor(0, 1);                       //2.Zeile
  lcd.print(capacity);                       //Update der freien Plätze auf dem LCD


  /**************************************
   Routine bei interruptFlag == 1
  **************************************/
  if (interruptFlag == 1){                   //wenn Interrupt-Button gedrückt wurde       
    myServo.write(servoMax);                 //Schranke öffnen
    lcd.clear();                             //Anzeige auf LCD entfernen
    lcd.print("manuell offen:");             //Ausgabe in 1. Zeile
    lcd.setCursor(1, 1);                     //2.Zeile, 2.Zelle
    lcd.print("s");                          //für Sekunden

    //9s-Countdown
    for (int i = (manServoDelay/1000); i>0;i=i-1){ 
      lcd.setCursor(0, 1);
      lcd.print(i); 
      delay(1000); 
    }

    //manCapacity Button
    
    //neue LCD-Ausgabe + 5s-Countdown
    lcd.clear();
    lcd.print("Freie Plaetze");
    lcd.setCursor(0, 1); //2. Zeile
    lcd.print("anpassen?");
    lcd.setCursor(12, 1);
    lcd.print("s"); 
    for (int i=5;i>0; i=i-1){
      lcd.setCursor(11, 1);
      lcd.print(i); 
      delay(1000);
      digitalRead(manCapacityButton);
      
      if (digitalRead(manCapacityButton) == LOW){       //wenn innerhalb des Countdowns der manCapacityButton gedrückt wird
        manCapacity = 0;
        lcd.clear();
        lcd.print("Freie Plaetze:");
        lcd.setCursor(0, 1); //2. Zeile
        while(digitalRead(manCapacityButton) == LOW){   //solange manCapacityButton gedrückt halten bis gewünschte Anzahl an freien Plätzen angezeigt wird
          lcd.setCursor(0, 1);
          lcd.print(manCapacity);                       
          delay(1000);                                  
          manCapacity++;                                //solange manCapacityButton gedrückt wird, erhöht sich jede Sekunde manCapacity um 1
          if (manCapacity > 5) {                        //mehr als 5 freie Plätze können auch manuell nicht festgelegt werden
            break;
          }
          digitalRead(manCapacityButton);               //wird manCapacityButton noch immer gedrückt? --> Ausstiegsbedingung der while-Schleife
        }
        
        break;                                          //Ausstieg aus der if-Schleife, auch wenn manCapacityButton noch immer gedrückt
      }
    }

    myServo.write(servoMin);                            //Schranke schließen
    lcd.clear();
    lcd.print("Freie Plaetze:");
    lcd.setCursor(0, 1); //2. Zeile
    if (manCapacity >= 0) {                             //wenn manCapacity manuell gesetzt wurde
      capacity = manCapacity - 1;                       //da Schranke beim manuellen Öffnen auf ist, wird danach der Counter um 1 erhöht --> 1 von manCapacity abziehen
    }
    interruptFlag = 0;                                  //interruptFlag auf 0 zurücksetzen, Ende der durch den Interrupt ausgelösten Routine
  }
}


/******************************************************************************************************
**********************************************INTERRUPT************************************************
******************************************************************************************************/
void schrankeAuf(){
  interruptFlag = 1;       
}                            

