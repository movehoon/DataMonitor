#include <WiFi.h>
#include <WebSocketsServer.h>

#define LED_BUILTIN 13

char ssid[32];
char password[32];
int counter = 0;
char str[256],push_cmd_str[128],cmd_str[128],reply_str[256],serial_recv_str[128];
int g_tid=1,tid=0,ret;
long templ=1000;
int cmd_index=0;
int debug_mode=0;
int push_data=0;
int connect_stat;
char remoteIPAddress[32];
IPAddress myip;

int TITANCommandReply(char command[], char reply[]);

// Globals
WebSocketsServer webSocket = WebSocketsServer(80);

//************************************ Called back for when receiving any WebSocket message
void onWebSocketEvent(uint8_t num,  
                      WStype_t type, 
                      uint8_t * payload, 
                      size_t length) 
{
        switch(type) {// Figure out the type of WebSocket event
                case WStype_DISCONNECTED: // Client has disconnected
                        if (debug_mode>0) Serial.printf("[%u] Disconnected!\n", num);
                        connect_stat=0;
                        break;
                case WStype_CONNECTED: // New client has connected
                {
                        IPAddress ip = webSocket.remoteIP(num);
                        connect_stat=1;
                        sprintf(remoteIPAddress,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
                        if (debug_mode>0) {
                          Serial.printf("[%u] Connection from %s\n", num, remoteIPAddress);
                        }
                        break;
                }
                case WStype_TEXT: // Service text message from the client
                        sprintf(cmd_str,"%s",payload);
                        if (debug_mode>0) Serial.printf("[%u] Send:(%d) %s\n", num, strlen(cmd_str),cmd_str);
                        ret = TITANCommandReply(cmd_str,reply_str);
                        //sprintf(reply_str,"#01:EX=%ld;VX=506.85;POSD=2997243;VPROF=500;MST=0x5;CURQA=0.0102675;CURQD=0.0558196;CURDD=-0.158617;DIN=0x0;DOUT=0x0;LED=1;RGB=4",++templ);
                        if (debug_mode>0) Serial.printf("[%u] Reply:(%d) %s\n", num, strlen(reply_str),reply_str);
                        webSocket.sendTXT(num, reply_str);
                        break;
                case WStype_BIN: // For everything else: do nothing
                case WStype_ERROR:
                case WStype_FRAGMENT_TEXT_START:
                case WStype_FRAGMENT_BIN_START:
                case WStype_FRAGMENT:
                case WStype_FRAGMENT_FIN:
                default:
                        break;
        }
}

//************************ Serial Flush Input Buffers
void serial_flush(void) {
       while (Serial2.available()) { Serial.printf("(Flush:%c)\n",Serial2.read());}
}

//************************ TITAN Command and Reply 

int TITANCommandReply(char command[], char reply[])
{
      int ri=0,toi=0;
      char cc;
      reply[0]='\0';
      serial_flush();
      Serial2.printf("%s\n",command);
      while (true){
          if (Serial2.available()){       
            cc = Serial2.read();
            if (cc=='^'){
                ESP.restart();
            }
            if ((cc != '\n')&&(cc != '\r')) {
                reply[ri++]=cc;
                reply[ri]='\0';
            }
            if (ri>255) return ri;
            if ((cc == '\n')) {
              serial_flush();
              return ri;
            }
            toi=0;
          }
          else{
            delay(1);
            if (++toi>1000) return -1;
          }
      }
}

//************************** Check Reset Command
void checkReset()
{
      char cc;
      if (Serial2.available()){       
         cc = Serial2.read();
         if (cc=='^'){
            ESP.restart();   
         }
      }
}

//************************** Setup
void setup() {
        int i,ipa1,ipa2,ipa3,ipa4, ret,bit=0;
        long ipalong;
        char str[64],replystr[64],cmdstr[64];
      
        delay(1000);  //***Wait 1 seconds at startup
        pinMode(LED_BUILTIN, OUTPUT);
        for (i=0;i<20;i++){ //***Flash LED while waiting for TITAN to boot up
            digitalWrite(LED_BUILTIN, 0);
            delay(i*5);
            digitalWrite(LED_BUILTIN, 1);
            delay(i*5);
        }
        digitalWrite(LED_BUILTIN, 1);
      
        // Start Serial port
        Serial.begin(115200);
        Serial2.begin(115200);
        Serial2.flush();

        //***Load Preferences stored in flash
        LoadPreference();
        sscanf(GetPreferenceID(), "%d", &g_tid);
        if (g_tid<1 || g_tid>50) {
          printf("Default Titan ID reset from %02d to 01\n",g_tid);
          g_tid=1;
        }
        else{
          printf("Main Titan ID %02d\n",g_tid);
        }
        sprintf(cmdstr,"@%02d:FIRMVS",g_tid);
        i = TITANCommandReply(cmdstr,replystr);
        if (i<1){
              printf("No communication with main TITAN.  Check TITAN ID!\n");    
              sprintf(ssid, "NA");
              sprintf(password, "NA");
        }
        else{
              sprintf(cmdstr,"@%02d:WID",g_tid); //*** Get WiFi SSID
              i = TITANCommandReply(cmdstr,replystr);
              sscanf(replystr,"#%d:WID=%s",&ret,ssid);
              Serial.printf("SSID=%s\n",ssid);
            
              sprintf(cmdstr,"@%02d:WPW",g_tid); //*** Get WiFi Password
              i = TITANCommandReply(cmdstr,replystr);
              sscanf(replystr,"#%d:WPW=%s",&ret,password);
              Serial.printf("PWD=%s\n",password);
              
              // Connect to WiFi access point
              Serial.printf("Connecting %s:%s ",ssid,password);
              WiFi.begin(ssid, password);
              ret =1;
              while (ret<10){ //*** Check connection to WiFi access point
                if ( WiFi.status() == WL_CONNECTED ) {
                    ret = 300; //***Connection success!
                }
                else{
                    for (i=0;i<10;i++){
                        digitalWrite(LED_BUILTIN, bit);
                        if (bit) bit=0; else bit=1;
                        delay(100);
                        checkReset();
                    }
                    ret++;
                    Serial.printf(".");
                }
              }
              Serial.printf("\n");
              digitalWrite(LED_BUILTIN, 1);
              if (ret==300){
                    myip = WiFi.localIP();

                    Serial.printf("Connect Success and my IP = %d.%d.%d.%d\n",myip[0],myip[1],myip[2],myip[3]);
                    ipalong = (long)(myip[0]) | (long)(myip[1])<<8 | (long)(myip[2])<<16 | (long)(myip[3])<<24;
                    sprintf(cmdstr,"@%02d:WL_ADD=%ld",g_tid,ipalong);
                    i = TITANCommandReply(cmdstr,replystr); //***Upload the IP address to the TITAN
                    Serial.println("*** Ready and Websocket Server Started ***");
                    webSocket.begin(); // Start WebSocket server and assign callback
                    webSocket.onEvent(onWebSocketEvent);
              }
              else{
                    Serial.print("Connection Unsuccesful!  Check SSID and PWD\n");
              }
        }
        sprintf(push_cmd_str,"@%02d:EX;VX;POSD;VPROF;MST;CURQA;CURQD;CURDD;DIN;DOUT;LED;RGB",g_tid);
}

//****************************** Main Loop 
void loop() {
        // Look for and handle WebSocket data
        webSocket.loop();
        serialEvent();
        checkReset();
        if (push_data>0 && connect_stat==1){
            ret = TITANCommandReply(push_cmd_str,reply_str);
            sprintf(reply_str,"ALL:%s\n",reply_str); 
            webSocket.sendTXT(0, reply_str);
            delay(push_data);
       }
}
//*********************** Serial Event Handler
void serialEvent () {
        while(Serial.available()) {
          serial_recv_str[cmd_index] = (uint8_t)Serial.read();
          if (serial_recv_str[cmd_index] == 0x0A || serial_recv_str[cmd_index] == 0x0D) {
            serial_recv_str[cmd_index] = 0x00;
            //memcpy(cmd_str, serial_recv_str, cmd_index+1);
            if (serial_recv_str[0] == '@') {
                ret = sscanf(serial_recv_str,"@%02d:",&tid);
                if (ret==1 && tid == g_tid){
                    ret = TITANCommandReply(serial_recv_str, reply_str);
                    if (ret>0){
                        Serial.printf("%s\n",reply_str);
                        Serial.flush();
                    }
                }
                //Send2Titan((const char *)u1_cmd);
            }
            else{
              parse((char *)serial_recv_str);
            }
            cmd_index = 0;
            while (Serial.available()) { 
              Serial.read();
            }
            continue;
          }
          cmd_index++;
        }
}

void substring(char s[], char sub[], int p, int l) {
         int c = 0;
         while (c < l) {
            sub[c] = s[p+c];
            c++;
         }
         sub[c] = '\0';
}
//***************************Parse
void parse(char* cmd) {
        char str1[16],str2[16],msg[32];
        int id;
        //Serial.printf("parsing: %s\n",cmd);
        if (strlen(cmd)==0)
          return;
      
        ret = sscanf(cmd,"%s",str1);
        
        if (strcmp(str1,"ID")==0) {
          sprintf(msg, "ID=%s\n", GetPreferenceID());
          printf(msg);
        }
        else if (str1[0]=='I' && str1[1]=='D' && str1[2]=='=') {
            substring(str1, str2,3, strlen(str1)-3);
            ret = sscanf(str2,"%d",&id);
            if (id<1 || id>50) {
              printf("ID set from %s to default 01\n",str2);
              id=1;
            }
            sprintf(str2,"%02d",id);
            SavePreferenceID(str2);
            sprintf(msg, "ID=%s\n", GetPreferenceID());
        }
        else if (str1[0]=='D' && str1[1]=='B' && str1[2]=='=') {
            substring(str1, str2,3, strlen(str1)-3);
            ret = sscanf(str2,"%d",&id);
            debug_mode=id;
            sprintf(msg, "DB=%d\n", debug_mode);
        }
        else if (str1[0]=='P' && str1[1]=='D' && str1[2]=='=') {
            substring(str1, str2,3, strlen(str1)-3);
            ret = sscanf(str2,"%d",&id);
            push_data=id;
            sprintf(msg, "PD=%d\n", push_data);
        }
        else if (strcmp(str1,"SSID")==0) {
          sprintf(msg, "SSID=%s\n", ssid);
          printf(msg);
        }
        else if (strcmp(str1,"PWD")==0) {
          sprintf(msg, "PWD=%s\n", password);
          printf(msg);
        }
        else if (strcmp(str1,"IPA")==0) {
          myip = WiFi.localIP();
          Serial.printf("IPA=%d.%d.%d.%d\n",myip[0],myip[1],myip[2],myip[3]);
        }
        else if (strcmp(str1,"RESET")==0) {
          printf("Resetting Wifi Module\n");
          ESP.restart();
        }
        else{
          printf("?%s\n",str1);
        }
}
