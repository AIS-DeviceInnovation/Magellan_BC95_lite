/*
Copyright (c) 2020, Advanced Wireless Network
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Magellan_BC95_lite v1.0.2 NB-IoT Magellan Platform .
Quectel BC95
NB-IoT with AT command

Library/SDK has developed with CoAP protocol.
reference with
https://tools.ietf.org/html/rfc7252

support post and get method
and supported only Magellan IoT Platform

*** the payload has limit with the dynamic memory of your board

Author: Device Innovation team
Create Date: 3 February 2020.
Modified: 06 June 2020.

(*v1.0.1 customize dynamic memory and add auto reset function)
(*v1.0.2 bug fix logic response code only 40300 and limit maximum payload 100 character)
Released for private usage.
*/

#include "Magellan_BC95_lite.h"

Magellan_BC95_lite nbiot;

#if defined(__ARDUINO_X86__)
    #define Serial_PORT Serial1
#elif defined(ARDUINO_NUCLEO_L476RG)
    #define Serial_PORT Serial1
#elif defined(ARDUINO_Node32s)
    #define Serial_PORT Serial2
#elif defined(ARDUINO_NodeMCU_32S)
    #define Serial_PORT Serial2
#elif defined(ARDUINO_AVR_UNO) || (ARDUINO_AVR_MEGA2560)
    AltSoftSerial moduleserial;
#else
    AltSoftSerial moduleserial;
#endif

const char serverIP[] = "119.31.104.1";

Magellan_BC95_lite::Magellan_BC95_lite() {
}
/*------------------------------
  Connect to NB-IoT Network
              &
  Initial Magellan
  ------------------------------
*/
String Magellan_BC95_lite::thingsRegister() {
    Serial.println(F("--------registerThings---------"));
    Token = post_data("", true);
    if (Token.length() == 36) {
        token_error_report = true;
    }
    return Token;
}

bool Magellan_BC95_lite::begin() {
    bool created       = false;
    token_error_report = true;
#if defined(__ARDUINO_X86__)
    Serial1.begin(9600);
    _Serial = &Serial1;
    // Serial.println(F("PLEASE USE Hardware Serial"))
#elif defined(ARDUINO_NUCLEO_L476RG)
    Serial1.begin(9600);
    _Serial = &Serial1;
    // Serial.println(F("PLEASE USE PIN RX=48 & TX=46"))
#elif defined(ARDUINO_Node32s)
    Serial2.begin(9600);
    _Serial = &Serial2;
    Serial.println();
    Serial.println(F("PLEASE USE PIN RX=RX2 & TX=TX2 & 3V3=IOREF"));
#elif defined(ARDUINO_NodeMCU_32S)
    Serial2.begin(9600);
    _Serial = &Serial2;
    Serial.println();
    Serial.println(F("PLEASE USE PIN RX=RX2 & TX=TX2 & 3V3=IOREF"));
#elif defined(ARDUINO_AVR_UNO)
    moduleserial.begin(9600);
    _Serial = &moduleserial;
#elif defined(ARDUINO_AVR_MEGA2560)
    moduleserial.begin(9600);
    _Serial = &moduleserial;
    Serial.println(F("PLEASE USE PIN RX=48 & TX=46"));
#else
    moduleserial.begin(9600);
    _Serial = &moduleserial;
#endif

    Serial.println();
    Serial.println(F("          AIS NB-IoT Magellan_BC95_lite V1.0.2"));

    /*---------------------------------------
        Initial BC95 Module
      ---------------------------------------
    */
    previous_check = millis();

    if (LastError != "") {
        Serial.print(F("LastError"));
        Serial.println(LastError);
    }
    check_module_ready();

    if (!hw_connected) {
        en_post = false;
        en_get  = false;
        return false;
    }
    reboot_module();
    setup_module();

    previous = millis();
    created  = true;

    return created;
}

void Magellan_BC95_lite::reboot_module() {
    Serial.println(F(">>Rebooting "));
    _Serial->println(F("AT+NRB"));
    delay(100);

    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (data_input.indexOf(F("OK")) != -1) {
                Serial.println(data_input);
                break;
            }
            else {
                if (data_input.indexOf(F("REBOOT_")) != -1) {
                    Serial.println(data_input);
                }
                else {
                    Serial.print(F("."));
                }
            }
        }
    }
    delay(10000);
}

void Magellan_BC95_lite::setup_module() {
    data_input = "";
    Serial.print(F(">>Setting"));
    _Serial->println(F("AT+CMEE=1"));
    delay(500);
    Serial.print(F("."));

    /*--------------------------------------
        Config module parameter
      --------------------------------------
    */
    _Serial->println(F("AT+CFUN=1"));
    delay(6000);
    Serial.print(F("."));
    _Serial->println(F("AT+NCONFIG=AUTOCONNECT,true"));
    delay(500);
    Serial.println(F("OK"));

    delay(4000);
    Serial.print(F(">>RSSI  "));
    Serial.print(rssi());
    Serial.println(F(" dBm"));

    // IMSI
    _serial_flush();
    data_input = "";
    imsi       = "";
    Serial.print(F(">>IMSI  "));
    _Serial->println(F("AT+CIMI"));
    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (data_input.indexOf(F("OK")) != -1 && imsi.indexOf(F("52003")) != -1)
                break;
            else if (data_input.indexOf(F("ERROR")) != -1)
                _Serial->println(F("AT+CIMI"));
            else
                imsi += data_input;
        }
    }
    imsi.replace(F("OK"), "");
    imsi.trim();
    Serial.println(imsi);

    // ICCID
    _serial_flush();
    data_input = "";
    Serial.print(F(">>ICCID "));
    _Serial->println(F("AT+NCCID"));
    while (1) {

        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (data_input.indexOf(F("+NCCID:")) != -1) {
                data_input.replace(F("+NCCID:"), "");
                Serial.println(data_input);
            }
            else if (data_input.indexOf(F("OK")) != -1 && data_input != "")
                break;
        }
    }

    // IMEI
    _serial_flush();
    data_input = "";
    Serial.print(F(">>IMEI  "));
    _Serial->println(F("AT+CGSN=1"));
    while (1) {

        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (data_input.indexOf(F("+CGSN:")) != -1) {
                data_input.replace(F("+CGSN:"), "");
                Serial.println(data_input);
            }
            else if (data_input.indexOf(F("OK")) != -1 && data_input != "")
                break;
        }
    }

    _Serial->println(F("AT+CGATT=1"));
    delay(500);
    Serial.print(F(">>Connecting"));
    _serial_flush();

    /*--------------------------------------
        Check network connection
      --------------------------------------
    */
    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (debug)
                Serial.println(data_input);
            if (data_input.indexOf(F("+CGATT:1")) != -1) {
                break;
            }
        }
        _Serial->println(F("AT+CGATT?"));
        Serial.print(F("."));
        delay(2000);
    }

    Serial.println(F("OK"));

    // Device IP
    _serial_flush();
    data_input = "";
    deviceIP   = "";
    _Serial->println(F("AT+CGPADDR=0"));
    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (data_input.indexOf(F("+CGPADDR")) != -1) {
                byte index  = data_input.indexOf(F(":"));
                byte index2 = data_input.indexOf(F(","));
                deviceIP    = data_input.substring(index2 + 1, data_input.length());
            }
            else if (data_input.indexOf(F("OK")) != -1)
                break;
        }
    }
    deviceIP.replace(F("\""), "");
    deviceIP.trim();
    // Serial.print(F(">>DeviceIP "));
    // Serial.println(deviceIP);

    /*-------------------------------------
        Create network socket
      -------------------------------------
    */
    data_input = "";
    _Serial->println(F("AT+NSOCR=DGRAM,17,5684,1"));
    delay(500);
    _serial_flush();

    while (true) {
        thingsRegister();
        if (success && token_error_report) {
            break;
        }
    }
}

void Magellan_BC95_lite::check_module_ready() {
    _Serial->println(F("AT"));
    delay(100);
    _Serial->println(F("AT"));
    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            if (data_input.indexOf(F("OK")) != -1) {
                hw_connected = true;
                break;
            }
        }
        else {
            unsigned int current_check = millis();
            if (current_check - previous_check > 5000) {
                Serial.println(F("Error to connect NB Module try reset or check your hardware"));
                previous_check = current_check;
                hw_connected   = false;
                // break;
            }
            else {
                _Serial->println(F("AT"));
                delay(100);
            }
        }
    }

    _serial_flush();
}

void Magellan_BC95_lite::_serial_flush() {
    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
        }
        else {
            break;
        }
    }
}
/*------------------------------
    CoAP Message menagement
  ------------------------------
*/
void Magellan_BC95_lite::printHEX(char* str) {
    char* hstr;
    hstr        = str;
    char out[3] = "";
    int  i      = 0;
    bool flag   = false;
    while (*hstr) {
        flag = itoa((int)*hstr, out, 16);

        if (flag) {
            _Serial->print(out);

            if (debug) {
                Serial.print(out);
            }
        }
        hstr++;
    }
}
void Magellan_BC95_lite::printmsgID(unsigned int messageID) {
    char Msg_ID[3];

    utoa(highByte(messageID), Msg_ID, 16);
    if (highByte(messageID) < 16) {
        if (debug)
            Serial.print(F("0"));
        _Serial->print(F("0"));
        if (debug)
            Serial.print(Msg_ID);
        _Serial->print(Msg_ID);
    }
    else {
        _Serial->print(Msg_ID);
        if (debug)
            Serial.print(Msg_ID);
    }

    utoa(lowByte(messageID), Msg_ID, 16);
    if (lowByte(messageID) < 16) {
        if (debug)
            Serial.print(F("0"));
        _Serial->print(F("0"));
        if (debug)
            Serial.print(Msg_ID);
        _Serial->print(Msg_ID);
    }
    else {
        _Serial->print(Msg_ID);
        if (debug)
            Serial.print(Msg_ID);
    }
}
void Magellan_BC95_lite::print_pathlen(unsigned int path_len, char* init_str) {
    unsigned int extend_len = 0;

    if (path_len >= 13) {
        extend_len = path_len - 13;

        char extend_L[3];
        itoa(lowByte(extend_len), extend_L, 16);
        _Serial->print(init_str);
        _Serial->print(F("d"));

        if (debug)
            Serial.print(init_str);
        if (debug)
            Serial.print(F("d"));

        if (extend_len <= 15) {
            _Serial->print(F("0"));
            _Serial->print(extend_L);

            if (debug)
                Serial.print(F("0"));
            if (debug)
                Serial.print(extend_L);
        }
        else {
            _Serial->print(extend_L);
            if (debug)
                Serial.print(extend_L);
        }
    }
    else {
        if (path_len <= 9) {
            char hexpath_len[2] = "";
            sprintf(hexpath_len, "%i", path_len);
            _Serial->print(init_str);
            _Serial->print(hexpath_len);
            if (debug)
                Serial.print(init_str);
            if (debug)
                Serial.print(hexpath_len);
        }
        else {
            if (path_len == 10) {
                _Serial->print(init_str);
                _Serial->print(F("a"));
                if (debug)
                    Serial.print(init_str);
                if (debug)
                    Serial.print(F("a"));
            }
            if (path_len == 11) {
                _Serial->print(init_str);
                _Serial->print(F("b"));
                if (debug)
                    Serial.print(init_str);
                if (debug)
                    Serial.print(F("b"));
            }
            if (path_len == 12) {
                _Serial->print(init_str);
                _Serial->print(F("c"));
                if (debug)
                    Serial.print(init_str);
                if (debug)
                    Serial.print(F("c"));
            }
        }
    }
}

/*-----------------------------
    CoAP Method POST
  -----------------------------
*/
String Magellan_BC95_lite::report(String payload) {
#if defined(ARDUINO_AVR_PRO) || (ARDUINO_AVR_UNO)
    if (payload.length() > 100) {
        Serial.println("Warning payload size exceed the limit of memory");
        return post_data(payload, false);
    }
    else {
        return post_data(payload, false);
    }
#else
    return post_data(payload, false);
#endif
}

String Magellan_BC95_lite::post_data(String payload, bool register_thing) {
    unsigned int timeout[5] = {12000, 14000, 18000, 26000, 42000};
    unsigned int prev_send  = millis();

    if (en_post) {
        data_buffer   = "";
        previous_send = millis();
        send_ACK      = false;
        ACK           = false;
        success       = false;
        token         = random(0, 32767);
        post_token    = token;
        if (debug)
            Serial.println(F("Load new payload"));
        Msg_ID = random(0, 65535);
        for (byte i = 0; i <= 4; ++i) {
            post_process                    = true;
            en_chk_buff                     = false;
            post_ID                         = Msg_ID;
            char data[payload.length() + 1] = "";
            payload.toCharArray(data, payload.length() + 1);
            Msgsend(data, payload.length(), register_thing);

            while (true) {
                en_chk_buff = true;
                waitResponse();
                unsigned int currenttime = millis();
                if (currenttime - previous_send > timeout[i] || success) {
                    if (debug)
                        Serial.println(currenttime - previous_send);
                    previous_send = currenttime;
                    en_post       = true;
                    en_get        = true;
                    post_process  = false;
                    break;
                }
            }
            if (success) {
                break;
            }
            else {
                if (i + 1 < 5) {
                    if (printstate)
                        Serial.print(F(">> Retransmit"));
                    if (printstate)
                        Serial.println(i + 1);
                    if (printstate)
                        Serial.println(timeout[i + 1]);
                }
            }
        }
        if (!success) {
            Serial.print(F("Timeout : "));
            data_input = "";
            count_timeout++;
            Serial.println(count_timeout);
            if (printstate)
                Serial.println();

            if (count_timeout >= 3) {
                count_timeout = 0;
                do {
                    check_module_ready();
                    __asm__ __volatile__("jmp 0x0000");
                } while (!hw_connected);
                reboot_module();
                setup_module();
            }
        }
    }

    if (data_buffer.indexOf(F("20000")) != -1 || data_buffer.length() == 36)
        count_error_token_post = true;

    else {
        Serial.println(F("Device has not registered to the Magellan Platform or Invalid Token."));
        token_error_report = false;
        count_error_token_post++;
        if (count_error_token_post >= 10) {
            count_error_token_post = 0;
            do {
                check_module_ready();
                __asm__ __volatile__("jmp 0x0000");
            } while (!hw_connected);
            reboot_module();
            setup_module();
            token_error_report = true;
        }
    }

    post_process = false;
    _Serial->flush();
    return data_buffer;
}

void Magellan_BC95_lite::Msgsend(char* payload_c, unsigned int payload_len, bool register_thing) {
    if (en_post) {

        if (printstate)
            Serial.print(F(">> post: Msg_ID "));
        if (printstate)
            Serial.print(Msg_ID);
        if (printstate)
            Serial.print(F(" "));
        if (printstate)
            Serial.println(payload_c);

        _Serial->print(F("AT+NSOST=0,"));
        _Serial->print(serverIP);
        _Serial->print(F(",5683,"));

        byte deviceIP_pathlen = 1;
        if (deviceIP.length() >= 13)
            deviceIP_pathlen = 2;

        unsigned int buff_len;
        if (register_thing == true)
            buff_len = 6 + 16 + 2 + imsi.length() + deviceIP_pathlen + deviceIP.length() + 2;
        if (register_thing == false)
            buff_len = 6 + 14 + 2 + Token.length() + deviceIP_pathlen + deviceIP.length() + 2 + 1 + payload_len;

        _Serial->print(buff_len);
        if (debug)
            Serial.print(F("AT+NSOST=0,"));
        if (debug)
            Serial.print(serverIP);
        if (debug)
            Serial.print(F(",5683,"));
        if (debug)
            Serial.print(buff_len);
        if (debug)
            Serial.print(F(",4202"));

        _Serial->print(F(",4202"));
        printmsgID(Msg_ID);
        printmsgID(post_token); // print token

        if (register_thing == true) {
            _Serial->print(F("b872656769737465720373696d027631"));
            if (debug)
                Serial.print(F("b872656769737465720373696d027631"));

            print_pathlen(imsi.length(), "0");
            char data_imsi[imsi.length() + 1] = "";
            imsi.toCharArray(data_imsi, imsi.length() + 1);
            printHEX(data_imsi);

            print_pathlen(deviceIP.length(), "0");
            char data_ip[deviceIP.length() + 1] = "";
            deviceIP.toCharArray(data_ip, deviceIP.length() + 1);
            printHEX(data_ip);
            if (debug)
                Serial.println(F("1132"));
            _Serial->print(F("1132")); // content-type json
            _Serial->println();
        }
        else if (register_thing == false) {
            _Serial->print(F("b67265706f72740373696d027631"));
            if (debug)
                Serial.print(F("b67265706f72740373696d027631"));

            // Serial.println(Token);
            print_pathlen(Token.length(), "0");
            char data_token[Token.length() + 1] = "";
            Token.toCharArray(data_token, Token.length() + 1);
            printHEX(data_token);

            print_pathlen(deviceIP.length(), "0");
            char data_ip[deviceIP.length() + 1] = "";
            deviceIP.toCharArray(data_ip, deviceIP.length() + 1);
            printHEX(data_ip);
            if (debug)
                Serial.print(F("1132"));
            _Serial->print(F("1132")); // content-type json

            _Serial->print(F("ff"));
            if (debug)
                Serial.print(F("ff"));

            printHEX(payload_c);
            _Serial->println();
            if (debug)
                Serial.println();
        }
    }
}

/*-------------------------------------
    Response Message management

    Example Message
    Type: ACK
    MID: 000001
    Code: Created
    Payload:200
  -------------------------------------
*/

void Magellan_BC95_lite::print_rsp_header(String Msgstr) {

    // if(debug) Serial.println("2_"+Msgstr);

    resp_msgID = (unsigned int)strtol(&Msgstr.substring(4, 8)[0], NULL, 16);
    print_rsp_Type(Msgstr.substring(0, 2), resp_msgID);

    bool en_print = (post_process && resp_msgID == post_ID);

    switch ((int)strtol(&Msgstr.substring(2, 4)[0], NULL, 16)) {
        case EMPTY :
            EMP = true;
            Msgstr.remove(0, 8);
            break;
        case CONTENT :
            EMP        = false;
            NOTFOUND   = false;
            GETCONTENT = false;
            RCVRSP     = true;

            if (Msgstr.length() / 2 > 4) {
                rsptoken = (unsigned int)strtol(&Msgstr.substring(8, 12)[0], NULL, 16);
                if (post_process && post_token == rsptoken) {
                    if (debug)
                        Serial.println(F("match token"));
                    if (debug)
                        Serial.println(rsptoken);
                    success = true;
                }

                Msgstr.remove(0, 12);
            }
            else {
                Msgstr.remove(0, 8);
            }

            if (printstate && en_print)
                Serial.println(F("2.05 CONTENT"));
            break;
        case DELETED : // if(printstate && en_print) Serial.println(F("2.02 DELETED"));
            break;
        case VALID : // if(printstate && en_print) Serial.println(F("2.03 VALID"));
            break;
        case CHANGED : // if(printstate && en_print) Serial.println(F("2.04 CHANGED"));
            break;
        case CREATED :
            /*EMP=false;
              NOTFOUND=false;
              GETCONTENT=true;
              RCVRSP=false;
              if(get_process){
                String blocksize=Msgstr.substring(20,22);
                if(blocksize.indexOf(F("0C"))!=-1)
                  {
                    more_flag=true;
                  }
                if(blocksize.indexOf(F("04"))!=-1){
                    more_flag=false;
                  }

                if (Msgstr.length()/2>4)
                {
                  rsptoken=(unsigned int) strtol( &Msgstr.substring(8,12)[0], NULL, 16);
                  if (get_process && get_token==rsptoken)
                  {
                    if(debug) Serial.println(F("match token"));
                    if(debug) Serial.print(rsptoken);
                    if(!more_flag) success=true;

                  }
                }2.01 CREATED
              }*/
            Msgstr.remove(0, 8);
            if (printstate && en_print)
                Serial.println(F("2.01 CREATED"));
            break;
        case CONTINUE : // if(printstate && en_print) Serial.println(F("2.31 CONTINUE"));
            Msgstr.remove(0, 8);
            break;
        case BAD_REQUEST :
            if (printstate && en_print)
                Serial.println(F("4.00 BAD_REQUEST"));
            Msgstr.remove(0, 8);
            break;
        case FORBIDDEN :
            if (printstate && en_print)
                Serial.println(F("4.03 FORBIDDEN"));
            Msgstr.remove(0, 8);
            break;
        case NOT_FOUND :
            if (printstate && en_print)
                Serial.println(F("4.04 NOT_FOUND"));
            GETCONTENT = false;
            NOTFOUND   = true;
            RCVRSP     = false;
            break;
        case METHOD_NOT_ALLOWED :
            RCVRSP = false;
            if (printstate && en_print)
                Serial.println(F("4.05 METHOD_NOT_ALLOWED"));
            break;
        case NOT_ACCEPTABLE :
            if (printstate && en_print)
                Serial.println(F("4.06 NOT_ACCEPTABLE"));
            break;
        case REQUEST_ENTITY_INCOMPLETE : // if(printstate && en_print) Serial.println(F("4.08
                                         // REQUEST_ENTITY_INCOMPLETE"));
            break;
        case PRECONDITION_FAILED : // if(printstate && en_print) Serial.println(F("4.12 PRECONDITION_FAILED"));
            break;
        case REQUEST_ENTITY_TOO_LARGE : // if(printstate && en_print) Serial.println(F("4.13
                                        // REQUEST_ENTITY_TOO_LARGE"));
            break;
        case UNSUPPORTED_CONTENT_FORMAT :
            if (printstate && en_print)
                Serial.println(F("4.15 UNSUPPORTED_CONTENT_FORMAT"));
            break;
        case INTERNAL_SERVER_ERROR :
            if (printstate && en_print)
                Serial.println(F("5.00 INTERNAL_SERVER_ERROR"));
            break;
        case NOT_IMPLEMENTED : // if(printstate && en_print) Serial.println(F("5.01 NOT_IMPLEMENTED"));
            break;
        case BAD_GATEWAY :
            if (printstate && en_print)
                Serial.println(F("5.02 BAD_GATEWAY"));
            break;
        case SERVICE_UNAVAILABLE :
            if (printstate && en_print)
                Serial.println(F("5.03 SERVICE_UNAVAILABLE"));
            break;
        case GATEWAY_TIMEOUT :
            if (printstate && en_print)
                Serial.println(F("5.04 GATEWAY_TIMEOUT"));
            break;
        case PROXY_NOT_SUPPORTED :
            if (printstate && en_print)
                Serial.println(F("5.05 PROXY_NOT_SUPPORTED"));
            break;

        default : // Optional
            GETCONTENT = false;
    }

    if (printstate && en_print)
        Serial.print(F("   Msg_ID "));

    if (printstate && en_print)
        Serial.println(resp_msgID);
}

void Magellan_BC95_lite::print_rsp_Type(String Msgstr, unsigned int msgID) {
    bool en_print = (post_process && resp_msgID == post_ID);

    if (Msgstr.indexOf(ack) != -1 || Msgstr.indexOf(acktk) != -1) {

        if (printstate && en_print)
            Serial.print(F("<< ACK: "));
        if ((resp_msgID == get_ID || resp_msgID == post_ID) && !EMP) {
            ACK = true;
        }

        flag_rcv = true;
        en_post  = true;
        en_get   = true;

        send_ACK      = false;
        cnt_cmdgetrsp = 0;
    }
    if (Msgstr.indexOf(con) != -1 || Msgstr.indexOf(contk) != -1) {
        if (printstate && en_print)
            Serial.print(F("<< CON: "));
        // resp_cnt++;

        en_post = false;
        en_get  = true;

        if (debug)
            Serial.println(F("Send ack"));
        _Serial->print(F("AT+NSOST=0,"));
        _Serial->print(serverIP);
        _Serial->print(F(",5683,"));
        _Serial->print(F("4"));
        _Serial->print(F(",6000"));
        printmsgID(msgID);
        _Serial->println();

        send_ACK = true;

        ACK           = false;
        cnt_cmdgetrsp = 0;
    }
    if (Msgstr.indexOf(rst) != -1) {
        if (printstate && en_print)
            Serial.print(F("<< RST: "));
        flag_rcv      = true;
        ACK           = false;
        cnt_cmdgetrsp = 0;
    }
    if (Msgstr.indexOf(non_con) != -1) {
        if (printstate && en_print)
            Serial.print(F("<< Non-Con: "));
        flag_rcv      = true;
        ACK           = false;
        cnt_cmdgetrsp = 0;
    }
}

/*-----------------------------------
  Get response data from BC95 Buffer
  -----------------------------------
*/
void Magellan_BC95_lite::miniresponse(String rx) {
    // if(debug) Serial.println("1_"+rx);
    print_rsp_header(rx);

    bool en_print = (post_process && resp_msgID == post_ID);

    String       payload_rx   = rx.substring(12, rx.length());
    String       data_payload = "";
    unsigned int indexff      = 0;

    indexff = payload_rx.indexOf(F("FF"));

    if (payload_rx.indexOf(F("FFF")) != -1) {
        data_payload = payload_rx.substring(indexff + 3, payload_rx.length());

        if (printstate && en_print)
            Serial.print(F("   RSP:"));
        // if(!more_flag) data_buffer="";                                          //clr buffer
        data_buffer = "";
        for (unsigned int k = 2; k < data_payload.length() + 1; k += 2) {
            char str = (char)strtol(&data_payload.substring(k - 2, k)[0], NULL, 16);
            if (printstate && en_print)
                Serial.print(str);

            if (GETCONTENT or RCVRSP) {
                if (post_process && post_token == rsptoken) {
                    data_buffer += str;
                }
            }
        }
        if (GETCONTENT) {
            rcvdata += data_buffer;
            // if(!more_flag) data_buffer="";
            data_buffer = "";
            getpayload  = true;
        }
        if (printstate && en_print)
            Serial.println(F(""));
    }
    else {
        data_payload = payload_rx.substring(indexff + 2, payload_rx.length());
        if (printstate && en_print)
            Serial.print(F("   RSP:"));
        // if(!more_flag) data_buffer="";
        data_buffer = ""; // clr buffer
        for (unsigned int k = 2; k < data_payload.length() + 1; k += 2) {
            char str = (char)strtol(&data_payload.substring(k - 2, k)[0], NULL, 16);
            if (printstate && en_print)
                Serial.print(str);

            if (GETCONTENT or RCVRSP) {
                if (post_process && post_token == rsptoken) {
                    data_buffer += str;
                }
            }
        }
        if (GETCONTENT) {
            rcvdata += data_buffer;
            // if(!more_flag) data_buffer="";
            data_buffer = "";
            getpayload  = true;
        }
        if (printstate && en_print)
            Serial.println(F(""));
    }

    if (success) {
        if (printstate && en_print)
            Serial.println(F("------------- End -------------"));
        count_timeout = 0;
        /*
        while(1){
          if(_Serial->available()){
            data_input=_Serial->readStringUntil('\n');
          }
          else{
            break;
          }
        }
        */
    }
}

void Magellan_BC95_lite::waitResponse() {
    unsigned long current = millis();
    if (en_chk_buff && (current - previous >= 500) && !(_Serial->available())) {
        _Serial->println(F("AT+NSORF=0,512"));
        cnt_cmdgetrsp++;
        previous = current;
    }

    if (_Serial->available()) {
        char data = (char)_Serial->read();
        if (data == '\n' || data == '\r') {
            if (k > 2) {
                end = true;
                k   = 0;
            }
            k++;
        }
        else {
            data_input += data;
        }
    }
    if (end) {
        if (debug)
            Serial.println(data_input);
        if (data_input.indexOf(F("+NSONMI:")) != -1) {
            if (debug)
                Serial.print(F("send_NSOMI "));
            if (data_input.indexOf(F("+NSONMI:")) != -1) {
                _Serial->println(F("AT+NSORF=0,512"));
                data_input = F("");
                send_NSOMI = true;
                if (printstate)
                    Serial.println();
            }
            end = false;
        }
        else {

            end = false;

            if (data_input.indexOf(F("0,119.31.104.1")) != -1) {
                int index1 = data_input.indexOf(F(","));
                if (index1 != -1) {
                    int index2 = data_input.indexOf(F(","), index1 + 1);
                    index1     = data_input.indexOf(F(","), index2 + 1);
                    index2     = data_input.indexOf(F(","), index1 + 1);
                    index1     = data_input.indexOf(F(","), index2 + 1);

                    if (debug)
                        Serial.println(data_input.substring(index2 + 1, index1));

                    if (cnt_rcv_resp >= 1 && data_input.substring(index2 + 1, index1).indexOf(F("FF")) != -1 &&
                        more_flag) {
                        miniresponse(data_input.substring(index2 + 1, index1));
                    }
                    else {
                        miniresponse(data_input.substring(index2 + 1, index1));
                    }

                    index2 = data_input.indexOf(F(","), index1 + 1);
                    if (data_input.substring(index1 + 1, index2) != "0") {
                        if (debug)
                            Serial.println(F("found buffer"));
                        _Serial->println(F("AT+NSORF=0,512"));
                        cnt_rcv_resp++;
                    }
                    else {
                        cnt_rcv_resp = 0;
                    }
                }
            }
            else if ((data_input.indexOf(F("ERROR")) != -1)) {
                if ((data_input.indexOf(F("+CME ERROR"))) != -1) {
                    int index1 = data_input.indexOf(F(":"));
                    if (index1 != -1) {
                        LastError = data_input.substring(index1 + 1, data_input.length());

                        if ((LastError.indexOf(F("159"))) != -1) {
                            Serial.print(F("Uplink busy"));
                            data_buffer = ""; // Ver 1.54  clr buffer
                        }
                    }
                }
            }
            else if ((data_input.indexOf(F("REBOOT"))) != -1) {
                breboot_flag = true;
            }

            else if (breboot_flag && data_input.indexOf(F("OK")) != -1) {
                Serial.print(F(">>HW RST"));
                post_process = false;
                breboot_flag = false;
                setup_module();
            }

            send_NSOMI = false;
            data_input = F("");
        }
    }
}
String Magellan_BC95_lite::rssi() {
    delay(200);
    _serial_flush();
    int    rssi     = 0;
    String data_csq = "";
    data_input      = "";
    _Serial->println(F("AT+CSQ"));
    delay(500);
    while (1) {
        if (_Serial->available()) {
            data_input = _Serial->readStringUntil('\n');
            // Serial.println(data_input);
            if (data_input.indexOf(F("OK")) != -1) {
                break;
            }
            else {
                if (data_input.indexOf(F("+CSQ")) != -1) {
                    int start_index = data_input.indexOf(F(":"));
                    int stop_index  = data_input.indexOf(F(","));
                    data_csq        = data_input.substring(start_index + 1, stop_index);
                    if (data_csq == "99") {
                        data_csq = "N/A";
                    }
                    else {
                        rssi     = data_csq.toInt();
                        rssi     = (2 * rssi) - 113;
                        data_csq = String(rssi);
                    }
                }
            }
        }
    }
    return data_csq;
}
