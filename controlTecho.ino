#include <SPI.h>
#include <Ethernet.h>
#include <avr/interrupt.h>
#define MaxHeaderLength 25    //maximum length of http header required


/////////  INICIO seccion WEBSERVER

// (gateway) 192.168.1.1
// arduino (webserver) 192.168.0.116 -- encontrada al conectar la eth shield....
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0E, 0xD0, 0x58 };  // MAC especifica a 'esta' arduino Ethernet Shield
byte ip[] = { 
  146,155, 21, 223 };    // 192.168.0.116 en Santa Martina, 146.155.21.223 en San Joaquin
byte gateway[] ={ 
  146, 155, 21, 1 };    // 146.155.21.1
EthernetServer server(80);
String HttpHeader = String(MaxHeaderLength); 


/////////  INICIO seccion RELAYS 
int switchPin1 = 2;   // switch button input
int switchPin2 = 3;   // switch button input
int relayPin1 = 7;   // IN1 connected to digital pin 7
int relayPin2 = 8;   // IN2 connected to digital pin 8
int relayLed1 = 4;   
int relayLed2 = 6;   



volatile int val1 = 0;    // el valor recogido en el pin de entrada
volatile int buttonState1 = 0;  // variable to hold the last button state

/// arduino debounce interrupts http://www.instructables.com/id/Arduino-Software-debouncing-in-interrupt-function/
long debouncing_time = 400; //Debouncing Time in Milliseconds
volatile unsigned long last_micros1;
volatile unsigned long last_micros2;

int techoStatus = 0; 

void initWebServer()
{
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  HttpHeader="";
}

void webServerDaemon() 
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //read MaxHeaderLength number of characters in the HTTP header
        //discard the rest until \n
        if (HttpHeader.length() < MaxHeaderLength)
        {
          //store characters to string
          HttpHeader = HttpHeader + c;
        }        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          int api = HttpHeader.indexOf("/api/");
          String apiCallback;           
          if (api>0) {
            int switch1 = HttpHeader.indexOf("switch1");
            if (switch1>0) {
              int p1 = HttpHeader.indexOf("s=1");
              if (p1>=0) {
                digitalWrite(relayPin1, LOW);
                techoStatus=1;
                delay(2500);                    // this and next line for a 'momentary action'
                digitalWrite(relayPin1, HIGH);
                digitalWrite(relayLed1, HIGH); 
                techoStatus = 0;         // probando !...
                apiCallback = "callBack(1, 1);";                  
              } 
              else 
              {
                digitalWrite(relayPin1, HIGH);  
                digitalWrite(relayLed1, LOW);   
                techoStatus = 0;
                apiCallback = "callBack(1, 0);";
              }
            }   
          }          
          
          String classtechoStatus;


          if (techoStatus == 1){
            classtechoStatus = "EN ACCION";
          } 
          else {
            classtechoStatus = "EN ESPERA";
          }          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();  

          if (api>0) {
            client.println(apiCallback);
          }
          else {
            client.println("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>Eth+Arduino para abrir/cerrar techo ODUC 1</title>");
            client.println("<meta name=\"modificado-por\" content=\"rlaviles@gmail.com\">");
            client.println("<script src=\"http://code.jquery.com/jquery-1.10.1.min.js\"></script>");
            client.println("<script src=\"http://dl.dropboxusercontent.com/s/9j1m837hr3ol0z7/temp.js\"></script>");
            client.println("<link href=\"http://dl.dropboxusercontent.com/s/a24yjk5l1yjqlbg/temp.css\" rel=\"stylesheet\" type=\"text/css\" />");
            client.println("</head><body><div id=\"container\">");
            client.println("<div class=\"data transparent_class\"><table><tr><td class=\"value\">");
            char buffer[10];
;     
            client.println("<tr><td class=\"desc\">EL TECHO ESTA 'EN ESPERA' -- AL PULSAR EL SWITCH, EL RELE se activa por 2.5[s]</td></tr>");
            client.println("<tr><td class=\"desc\">activando el motor (techo 'en acción'); liberado el RELE es posible usar el botón</td></tr>");
            client.println("<tr><td class=\"desc\">VERDE o bien pulse para volver a 'EN ESPERA' Y LUEGO PULSE PARA la próxima ACCIÓN</td></tr>");
            client.println("<tr><td class=\"desc\">la cual será APAGAR EL MOTOR</td></tr>");
            client.println("</table></div>");
            
            client.println("</div>");
            
            client.println("<div id=\"footer\">");
              client.println("<div class=\"switch transparent_class ");
              client.println(classtechoStatus);
              client.println("\" id=\"switch1\">el techo esta <span>");
              client.println(classtechoStatus);
              client.println("</span></div>");

            client.println("</div>");
            client.println("</body></html>");          
          }
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    HttpHeader="";
    // close the connection:
    client.stop();
  }
}

/////////  FINAL PART WEBSERVER


void buttonRelaysInit() {
  pinMode(switchPin1, INPUT);
  pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
  pinMode(relayLed1, OUTPUT);      // sets the digital pin as output
  digitalWrite(relayPin1, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayLed1, LOW);
  buttonState1 = digitalRead(switchPin1);   // read the initial state
}

void processRelays()
{
  val1 = digitalRead(switchPin1);   // read input value and store it in val 

  if (val1 != buttonState1) {          // the button state has changed!
    if (val1 == HIGH) {                // check if the button is pressed
      if (techoStatus == 0){
          digitalWrite(relayPin1, HIGH);   // energizes the relay and lights the LEDx
          digitalWrite(relayLed1, LOW);
          techoStatus = 1;        
      } else {
          digitalWrite(relayPin1, LOW);   // energizes the relay and lights the LEDx
          digitalWrite(relayLed1, HIGH);
          techoStatus = 0;
      }       
    } else {                         // the button is -not- pressed...
    }
  }
  buttonState1 = val1;                 // save the new state in our variable        
}


void processRelay1()
{

  if (techoStatus == 0){
    digitalWrite(relayPin1, LOW);  
    techoStatus = 1;
  } 
  else {
    digitalWrite(relayPin1, HIGH);   
    techoStatus = 0;
  }        
}

/////////  FINAL PART RELAYS

void debounceInterrupt1() {
  if((long)(micros() - last_micros1) >= debouncing_time * 1000) {
    processRelay1();
    last_micros1 = micros();
  }
}


void setup() {                
  buttonRelaysInit();
  initWebServer();
  techoStatus =0;
}

void loop() {
  processRelays();
  webServerDaemon(); 
  delay(100);       
}

