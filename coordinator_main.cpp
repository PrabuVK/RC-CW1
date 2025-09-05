#include "mbed.h"
#include <cstring>

#define BUFFER_SIZE 16

// Peripheral Initializations
AnalogIn potentiometer(A0);
BufferedSerial pc(USBTX, USBRX, 9600);
BufferedSerial xbee(PA_11, PA_12, 9600);
DigitalIn button(PC_13);

// receiver MAC Address
char checksumHeader[] = {0x10, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x42, 0x34, 0x74, 0x97, 0xFF, 0xFE, 0x00, 0x00};
char transmitBuffer[BUFFER_SIZE] = {0}; // Message Buffer
char xbeeBuffer[BUFFER_SIZE] = {0};

void constructPacket(char *xbeeMsg, char *data, int dataLen) {
    char startDelimiter = 0x7E;
    char lengthField[] = {0x00, 0x0F};
    char frameType = 0x10;
    char frameId = 0x01;
    char destinationAddr[] = {0x00, 0x13, 0xA2, 0x00, 0x42, 0x34, 0x74, 0x97, 0xFF, 0xFE};
    char broadcastRadius = 0x00;
    char options = 0x00;
    
    // Checksum Calculation
    int checksum = 0;
    for (int i = 0; i < sizeof(checksumHeader); i++) {
        checksum += checksumHeader[i];
    }
    for (int i = 0; i < dataLen; i++) {
        checksum += data[i];
    }
    char finalChecksum = (0xFF - checksum) & 0xFF;
    
    // **Prepare Full XBee Message**
    int packetLength = 17 + dataLen;
    char xbeeFrame[packetLength];
    
    xbeeFrame[0] = startDelimiter;
    memcpy(&xbeeFrame[1], lengthField, sizeof(lengthField));
    xbeeFrame[3] = frameType;
    xbeeFrame[4] = frameId;
    memcpy(&xbeeFrame[5], destinationAddr, sizeof(destinationAddr));
    xbeeFrame[15] = broadcastRadius;
    xbeeFrame[16] = options;
    
    // Attach Payload Data
    memcpy(&xbeeFrame[17], data, dataLen);
    
    // Append Checksum
    xbeeFrame[packetLength - 1] = finalChecksum;
    
    // Send Data to XBee Module
    xbee.write(xbeeFrame, packetLength);
    
    printf("\nPacket Sent to XBee:\n");
    for (int i = 0; i < packetLength; i++) {
        printf("%02X ", xbeeFrame[i]);
    }
    printf("\n");
}

// **Main Execution Loop**
int main() {
    while (true) {
        int buttonState = button.read();
        float potRead = potentiometer.read();
        int convertedValue = static_cast<int>(potRead * 255);
        
        char dataStr[10];
        snprintf(dataStr, sizeof(dataStr), "%d", convertedValue);
        
        pc.write(dataStr, strlen(dataStr));
        pc.write("\r\n", 2);
        
        transmitBuffer[0] = convertedValue;
        constructPacket(xbeeBuffer, transmitBuffer, 1);
        
        ThisThread::sleep_for(500ms);
    }
}