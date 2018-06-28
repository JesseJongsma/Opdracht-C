// Include files.
#include <SPI.h>                  // Ethernet shield uses SPI-interface
#include <Ethernet.h>             // Ethernet library (use Ethernet2.h for new ethernet shield v2)
#include <Servo.h>

// Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0x42, 0x6c, 0x8f, 0x36, 0x84, 0x8a }; // Ethernet adapter shield
int ethPort = 3300;                                  // Take a free port (check your router)

EthernetServer server(ethPort);              // EthernetServer instance (listening on port <ethPort>).
Servo lever;

bool connected = false;                      // Used for retrying DHCP
bool leverOpen = true;
bool automatic = true;
bool first = false;

unsigned long beginCount = 0;

// constants won't change:
const long interval = 10000;

#define ledPin 8
#define trigPin 6
#define echoPin 5

void setup()
{  
   lever.attach(3);
   lever.write(90);
   pinMode(trigPin, OUTPUT);
   pinMode(echoPin, INPUT);
   Serial.begin(9600);
   //while (!Serial) { ; }               // Wait for serial port to connect. Needed for Leonardo only.

   pinMode(ledPin, OUTPUT);          // sets the digital pin ledPin as output

   Serial.println("Server started, trying to get IP...");

   //Try to get an IP address from the DHCP server.
   if (Ethernet.begin(mac) == 0)
   {
      Serial.println("Could not obtain IP-address from DHCP");
      Serial.println("Retry in 10 seconds");
      delay(10000);
      
      for (int i = 0; i < 100; i++) {
        if (Ethernet.begin(mac) != 0) {
          connected = true;
          return;
        } else {
          Serial.print("Try"); Serial.print(i); Serial.println(" failed");
          Serial.println("Retry in 10 seconds");
          delay(10000);
        }
      }

      if (!connected) {
        Serial.println("Could not get ip from DHCP, stopped retrying");
        
        while (true){     // no point in carrying on, so do nothing forevermore; check your router
          
        }
      }
   }
   
   Serial.print("LED (for connect-state and pin-state) on pin "); Serial.println(ledPin);
   Serial.println("Ethernetboard connected (pins 10, 11, 12, 13 and SPI)");
   Serial.println("Connect to DHCP source in local network (blinking led -> waiting for connection)");
   
   //Start the ethernet server.
   server.begin();

   // Print IP-address and led indication of server state
   Serial.print("Listening address: ");
   Serial.print(Ethernet.localIP());
   
   // for hardware debug: LED indication of server state: blinking = waiting for connection
   int IPnr = getIPComputerNumber(Ethernet.localIP());   // Get computernumber in local network 192.168.1.3 -> 3)
   Serial.print(" ["); Serial.print(IPnr); Serial.print("] "); 
   Serial.print("  [Testcase: telnet "); Serial.print(Ethernet.localIP()); Serial.print(" "); Serial.print(ethPort); Serial.println("]");
}

void loop()
{
   // Listen for incomming connection (app)
   EthernetClient ethernetClient = server.available();
   if (!ethernetClient) {
      return; // wait for connection
   }

   Serial.println("Application connected");
   digitalWrite(ledPin, HIGH);

   // Do what needs to be done while the socket is connected.
   while (ethernetClient.connected()) 
   {
      //sensorValue = readSensor(0, 100);         // update sensor value
   
      // Execute when byte is received.
      while (ethernetClient.available())
      {
        // Read client character
         char inByte = ethernetClient.read();   // Get byte from the client
         executeCommand(inByte);                // Wait for command to execute
         inByte = NULL;                         // Reset the read byte.
         break;
      }
      
      if(automatic)
      {
        leverDistance();
        
        if(!leverOpen)
        {
          keepClosed(1000);
        }
      }
      else
      {
        if(first)
        {
          keepClosed(10000);
        }
      }
   }
   digitalWrite(ledPin, LOW);
   Serial.println("Application disonnected");
}

// Implementation of (simple) protocol between app and Arduino
// Request (from app) is single char ('a', 's', 't', 'i' etc.)
// Response (to app) is 4 chars  (not all commands demand a response)
void executeCommand(char cmd)
{
         char buf[4] = {'\0', '\0', '\0', '\0'};

         // Command protocol
         Serial.print("["); Serial.print(cmd); Serial.print("] -> ");
         
         switch (cmd) {
          // Case a is example of value (int) to buf
         case 'a':    
            if (automatic) { automatic = false; }
            else { automatic = true;}
            break;
         case 't': // Report sensor value to the app  
            if (leverOpen) 
            { 
              lever.write(0); 
              leverOpen = false;
              first = true;
            }
            else { lever.write(90); leverOpen = true;}
            automatic = false;
            break;
         case 's':    
            if (leverOpen) { server.write("ON__"); Serial.println("ON"); }
            else { server.write("OFF_"); Serial.println("OFF");}
            break;
         default:
            break;
         }
}

// Convert int <val> char buffer with length <len>
void intToCharBuf(int val, char buf[], int len)
{
   String s;
   s = String(val);                        // convert tot string
   if (s.length() == 1) s = "0" + s;       // prefix redundant "0" 
   if (s.length() == 2) s = "0" + s;  
   s = s + "\n";                           // add newline
   s.toCharArray(buf, len);                // convert string to char-buffer
}

// Convert IPAddress tot String (e.g. "192.168.1.105")
String IPAddressToString(IPAddress address)
{
    return String(address[0]) + "." + 
           String(address[1]) + "." + 
           String(address[2]) + "." + 
           String(address[3]);
}

// Returns B-class network-id: 192.168.1.3 -> 1)
int getIPClassB(IPAddress address)
{
    return address[2];
}

// Returns computernumber in local network: 192.168.1.3 -> 3)
int getIPComputerNumber(IPAddress address)
{
    return address[3];
}

// Returns computernumber in local network: 192.168.1.105 -> 5)
int getIPComputerNumberOffset(IPAddress address, int offset)
{
    return getIPComputerNumber(address) - offset;
}

int distance(int trig, int echo)
{
  // Turn trigPin on for 10 micro seconds
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // Calculate distance 
  int t = pulseIn(echo, HIGH); // t = time between sending the pulse and recieving the pulse
  int s = t * 0.034 / 2; // s = distance between an object and the sensor

  // Print values to the serial monitor
  return s;
}
int average[6];
int index = 0;
int MIN = 0;
int MAX = 0;
int distanceAv(int trig, int echo)
{
  // Record the distance
  int a = distance(trig, echo);

  // Count to the end of the array
  if(average[index] == 0)
  {
    average[index] = a;
  }
  else
  {
    index++;
  }

  // check if the array is full
  if(index == 6)
  {
    // Reset total
    int total = 0;
    
    // Sum array
    for(int i = 0; i < 6; i++)
    {
      // if the program is at the first element, the element is the highest and lowest
      if(i == 0)
      {
        MIN = average[i];
        MAX = average[i];
      }

      if(average[i] > MAX)
      {
        MAX = average[i];
      }

      if(average[i] < MIN)
      {
        MIN = average[i];
      }
      total = total + average[i];
      average[i] = 0;
    }
    index = 0;
    total = total - (MIN + MAX);
    
    return total/4;
  }
  else
  {
    return 0;
  }
}

int leverDistance()
{
  int d = distanceAv(trigPin, echoPin);
  if(d != 0)
  {
    if(d > 0 && d <= 20)
    {
      lever.write(1);
      leverOpen = false;
    }
  }
}

void keepClosed(const long interval)
{
  int d = distanceAv(trigPin, echoPin);
  if(d != 0)
  {
    Serial.println(d);
  }
  
  unsigned long currentMillis = millis();
  if(beginCount == 0)
  {
    beginCount = currentMillis;
  }

  if(!first)
  {
    if(d > 0 && d <= 20)
    {
      beginCount = currentMillis;
    }
  }
  
  if (currentMillis - beginCount >= interval) {
    lever.write(90);
    leverOpen = true;
    //automatic = true;
    beginCount = 0;
  }
}

