/* LoRa terminal
 *  Sketch to be loaded in the remote LoRa terminal as a slave controlled by an Arduino type board as a master.
 *  The Lora board is a SX1278, which exchanges data at 433MHz, moduling the data according to the LoRa protocol.
 *  The pinout connection is


NodeMCU Pins   or  Arduino pins       SX1278 Pins
GND                                       GND
3.3V                                      VCC
D8                   GPIO15               NSS
D7                   GPIO13               MOSI
D6                   GPIO12               MISO
D5                   GPIO14               SCK
D0                   GPIO16               RST
D2                   GPIO4                DIO0

In the case that the master board is an Arduino, have into account that it handles 5V input/output, whereas the SX1278 works with 3.3V, so the voltage levels must be adapted.
The communication from this LoRa terminal is with another SX1278 board which plays the role of a LoRa-internet interface. This interface transmits and receives data from a remote user 
connected to internet.
For communication with the interface, it Implements a one-byte addressing scheme,  with 0xBB as the broadcast address.
The Lora terminal receives commands from the LoRa-internet interface, and sends an answer according to this command.
In this preliminary test, the only commands considered are:
/LED=ON  : puts on the led and sends the message "led is on"
/LED=OFF  : puts off the led and sends the message "led is off"
/STATEBUTTON  : sends the messages "button pressed" or "button unpressed" according to the state of the button.
/ANALOGINPUT  : sends the message "analog input=xxx", where xxx is the value of the analog input read (0-1023).
Every message received is displayed on the serial monitor of the master board, supposing it is connected to a computer. The messages considered include other sentences sent by 
the user. Of course, connection to a computer is not mandatory because, once the sketch is stored on the master board, it is autonomous.
If a sentence is introduced via the serial monitor, it is sent via LoRa to the Lora-internet interface

 Copyright: Fernando Pazos
 22/08/2024
 
 */


#include <SPI.h>
#include <LoRa.h>

//pinout of the master board
#define ss 15
#define rst 16
#define dio0 4

//addresses defined in order to identificate the terminals connected
//One of these bytes must be the first one in all the messages sent and received, respectively
byte localAddress = 0xBB;            // address of this device
byte destinationAddress = 0xFF;      // destination to send to

//GPIO of the led and button on the nodeMCU board
#define LED_BUILTIN 2
#define BUTTON_BUILTIN 0


void setup() {
  // put your setup code here, to run once:
  //establece las entradas/salidas
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_BUILTIN,INPUT);

  Serial.begin(115200);
  while (!Serial);
  //initialize SX1278 at 433MHz
  Serial.println();
  Serial.println("LoRa Terminal");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    //while (1);
  }
  // register the receive callback
  LoRa.onReceive(onReceive);
  // put the radio into receive mode, enabling the interrupt which is called every time a packet is received
  LoRa.receive();
  Serial.println("LoRa init succeeded.");

}

String request="";

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()>0){              //sends to the other terminal a message introduced by the serial monitor
    String messerver=Serial.readString();
    SendMessage(messerver);
    LoRa.receive();                       // go back into receive mode
  }
  if (request!="") {                      //if a command was sent, it is executed
    executeCommand(request);
    request="";
  }
}

//function to send a message via LoRa at 433MHz with the destination address as first byte
void SendMessage(String outgoing){
  LoRa.beginPacket();                   // start packet
  LoRa.write(destinationAddress);       // add destination address
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

//interrupt function executed every time a packet is incoming
void onReceive(int packetSize) {
  // received a packet
  if (packetSize == 0) return;          // if there's no packet, return  , otherwise
  Serial.println("Received packet with "+String(packetSize)+ " bytes");
  byte addr=LoRa.read();
  if (addr!=localAddress){
    Serial.println("but it is not for me");}
  else{
    // read packet
    //String request="";
    for (int i = 0; i < packetSize-1; i++) {
    //while (LoRa.available()) {            // can't use readString() in callback, so
      request+=(char)LoRa.read(); }         //reads the incoming request
    Serial.println("Received message: "+request);
    // print RSSI of packet
    //Serial.print(", with RSSI: ");
    //Serial.println(LoRa.packetRssi());
    //executeCommand(request);
    }
}

//function that execute the command received
//messages sent by the internet client have a /r/n final characters
void executeCommand(String command) {
  if (command.indexOf("/LED=ON") != -1)  {          //puts on the led
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("led ligado");
    SendMessage("led is on");
    }

  if (command.indexOf("/LED=OFF") != -1)  {         //puts off the led
    digitalWrite(LED_BUILTIN, HIGH); 
    Serial.println("led desligado");
    SendMessage("led is off");
    }
  
  if (command.indexOf("/STATEBUTTON") != -1){
    int estado = digitalRead(BUTTON_BUILTIN);       //reads the button status
    if (estado) {
        Serial.println("botão não pressionado");
        SendMessage("botton unpressed");
        }        
      else {
        Serial.println("botão pressionado");
        SendMessage("button pressed");
        }
    }         
  
  if (command.indexOf("/ANALOGINPUT") != -1){       //reads the analog input
    int value = analogRead(A0);
    Serial.printf("analog input= %d\r\n",value);
    SendMessage("analog input: " + String(value));
    }
  LoRa.receive();                       // go back into receive mode
}

