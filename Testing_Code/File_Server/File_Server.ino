/*  File Server

    This sketch will host my website!
    
    Programmed by Andy Tracy
*/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

// MAC address and IP address
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x0D, 0x67 };
IPAddress ip(192, 168, 0, 14);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 0, 0);

// Initialize the Ethernet server library
// (port 80 is default for HTTP)
EthernetServer server(80);

// SD card info
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;
const int cardpin = 4; // The card uses pin 4

void setup() {
  Serial.begin(9600);
  
  pinMode(10, OUTPUT); // Set this to output for initial setup
  digitalWrite(10, HIGH);
  
  // Initial debug run to make sure card is good
  if (!card.init(SPI_HALF_SPEED, 4)) Serial.println("card.init failed!");
  if (!volume.init(&card)) Serial.println("vol.init failed!");
  
  Serial.print("Volume is FAT");
  Serial.println(volume.fatType(),DEC);
  Serial.println();
 
  if (!root.openRoot(&volume)) Serial.println("openRoot failed");
 
  // List files in root with date and size
  Serial.println("Files found in root:");
  root.ls(LS_DATE | LS_SIZE);
  Serial.println();
 
  // Recursive list of all directories
  Serial.println("Files found in all dirs:");
  root.ls(LS_R);
 
  Serial.println();
  Serial.println("Done with card check; starting ethernet connection");
  
  // Initialize the Ethernet connection and the server
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
}

// How big our line buffer should be. 100 is plenty!
#define BUFSIZ 100

void loop(){
  char clientline[BUFSIZ];
  int index = 0;
  
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {    
    // reset the input buffer
    index = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) 
            index = BUFSIZ -1;
 
          // continue to read more data!
          continue;
        }
        clientline[index] = 0;
        
        // Look for substring such as a request to get the root file
        if (strstr(clientline, "GET / ") != 0) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
 
          // print all the files, use a helper to keep it clean
          client.println("<h2>Files:</h2>");
          ListFiles(client, 0);
        } else if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *filename;
 
          filename = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
 
          if (! file.open(&root, filename, FILE_READ)) {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<h2>File Not Found!</h2>");
            break;
          }
 
          Serial.println("Opened!");
 
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println();
 
          int16_t c;
          while ((c = file.read()) > 0) {
              // uncomment the serial to debug (slow!)
              //Serial.print((char)c);
              client.print((char)c);
          }
          file.close();
        } else {
          // everything else is a 404
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>File Not Found!</h2>");
        }
          
        break;
      }
    }
  }
  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client.stop();
}

void ListFiles(EthernetClient client, uint8_t flags) {
  dir_t p;
 
  root.rewind();
  client.println("<ul>");
  while (root.readDir(p) > 0) {
    // done if past last used entry
    if (p.name[0] == DIR_NAME_FREE) break;
 
    // skip deleted entry and entries for . and  ..
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;
 
    // only list subdirectories and files
    if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;
 
    // print any indent spaces
    client.print("<li><a href=\"");
    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        client.print('.');
      }
      client.write(p.name[i]);
    }
    client.print("\">");
 
    // print file name with possible blank fill
    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        client.print('.');
      }
      client.write(p.name[i]);
    }
 
    client.print("</a>");
 
    if (DIR_IS_SUBDIR(&p)) {
      client.print('/');
    }
 
    // print modify date/time if requested
    if (flags & LS_DATE) {
       root.printFatDate(p.lastWriteDate);
       client.print(' ');
       root.printFatTime(p.lastWriteTime);
    }
    // print size if requested
    if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE)) {
      client.print(' ');
      client.write(p.fileSize);
    }
    client.println("</li>");
  }
  client.println("</ul>");
}
