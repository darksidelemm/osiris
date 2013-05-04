/*
	Interactive Uplink
	
	Authors:	Mark Jessop (mjessop<at>eleceng.adelaide<dot>edu.au)
				
	Date: 2011-08-03
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a full copy of the GNU General Public License, 
    see <http://www.gnu.org/licenses/>.

*/

#include <RF22.h>
#include <SPI.h>
#include <string.h>
#include <util/crc16.h>


#define BOOM_INTERRUPT 1
#define PWR_LED	4
#define	STATUS_LED	A3
#define WIRE_FET 8
#define VALVE_FET 7
#define BATT	7

// This will have to be changed for whatever PCB is used.
#define RF22_SLAVESELECT	8 // Pin 10 on Osiris boards, pin 8 on badge
#define RF22_INTERRUPT		0

#define TX_POWER	RF22_TXPOW_5DBM  // Options are 1,2,5,8,11,14,17,20 dBm
#define LISTEN_TIME	200
#define RTTY_DELAY	19500 // 50 baud
//#define RTTY_DELAY	3400 // 300 baud

// Singleton instance of the RFM22B Library 
RF22 rf22(RF22_SLAVESELECT,RF22_INTERRUPT);

float FREQ = 431.700;

// Variables & Buffers
char txbuffer [128];
int	rfm_temp;
int rssi_floor = 0;
int last_rssi = 0;
char relaymessage[40] = "$3 CUT";
int batt_mv = 0;
unsigned int count = 0;
unsigned int rx_count = 0;
int current_power = 5;
unsigned char preamble_length = 8;

int transmit_trigger = 0;

void setup(){
	// Setup out IO pins
	pinMode(PWR_LED, OUTPUT);
	pinMode(STATUS_LED, OUTPUT);
	pinMode(WIRE_FET, OUTPUT);
	pinMode(VALVE_FET, OUTPUT);
	pinMode(3, INPUT);
	attachInterrupt(1,boom_handler,FALLING);
	analogReference(INTERNAL);
	digitalWrite(PWR_LED, HIGH);
	digitalWrite(STATUS_LED, LOW);
	digitalWrite(WIRE_FET, LOW);
	digitalWrite(VALVE_FET, LOW);
	Serial.begin(9600);
	Serial.println("Booting...");
	
	// Attempt to start the radio. If fail, blink.
	if(!rf22.init()) payload_fail();
	rf22.setTxPower(TX_POWER|0x08);
	RFM22B_RX_Mode();
	Serial.println("Transmitter is up.");
	settings();
}

void loop(){
	while(Serial.available()>0){ Serial.read();} // Flush the input buffer
	Serial.println("");
	Serial.println("CHOOSE WISELY:");
	Serial.println("(v)iew current settings.");
	Serial.println("Change (f)requency.");
	Serial.println("Change TX (p)ower.");
	Serial.println("Set (m)essage.");
	Serial.println("(s)end message with RTTY preamble.");
	Serial.println("(i)mmediate send.");
        Serial.println("Set GMSK packet pre(a)mble Length.");
	Serial.println("Turn reference tone (o)ff.");
	Serial.println("Turn reference tone o(n).");
	Serial.println("Go to interactive (l)isten mode.");
	
	while(Serial.available()==0 && transmit_trigger==0){} // Wait for input
	
	if(transmit_trigger){
		 Serial.println("CAUGHT BUTTON PRESS. TRANSMITTING.");
		 RFM22B_RX_Mode();
		 delay(100);
		 rf22.send((uint8_t*)relaymessage, (uint8_t)strlen(relaymessage));
		 rf22.waitPacketSent();
		 Serial.print("Sent: ");
		 Serial.println(relaymessage);
		 transmit_trigger = 0;
		 return;
	}
	
	char cmd = Serial.read();
	delay(300);
	while(Serial.available()>0){ Serial.read();} // Flush the input buffer
	Serial.println("");
	switch(cmd){
		case 'f':
			set_frequency();
			break;
			
		case 'm':
			read_message();
			break;
		
		case 's':
			send_message();
			break;
			
		case 'i':
			send_message_immediate();
			break;
			
		case 'o':
			RFM22B_RX_Mode();
			Serial.println("Done");
			break;
			
		case 'n':
			RFM22B_RTTY_Mode();
			Serial.println("Done");
			break;
			
		case 'l':
			interactive_listen();
			break;
		
		case 'p':
			set_power();
			break;
		
		case 'v':
			settings();
			break;

        case 'a':
            set_preamble();
            break;
		default:
			break;
	}
}

void read_message(){
	while(Serial.available()>0){ Serial.read();} // Flush the input buffer
	Serial.print("Enter message, then LF: ");
	int i = 0;
	while(1){
		while(Serial.available()==0){} // Wait for a character
		char temp = Serial.read();
		if(temp == '\n' || temp == '\r'){
			relaymessage[i] = 0;
			Serial.println("");
			Serial.print("New Message: ");
			Serial.println(relaymessage);
			return;
		}else{
			relaymessage[i] = temp;
			Serial.print(temp);
		}
		i++;
	}
}

void send_message(){
	RFM22B_RTTY_Mode();
	Serial.println("Sending 10s of RTTY: .");
	rtty_txstring("VK5QI blahblahblahblahblah tune me up scotty 54321\n");
	delay(100);
	
	RFM22B_RX_Mode();
	delay(400);
	

	rf22.send((uint8_t*)relaymessage, (uint8_t)strlen(relaymessage));
	rf22.waitPacketSent();
	Serial.print("Sent: ");
	Serial.println(relaymessage);
	delay(300);
	//RFM22B_RTTY_Mode();
}

void send_message_immediate(){
	RFM22B_RX_Mode();
	delay(200);
	

	rf22.send((uint8_t*)relaymessage, (uint8_t)strlen(relaymessage));
	rf22.waitPacketSent();
	Serial.print("Sent: ");
	Serial.println(relaymessage);
	delay(300);
	//RFM22B_RTTY_Mode();
}

void set_frequency(){
	while(Serial.available()>0){ Serial.read();} // Flush the input buffer
	Serial.print("Enter Frequency in MHz (XXX.XXX): ");
	while(Serial.available()<8){}
	FREQ = Serial.parseFloat();
	rf22.setFrequency(FREQ);
	Serial.println("");
	Serial.print("Frequency set to ");
	Serial.println(FREQ,3);
}

void set_preamble(){
	while(Serial.available()>0){ Serial.read();} // Flush the input buffer
	Serial.println("Set preamble length (008-255):");
	while(Serial.available()<3){}
	int input = Serial.parseInt();
    
        if(input<255 && input>=8){
          preamble_length = input;
        }
        
        Serial.print("New Preamble Length: ");
        Serial.println(preamble_length,DEC);

}

void set_power(){
	while(Serial.available()>0){ Serial.read();} // Flush the input buffer
	Serial.println("Valid levels are 1,2,5,8,11,14,17,20 dBm.");
	Serial.println("5dBm produces approx 5W output on the Uplink box.");
	while(Serial.available()<2){}
	int input = Serial.parseInt();
	
	switch (input){
		case 1:
			rf22.setTxPower(RF22_TXPOW_1DBM|0x08);
			break;
		case 2:
			rf22.setTxPower(RF22_TXPOW_2DBM|0x08);
			break;
		case 5:
			rf22.setTxPower(RF22_TXPOW_5DBM|0x08);
			break;
		case 8:
			rf22.setTxPower(RF22_TXPOW_8DBM|0x08);
			break;
		case 11:
			rf22.setTxPower(RF22_TXPOW_11DBM|0x08);
			break;
		case 14:
			rf22.setTxPower(RF22_TXPOW_14DBM|0x08);
			break;
		case 17:
			rf22.setTxPower(RF22_TXPOW_17DBM|0x08);
			break;
		case 20:
			rf22.setTxPower(RF22_TXPOW_20DBM|0x08);
			break;
		default:
			Serial.println("Invalid value.");
			return;
	}
	current_power = input;
	Serial.print("New power level: ");
	Serial.println(input,DEC);

}

void settings(){
	Serial.print("Frequency: "); Serial.println(FREQ,4);
	Serial.print("Power: "); Serial.println(current_power);
	Serial.print("Message: "); Serial.println(relaymessage);
        Serial.print("Preamble Length: "); Serial.println(preamble_length);
}


void interactive_listen(){

	uint8_t buf[RF22_MAX_MESSAGE_LEN];
	uint8_t len = sizeof(buf);
	
	RFM22B_RX_Mode();
	Serial.println("[ and ] to tune down and up, s to send preset packet, q to quit.");
	while(1){
		int rssi_floor = ((int)rf22.rssiRead()*51 - 12400)/100;
		Serial.print(FREQ,4); Serial.print(" "); Serial.print(rssi_floor); Serial.print("dBm - ");
		int numstars = (120+rssi_floor)/3;
		for(int i = 0; i<numstars; i++){
			Serial.print("*");
		}
		Serial.println("");
		
		
		if(rf22.waitAvailableTimeout(LISTEN_TIME)){
		// Heard a packet!
			if(rf22.recv(buf, &len)){
				int last_rssi = ((int)rf22.lastRssi()*51 - 12400)/100;
				Serial.print("Got Packet with RSSI "); Serial.print(last_rssi); Serial.print("dBm: ");
				Serial.println(len, DEC);
				Serial.write(buf,len); Serial.println("");
				delay(500);
			}
		}
	
		while(Serial.available()>0){
			switch(Serial.read()){
				case '[':
					FREQ = FREQ - 0.001;
					rf22.setFrequency(FREQ);
					break;
				
				case ']':
					FREQ = FREQ + 0.001;
					rf22.setFrequency(FREQ);
					break;
				
				case';':
					FREQ = FREQ - 0.0002;
					rf22.setFrequency(FREQ);
					break;
				case '\'':
					FREQ = FREQ + 0.0002;
					rf22.setFrequency(FREQ);
					break;
					
				case 's':
						rf22.send((uint8_t*)relaymessage, (uint8_t)strlen(relaymessage));
						rf22.waitPacketSent();
						Serial.print("Sent: ");
						Serial.println(relaymessage);
						break;
						
				case 'q':
					return;
					break;
					
				// Standard commands, hardcoded callsigns.
				
				case '0':{
					char cmd_data[] = "$0 VK5QI PING";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case '1':{
					char cmd_data[] = "$1 VK5QI ARM";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case '2':{
					char cmd_data[] = "$2 VK5QI FIRE2s";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case '3':{
					char cmd_data[] = "$3 VK5QI FIRE4s";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case '4':{
					char cmd_data[] = "$4 VK5QI FIRE6s";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case '5':{
					char cmd_data[] = "$5 VK5QI FIRE10s";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case 'a':{
					char cmd_data[] = "$A VK5QI";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case 'b':{
					char cmd_data[] = "$B VK5QI";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case 'c':{
					char cmd_data[] = "$C VK5QI 50bd";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case 'd':{
					char cmd_data[] = "$D VK5QI 300bd";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case 'e':{
					char cmd_data[] = "$E VK5QI";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				case 'f':{
					char cmd_data[] = "$F VK5QI";
					rf22.send((uint8_t*)cmd_data, sizeof(cmd_data));
					rf22.waitPacketSent();
					Serial.print("Sent: ");
					Serial.println(cmd_data);
					break;
				}
				
				default:
					break;
					
			}
		}
	
	}


}


void payload_fail(){
	while(1){
		digitalWrite(PWR_LED, HIGH);
		digitalWrite(STATUS_LED, LOW);
		delay(300);
		digitalWrite(PWR_LED, LOW);
		digitalWrite(STATUS_LED, HIGH);
		delay(300);
	}
}

void boom_handler(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 1000) 
  {
    transmit_trigger = 1;
  }
  last_interrupt_time = interrupt_time;
}
