/* Interface LoRa internet
 *  This sketch aims to implement an interface between a LoRa communication channel with device connected to internet.
 *  The sketch must be loaded on a ESP8266 based board, such as a NodeMCU v1 or a Wemos D1 R2 boards capable of connection to a WiFi access point.
 *  On the other side, this board acts as a master which controls a LoRa SX1278 board as a slave. 
 *  The SX1278 board transfers data at 433MHz using LoRa modulation protocol.
 *  Between the LoRa terminals, communication is implemented using a one-byte addressing scheme,  with 0xFF as the broadcast address.
 *  The ESP8266 connects to an access point as a server and waits for the connection of a client.
 *  Once a client is connected, this device only resends the messages received by the client to the other LoRa terminal, identified with a specific address.
 *  The messages received from the LoRa terminal are sent to the client connected via internet.
 *  The connection between the ESP8266 board and the SX1278 is 
ESP8266 board Pins        SX1278 Pins
GND                           GND
3.3V                          VCC
D8                            NSS
D7                            MOSI
D6                            MISO
D5                            SCK
D0                            RST
D2                            DIO0

The messages sent and received can be sentences (as a chat room net) or determined commands to execute an action on the remote LoRa terminal.
The commands considered in this preliminary project to be executed by the remote LoRa terminal are
/LED=ON  : puts on the led and sends the message "led is on"
/LED=OFF  : puts off the led and sends the message "led is off"
/STATEBUTTON  : sends the messages "button pressed" or "button unpressed" according to the state of the button.
/ANALOGINPUT  : sends the message "analog input=xxx", where xxx is the value of the analog input (0-1023).
/STOPCLIENT  : message sent by the internet client to this interface to stop the internet connection. It is not resent to the remote terminal.
Every message received is displayed on the serial monitor of the master board, supposing it is connected to a computer. The messages considered include other sentences sent by 
the client and the LoRa terminal. Of course, connection to a computer is not mandatory because, once the sketch is stored on the master board, it is autonomous.

 Copyright: Fernando Pazos
 22/08/2024
 */


#include <ESP8266WiFi.h> 
#include <SPI.h>
#include <LoRa.h>

const char* ssid = "Helena 2.4Ghz";    //"Helena-2G";  //      //variable que almacena el nombre de la red wifi a la que el nodemcu se va a conectar
const char* password = "cenoura04";   //"DaniyFercenoura04";  //     //variable que almacena la seña de la red wifi donde el nodemcu se va a conectar
 
//DEFINICIÓN DE IP FIJO PARA EL NODEMCU
  IPAddress ip(192,168,1,61);    //COLOQUE UNA FRANJA DE IP DISPONIBLE EN SU ROTEADOR. Ej: EX: 192.168.1.110 **** ESO VARÍA, EN MI CASO ES: 192.168.0.200
  IPAddress gateway(192,168,1,1);  //GATEWAY DE CONEXIÓN (ALTERE PARA EL GATEWAY DE SU ROTEADOR)
  IPAddress subnet(255,255,255,0); //MASCARA DE RED
 
//port forwarding 192:168:1:61 => port 120
  //variables servidor y cliente
  WiFiServer servidor(120);
  WiFiClient cliente;

//LoRa pins
#define ss 15
#define rst 16
#define clk 14
#define dio0 4

//addresses defined in order to identificate the terminals connected
//This bytes must be the first one of all the messages sent and received, respectively
byte localAddress = 0xFF;            // address of this device
byte destinationAddress = 0xBB;      // destination to send to


////////////////////////////////////////////////////////////////////////////

void setup() {
  // put your setup code here, to run once:
  //inicia comunicación serie con el monitor
  Serial.begin(115200);
  while (!Serial);
  delay(50);

  //manda al monitor serie el nombre de la red a la cual se conectará
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Configuramos el ESP8266 como estación WiFi y no como punto de acceso. 
   *  Si no lo hacemos  se configurará como cliente y punto de acceso al mismo tiempo */
   WiFi.mode(WIFI_STA); // Modo estación WiFi

  // Conectamos a la red WiFi
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet); //PASSA OS PARÂMETROS PARA A FUNÇÃO QUE VAI SETAR O IP FIXO

  // Esperamos a que estemos conectados a la red WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Iniciar el servidor
  servidor.begin();
  Serial.println("Server started");
  Serial.println("");
  Serial.println("WiFi connected"); 
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // Mostramos la IP que en este caso debe ser fija
  Serial.println("");
  
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println();
  
  //connects to the LoRa slave device
  //initialize SX1278 at 433MHz
  Serial.println("LoRa Sender");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    delay(100);
    //while (1);
  }
  // register the receive callback
  LoRa.onReceive(onReceive);
  // put the radio into receive mode, enabling the interrupt which is called every time a packet is received
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}


void loop() {
  // put your main code here, to run repeatedly:
  // listen for incoming clients
  cliente = servidor.available();
  //if there is a client connected to this server
  if (cliente) {
    Serial.println("Client connected");
    cliente.println("Welcome client");
    while (cliente.connected()) {
       //if there is a request from the client
       if (cliente.available()){
             String request=cliente.readString(); //Until('\r');       //read the client's request
             Serial.println("Message from the client: "+request);      //writes the request on the serial monitor
             SendMessage(request);                                     //and resends to the Lora terminal
             LoRa.receive();                                           // go back into receive mode
             if (request.indexOf("/STOPCLIENT") != -1) cliente.stop();     //desconnection request
             request="";}
    }
  }
}   


//function to send a message via LoRa at 433MHz with the destination address as first byte
void SendMessage(String outgoing){
  LoRa.beginPacket();                   // start packet
  LoRa.write(destinationAddress);       // add destination address
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}


//interrrupt function executed every time a message is incoming from the LoRa terminal
void onReceive(int packetSize) {
  // received a packet
  if (packetSize == 0) return;          // if there's no packet, return  , otherwise
  Serial.println("Received packet with "+String(packetSize)+ " bytes");
  byte addr=LoRa.read();
  if (addr!=localAddress){
    Serial.println("but it is not for me!");}
  else{
    // read packet
    String message="";
    //for (int i = 0; i < packetSize-1; i++) {
    while (LoRa.available()) {            // can't use readString() in callback, so
      message+=(char)LoRa.read();}
    Serial.println("Message received from the LoRa terminal: "+message);
    // print RSSI of packet
    //Serial.print(", with RSSI ");
    //Serial.println(LoRa.packetRssi());
    //and sends to the client, if it is connected 
    if (cliente) cliente.println(message);}
}
