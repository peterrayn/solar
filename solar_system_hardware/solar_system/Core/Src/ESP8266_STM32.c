/*
 * Inspired by Controllerstech STM32 ESP8266 ThingSpeak example.
 */
#include "ESP8266_STM32.h"
#include "ctype.h"
#include "stdlib.h"
#include "GFX_FUNCTIONS.h"
#include "st7735.h"

ESP8266_ConnectionState ESP_ConnState = ESP8266_DISCONNECTED;   // Default state
static char esp_rx_buffer[4096];
static ESP8266_Status ESP_GetIP(char *ip_buffer, uint16_t buffer_len);
static ESP8266_Status ESP_SendCommand(const char *cmd, const char *ack, uint32_t timeout);
static char qs[512];
static char encoded_qs[1024];
static char body[1200];
static char request[1500];
uint32_t read_DATA[5]={0};

void get_mid_string(const char* start, const char* end, char* out) {
    out[0] = '\0'; //

    // 第一步：找起点
    char* p1 = strstr(esp_rx_buffer, start);
    if (p1 == NULL) return;      
    p1 = p1 + strlen(start);     

    // 第二步：找终点
    char* p2 = strstr(p1, end);
    if (p2 == NULL) return; 

    // 第三步：计算中间有多长，并拷贝到 out 数组里
    int len = p2 - p1;
    strncpy(out, p1, len);
    out[len] = '\0';     
}

void url_encode(const char *source, char *dest) {
    static const char *hex = "0123456789ABCDEF";

    while (*source) {
        unsigned char c = (unsigned char)*source;

   
     
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *dest++ = c;
        } else {
            // 转义为 %XX
            *dest++ = '%';
            *dest++ = hex[c >> 4];     // 高 4 位
            *dest++ = hex[c & 0x0F];   // 低 4 位
        }
        source++;
    }
    *dest = '\0'; 
}



ESP8266_Status ESP_Init(void)
{
	ESP8266_Status res;
//	USER_LOG("Initializing ESP8266...");
	fillScreen(BLACK);
	ST7735_WriteString(10,3,"Initializing ESP8266...",Font_11x18,WHITE,BLACK);
//	HAL_Delay(1000);

	res = ESP_SendCommand("AT+RST\r\n", "OK", 2000);
    if (res != ESP8266_OK){
//    	DEBUG_LOG("Failed to Reset ESP8266...");
    	fillScreen(BLACK);
    	ST7735_WriteString(10,3,"Failed to Reset ESP8266...",Font_11x18,WHITE,BLACK);
//    	printf("%d",res);
    	HAL_Delay(3000);
    	return res;
    }

//    USER_LOG("Waiting 3 Seconds for Reset to Complete...");
//    HAL_Delay(3000);  // wait for reset to complete

    res = ESP_SendCommand("AT\r\n", "OK", 500);
    if (res != ESP8266_OK){
//    	DEBUG_LOG("ESP8266 Not Responding...");
    	fillScreen(BLACK);
    	ST7735_WriteString(10,3,"ESP8266 Not Responding...",Font_11x18,WHITE,BLACK);
    	HAL_Delay(3000);
    	return res;
    }

    res = ESP_SendCommand("ATE0\r\n", "OK", 500); // Disable echo
    if (res != ESP8266_OK){
//    	DEBUG_LOG("Disable echo Command Failed...");
    	fillScreen(BLACK);
    	ST7735_WriteString(10,3,"Disable echo Command Failed...",Font_11x18,WHITE,BLACK);
    	HAL_Delay(3000);
    	return res;
    }
//    USER_LOG("ESP8266 Initialized Successfully...");
   	fillScreen(BLACK);
    	ST7735_WriteString(10,3,"ESP8266 Initialized Successfully",Font_11x18,WHITE,BLACK);

    	return ESP8266_OK;
}

ESP8266_Status ESP_ConnectWiFi(const char *ssid, const char *password, char *ip_buffer, uint16_t buffer_len)
{
//    USER_LOG("Setting in Station Mode");
    // Set in Station Mode
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWMODE=1\r\n");

    ESP8266_Status result = ESP_SendCommand(cmd, "OK", 500); // wait up to 2s
    if (result != ESP8266_OK)
    {
//    	USER_LOG("Station Mode Failed.");
       	fillScreen(BLACK);
        ST7735_WriteString(10,3,"Station Mode Failed.",Font_11x18,WHITE,BLACK);
        return result;
    }
    
//    USER_LOG("Connecting to WiFi SSID: %s", ssid);
   	fillScreen(BLACK);
    ST7735_WriteString(10,3,"Connecting to WiFi",Font_11x18,WHITE,BLACK);
    // Send join command
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

    result = ESP_SendCommand(cmd, "WIFI CONNECTED", 5000); // wait up to 10s
    if (result != ESP8266_OK)
    {
//    	USER_LOG("WiFi connection failed.");
       	fillScreen(BLACK);
        ST7735_WriteString(10,3,"WiFi connection failed.",Font_11x18,WHITE,BLACK);
        ESP_ConnState = ESP8266_NOT_CONNECTED;
        return result;
    }

    USER_LOG("WiFi Connected. Waiting for IP...");
    ESP_ConnState = ESP8266_CONNECTED_NO_IP;
    // Fetch IP with retries inside ESP_GetIP
    result = ESP_GetIP(ip_buffer, buffer_len);
    if (result != ESP8266_OK)
    {
    	USER_LOG("Failed to fetch IP. Status=%d", result);
        return result;
    }

    USER_LOG("WiFi + IP ready: %s", ip_buffer);
    return ESP8266_OK;
}

ESP8266_Status stupid_SCAU(const char *ssid)
{
//    USER_LOG("Setting in Station Mode");
    // Set in Station Mode
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "AT+CWMODE=1\r\n");

    ESP8266_Status result = ESP_SendCommand(cmd, "OK", 500); // wait up to 2s
    if (result != ESP8266_OK)
    {
      	fillScreen(BLACK);
            ST7735_WriteString(10,3,"Station Mode Failed.",Font_11x18,WHITE,BLACK);
            HAL_Delay(3000);
            return result;
    }

  	fillScreen(BLACK);
        ST7735_WriteString(10,3,"connecting wifi...",Font_11x18,WHITE,BLACK);
    // Send join command
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"\"\r\n", ssid);

    result = ESP_SendCommand(cmd, "WIFI CONNECTED", 5000); // wait up to 10s
    if (result != ESP8266_OK)
    {
      	fillScreen(BLACK);
            ST7735_WriteString(10,3,"WIFI connect Failed.",Font_11x18,WHITE,BLACK);
        ESP_ConnState = ESP8266_NOT_CONNECTED;
        HAL_Delay(3000);
        return result;
    }



  	fillScreen(BLACK);
        ST7735_WriteString(10,3,"Redirecting...",Font_11x18,WHITE,BLACK);
    sprintf(cmd,"AT+CIPSTART=\"TCP\",\"%s\",80\r\n","1.1.1.1");
    result = ESP_SendCommand(cmd, "OK", 500); // wait up to 10s
    if (result != ESP8266_OK)
    {

    	fillScreen(BLACK);
            ST7735_WriteString(10,3,"failed to redirecting...AT+CIPSTART",Font_11x18,WHITE,BLACK);
            HAL_Delay(3000);
            return result;
    }


    result = ESP_SendCommand("AT+CIPSEND=52\r\n", ">", 200); // wait up to 10s
    if (result != ESP8266_OK)
    {
       	fillScreen(BLACK);
                ST7735_WriteString(10,3,"failed to redirecting...CIPSEND",Font_11x18,WHITE,BLACK);
                HAL_Delay(3000);
                return result;
    }


    result = ESP_SendCommand("GET / HTTP/1.1\r\nHost: 1.1.1.1\r\nConnection: close\r\n\r\n", "CLOSED", 5000); // wait up to 10s
    if (result != ESP8266_OK)
    {
        if (strstr(esp_rx_buffer, "html"))
        {
//        	USER_LOG("already authenticated");
           	fillScreen(BLACK);
                    ST7735_WriteString(10,3,"already authenticated",Font_11x18,WHITE,BLACK);
        	return ESP8266_OK; // mark as found but keep reading
        }

       	fillScreen(BLACK);
                ST7735_WriteString(10,3,"failed to redirecting...GET",Font_11x18,WHITE,BLACK);
//    	printf("%d",result);
                HAL_Delay(3000);
                return result;

    }


    get_mid_string("index.jsp?","'</script>", qs);

    if (strlen(qs)<1) {
//    	USER_LOG("already authenticated");
       	fillScreen(BLACK);
                ST7735_WriteString(10,3,"already authenticated",Font_11x18,WHITE,BLACK);

                return ESP8266_OK;}

    url_encode(qs, encoded_qs);

    sprintf (body, "",encoded_qs);
   uint16_t body_len=strlen(body);
    sprintf (request, "POST /eportal/InterFace.do?method=login HTTP/1.1\r\nHost: 192.168.253.3\r\nConnection: close\r\nContent-Type: application/x-www-form-urlencoded\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/146.0.0.0 Safari/537.36 Edg/146.0.0.0\r\nContent-Length: %u\r\n\r\n%s",body_len,body);
    uint16_t request_len=strlen(request);


    result = ESP_SendCommand("AT+CIPSTART=\"TCP\",\"192.168.253.3\",80\r\n", "OK", 2000); // wait up to 2s
    if (result != ESP8266_OK)
    {
//    	USER_LOG("can not build connection with guild");
     	fillScreen(BLACK);
                ST7735_WriteString(10,3,"can not build connection with guild",Font_11x18,WHITE,BLACK);
                HAL_Delay(3000);
                return result;
    }


    sprintf(cmd,"AT+CIPSEND=%u\r\n",request_len);
    result = ESP_SendCommand(cmd, ">", 500); // wait up to 2s
    if (result != ESP8266_OK)
    {
//    	USER_LOG("fail cipsend");
     	fillScreen(BLACK);
                ST7735_WriteString(10,3,"fail cipsend=num to guild",Font_11x18,WHITE,BLACK);
                HAL_Delay(3000);
                return result;
    }

//    USER_LOG("prepare to send REQUEST!");
 	fillScreen(BLACK);
            ST7735_WriteString(10,3,"prepare to send REQUEST!",Font_11x18,WHITE,BLACK);

    result = ESP_SendCommand(request, "success", 5000); // wait up to 2s
    if (result != ESP8266_OK)
    {
//    	USER_LOG("Fail wait guild close");
     	fillScreen(BLACK);
                ST7735_WriteString(10,3,"Fail to get success",Font_11x18,WHITE,BLACK);
                HAL_Delay(3000);
                return result;

    }

//    USER_LOG("successful authentication");
   	fillScreen(BLACK);
                ST7735_WriteString(10,3,"successful authentication",Font_11x18,GREEN,BLACK);

                return ESP8266_OK;

}





ESP8266_Status Interaction(char *VPS_IP,uint16_t VPS_port,float t1,float t2,float t3,float t4,float stm,float illum,float current,float power,float voltage){
//	USER_LOG("Ready to connect VPS...");

	char cmd[128];
	char msg[512];

	sprintf(cmd,"AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",VPS_IP,VPS_port);

	    ESP8266_Status result = ESP_SendCommand(cmd, "OK", 1000); // wait up to 2s
	    if (result != ESP8266_OK)
	    {
	       	fillScreen(BLACK);
	                    ST7735_WriteString(10,3,"can not build connection with VPS.",Font_11x18,GREEN,BLACK);

	                    HAL_Delay(3000);
	        return result;
	    }




	sprintf(msg,"GET /dataupload?t1=%.2f&t2=%.2f&t3=%.2f&t4=%.2f&stm=%.2f&illum=%.2f&current=%.2f&power=%.2f&voltage=%.2f&pass=123456 HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",t1,t2,t3,t4,stm,illum,current, power,voltage,VPS_IP);
//	sprintf(msg,"GET /dataupload?msg=fuckyou HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",VPS_IP);
    sprintf(cmd, "AT+CIPSEND=%u\r\n", (uint16_t)strlen(msg));



    result = ESP_SendCommand(cmd, ">", 500);
    if (result != ESP8266_OK) {
       	fillScreen(BLACK);
                    ST7735_WriteString(10,3,"VPS-->AT+CIPSEND failed",Font_11x18,GREEN,BLACK);
                    HAL_Delay(3000);
        return result;
    }


    result = ESP_SendCommand(msg, "CLOSED", 3000);
    if (result != ESP8266_OK) {
//        USER_LOG("Failed to send HTTP request.");
    	fillScreen(BLACK);
        ST7735_WriteString(10,3,"VPS-->Failed to send HTTP request.",Font_11x18,GREEN,BLACK);
        HAL_Delay(3000);

        return result;
    }


    memset(msg, 0, sizeof(msg));
    get_mid_string("hearme?","??", msg);

    char *p=msg;
    char temp[10]="";
    uint8_t idx=0,IDx=0;
    while (1){
        if (*p=='\0'){
            read_DATA[IDx]=atoi(temp);
            break;
        }
    	if (*p=='&'){
    		read_DATA[IDx]=atoi(temp);
    		IDx++;
    		idx=0;
    		p++;
    	}
    	temp[idx]=*p;
    	temp[idx+1]='\0';
    	p++;idx++;
    }


//    USER_LOG("send message successful...");


    return ESP8266_OK;


}




ESP8266_ConnectionState ESP_GetConnectionState(void)
{
    return ESP_ConnState;
}

static ESP8266_Status ESP_GetIP(char *ip_buffer, uint16_t buffer_len)
{
	DEBUG_LOG("Fetching IP Address...");

    for (int attempt = 1; attempt <= 3; attempt++)
    {
        ESP8266_Status result = ESP_SendCommand("AT+CIFSR\r\n", "OK", 5000);
        if (result != ESP8266_OK)
        {
        	DEBUG_LOG("CIFSR failed on attempt %d", attempt);
            continue;
        }

        char *search = esp_rx_buffer;
        char *last_ip = NULL;

        while ((search = strstr(search, "STAIP,")) != NULL)
        {
            char *ip_start = strstr(search, "STAIP,\"");
            if (ip_start)
            {
                ip_start += 7;
                char *end = strchr(ip_start, '"');
                if (end && ((end - ip_start) < buffer_len))
                {
                    last_ip = ip_start;
                }
            }
            search += 6;
        }

        if (last_ip)
        {
            char *end = strchr(last_ip, '"');
            strncpy(ip_buffer, last_ip, end - last_ip);
            ip_buffer[end - last_ip] = '\0';

            if (strcmp(ip_buffer, "0.0.0.0") == 0)
            {
            	DEBUG_LOG("Attempt %d: IP not ready yet (0.0.0.0). Retrying...", attempt);
                ESP_ConnState = ESP8266_CONNECTED_NO_IP;
                HAL_Delay(1000);
                continue;
            }

            DEBUG_LOG("Got IP: %s", ip_buffer);
            ESP_ConnState = ESP8266_CONNECTED_IP;
            return ESP8266_OK;
        }

        DEBUG_LOG("Attempt %d: Failed to parse STAIP.", attempt);
        HAL_Delay(500);
    }

    DEBUG_LOG("Failed to fetch IP after retries.");
    ESP_ConnState = ESP8266_CONNECTED_NO_IP;  // still connected, but no IP
    return ESP8266_ERROR;
}



static ESP8266_Status ESP_SendCommand(const char *cmd, const char *ack, uint32_t timeout)
{
    uint8_t ch;
    uint16_t idx = 0;
    uint32_t tickstart;
    int found = 0;

    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    tickstart = HAL_GetTick();

    if (strlen(cmd) > 0)
    {
//        DEBUG_LOG("Sending: %s", cmd);
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t*)cmd, strlen(cmd), HAL_MAX_DELAY) != HAL_OK)
            return ESP8266_ERROR;
    }

    while ((HAL_GetTick() - tickstart) < timeout && idx < sizeof(esp_rx_buffer) - 1)
    {
        if (HAL_UART_Receive(&ESP_UART, &ch, 1, 10) == HAL_OK)
        {
            esp_rx_buffer[idx++] = ch;
          }

   }
    esp_rx_buffer[idx]   = '\0';


            // check for ACK
            if (strstr(esp_rx_buffer, ack))
            {
//                DEBUG_LOG("Matched ACK: %s", ack);
                found = 1; // mark as found but keep reading
            }




    if (found)
    {
//        DEBUG_LOG("Full buffer: %s", esp_rx_buffer);
        return ESP8266_OK;
    }

    if (idx == 0)
        return ESP8266_NO_RESPONSE;

//    DEBUG_LOG("Timeout or no ACK. Buffer: %s", esp_rx_buffer);
    return ESP8266_TIMEOUT;
}
