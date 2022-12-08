#ifndef Magellan_BC95_lite_h
#define Magellan_BC95_lite_h


#include <Arduino.h>
#include <Stream.h>


#define MODE_STRING 0
#define MODE_STRING_HEX 1


#if defined( __ARDUINO_X86__)
    #define ATLSOFTSERIAL 0
#elif defined(ARDUINO_NUCLEO_L476RG)
    #define ATLSOFTSERIAL 0
#elif defined(ARDUINO_Node32s)
    #define ATLSOFTSERIAL 0
#elif defined(ARDUINO_NodeMCU_32S)
    #define ATLSOFTSERIAL 0
#elif defined(ARDUINO_AVR_UNO) || (ARDUINO_AVR_MEGA2560)
	#define ATLSOFTSERIAL 1
#else 
    #define ATLSOFTSERIAL 1
#endif 

#if ATLSOFTSERIAL 
	#include "AltSoftSerial.h"
#endif

  const byte maxretrans=4;	

  const char con[]="40";
  const char contk[]="42";
  const char non_con[]="50";
  const char ack[]="60";
  const char acktk[]="62";
  const char rst[]="70";

  //const char EMPTY[]="00";
  const char GET[]="01";
  const char POST[]="02";
  const char PUT[]="03";
  const char DELETE[]="04";


enum rspcode {
	EMPTY=00,
	CREATED=65,
	DELETED=66,
	VALID=67,
	CHANGED=68,
	CONTENT=69,
	CONTINUE=95,
	BAD_REQUEST=128,
	FORBIDDEN=131,
	NOT_FOUND=132,
	METHOD_NOT_ALLOWED=133,
	NOT_ACCEPTABLE=134,
	REQUEST_ENTITY_INCOMPLETE=136,
	PRECONDITION_FAILED=140,
	REQUEST_ENTITY_TOO_LARGE=141,
	UNSUPPORTED_CONTENT_FORMAT=143,
	INTERNAL_SERVER_ERROR=160,
	NOT_IMPLEMENTED=161,
	BAD_GATEWAY=162,
	SERVICE_UNAVAILABLE=163,
	GATEWAY_TIMEOUT=164,
	PROXY_NOT_SUPPORTED=165
};


class Magellan_BC95_lite
{


public:
	Magellan_BC95_lite();

	bool debug;
	//bool default_server;
	bool printstate=true;

    //################# Magellan Platform ########################
    bool begin();
    String thingsRegister();
    String rssi();
    String report(String payload);
    
private:
	unsigned int Msg_ID=0;
	//################### Device #######################
	String imsi="";
	String deviceIP="";
	String Token="";
	//################### Buffer #######################
	String data_input;
	String data_buffer;
	String rcvdata="";
	//################### counter value ################
	byte k=0;
	//################## flag ##########################
	bool breboot_flag=false;
	bool end=false;
	bool send_NSOMI=false;
	bool flag_rcv=true;
	bool en_get=true;
	bool en_post=true;
	bool en_send=true;
	bool en_chk_buff=false;
	bool getpayload=false;
	bool Created=false;
	bool sendget=false;
	bool NOTFOUND=false;
	bool GETCONTENT=false;
	bool RCVRSP=false;
	bool success=false;
	bool connected=false;
	bool post_process=false;
	bool ACK=false;
	bool EMP=false;
	bool send_ACK=false;
	bool more_flag=false;
	bool hw_connected=false;
	bool token_error_report=true;
	bool bc95;
	//################### Parameter ####################
	//unsigned int resp_cnt=0;
	unsigned int resp_msgID=0;
	unsigned int post_ID=0;
	unsigned int post_token=0;
	unsigned int get_ID=0;
	unsigned int path_hex_len=0;
	//-------------------- timer ----------------------
	unsigned int previous_send=0;
	unsigned int previous_check=0;
	unsigned long previous=0;
	unsigned int wait_time=5;
	//unsigned int last_msgid=0;
	unsigned int token=0;
	unsigned int rsptoken=0;
	byte count_timeout=0;
	byte count_error_token_post=0;
	//################### Diagnostic & Report ###########
	byte maxtry=0;
	byte cnt_retrans=0;
	byte cnt_retrans_get=0;
	byte cnt_error=0;
	byte cnt_cmdgetrsp=0;
	byte cnt_rcv_resp=0;
	String LastError="";
	//###################################################

	//################# Function #######################
	void reboot_module();
	void setup_module();
	void check_module_ready();
	void _serial_flush();
    void printHEX(char *str);
    void printmsgID(unsigned int messageID);
	void print(char *str);
	void print_rsp_header(String Msgstr);
	void print_rsp_Type(String Msgstr,unsigned int msgID);
	void miniresponse(String rx);
	String post_data(String payload,bool register_thing);
	void Msgsend(char *payload_c,unsigned int payload_len,bool register_thing);
	void print_pathlen(unsigned int path_len,char *init_str);
	void waitResponse();

protected:
	 Stream *_Serial;
};

extern Magellan_BC95_lite nbiot;

#endif
