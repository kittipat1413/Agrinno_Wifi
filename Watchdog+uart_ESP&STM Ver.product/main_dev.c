#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "CircularBuffer.h"
#include "ATparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void USART2_IRQHandler(void);

void init_usart1(void);
void init_usart2(void);
void init_iwdg(void);
void NVIC_Configuration(void);

void send_byte(uint8_t b);
void usart_puts(char* s);
void send_byte2(uint8_t b);
void usart_puts2(char *s);

// void clear_RXBuffer(void);
// void clear_dataBuffer(void);
// void copy_string(char *target, char *source); 
void delay(unsigned long ms);

int readFunc(uint8_t *data);
int writeFunc(uint8_t *buffer, size_t size);
bool readableFunc(void);
void sleepFunc(int us);


int count = 0;
int reset_cnt = 0;

char Debug_BUF[100];
uint8_t Command_BUF[10] = {'\0'};

// uint8_t RXC = 0;
// char RX_BUF[3] = {'\0'};
// char data_BUF[3] = {'\0'};

bool data_ready = false;
uint8_t State = 1;

circular_buf_t cbuf;
atparser_t parser;

// void clear_RXBuffer(void) {
// 	uint8_t RXi = 0;
// 	usart_puts("clear\n");
// 	usart_puts(RX_BUF);
// 	send_byte('\n');
// 	for (RXi=0; RXi<3; RXi++)
// 		RX_BUF[RXi] = '\0';

// 	RXC = 0;
// }

// void clear_dataBuffer(void) {
// 	uint8_t RXi = 0;
// 	usart_puts("clear\n");
// 	usart_puts(data_BUF);
// 	send_byte('\n');
// 	for (RXi=0; RXi<3; RXi++)
// 		data_BUF[RXi] = '\0';

// 	data_ready = false;
// }

// void copy_string(char *target, char *source) {
//     while (*source) {
//         *target = *source;
//         source++;
//         target++;
//     }
//     *target = '\0';
//     data_ready = true;
// }


void USART2_IRQHandler(void)
{
    char b;

    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART2);

      	// RX_BUF[RXC] = b;
    	  // RXC++;
        /* Uncomment this to loopback */
        // send_byte(b);
        // send_byte('\n');
        // usart_puts(RX_BUF);
        // send_byte('\n');

        circular_buf_put(&cbuf, b);

        switch (State)
      {
          case 1:
              
              if (b == '\r'){
                State++;  
              }
              break;
          
          case 2:
              
              if (b == '\n'){

                uint8_t command_size = circular_buf_size(&cbuf);
                if(command_size == 9){
                  
                  int ret = atparser_read(&parser, Command_BUF, 9);
                  sprintf(Debug_BUF, "\nCommand size : %d    Command:  %s \n", command_size,Command_BUF);
                  usart_puts(Debug_BUF);

                  if (ret != -1){
                    data_ready = true;
                  }
                }

                State = 1;
              }

              else{
                atparser_flush(&parser);
                usart_puts("Clear_RXBuffer");
                State = 1;
              }

              break;
          
          default:
              atparser_flush(&parser);
              usart_puts("Clear_RXBuffer");
              State = 1;
              break;
      }


        // if (RXC >= 2) {
        // 	copy_string(data_BUF,RX_BUF);
        // 	clear_RXBuffer();
        // }

	}
}

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 4; nCnt != 0; nCnt--)
      ;
}

void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1000; j++ );
}

void send_byte(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


void usart_puts(char* s)
{
    while(*s) {
    	send_byte(*s);
        s++;
    }
}


void send_byte2(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART2, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
    ;
}

void usart_puts2(char *s)
{
  while (*s)
  {
    send_byte2(*s);
    s++;
  }
}

void init_usart1(void)
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Tx as alternate function push-pull. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	USART_Cmd(USART1, ENABLE);

}

void init_usart2(void)
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Tx as alternate function push-pull. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART2, ENABLE);
}

void init_iwdg(void)
{
  /* Enable the LSI OSC */
  RCC_LSICmd(ENABLE);
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}

  /* Enable Watchdog*/
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_128); // 4, 8, 16 ... 256
  IWDG_SetReload(0x0FFF);//This parameter must be a number between 0 and 0x0FFF.
  IWDG_ReloadCounter();
  IWDG_Enable();

}


void NVIC_Configuration(void) {

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);

    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(4,1,0));

}

// ATparser callback function
int readFunc(uint8_t *data)
{
    return circular_buf_get(&cbuf, data);    
}

int writeFunc(uint8_t *buffer, size_t size)
{

    size_t i = 0;
    for (i = 0; i < size; i++)
    {
      send_byte2(buffer[i]);
    }
    
  return 0;
}

bool readableFunc()
{
    return !circular_buf_empty(&cbuf);
}

void sleepFunc(int us)
{
  Delay_1us(us);
}



int main(void)
{
  /* Initialize Leds mounted on STM32 board */
  GPIO_InitTypeDef  GPIO_InitStructure;
  /* Initialize Reset which connected to PA8, Enable the Clock*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Disable reset */
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

  // init_iwdg();
  init_usart1();
  init_usart2();
  NVIC_Configuration();


  circular_buf_init(&cbuf);
  atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);


  usart_puts("\r\nSTART\r\n");

  /* Reset ESP*/
  // GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  // delay(5000);
  // GPIO_SetBits(GPIOA, GPIO_Pin_8);

   /* Wait ESP ready*/
  // IWDG_ReloadCounter();
  // delay(5000);




  while (1)
  { 

   // IWDG_ReloadCounter(); 
   sprintf(Debug_BUF, "count is : %d", count);
   usart_puts(Debug_BUF);
   usart_puts("\n");
   sprintf(Debug_BUF, "reset cnt is : %d", reset_cnt);
   usart_puts(Debug_BUF);
   usart_puts("\n");

   // if (data_ready) {
   	
   // 		if(data_BUF[0] == 'O' && data_BUF[1] == 'K'){
   // 			count = 0;
   // 			clear_dataBuffer();
   // 		}
   // 		else{
   // 			usart_puts("\r\nNOT OK\r\n");
   // 			count++;
   // 			clear_RXBuffer();
   // 			clear_dataBuffer();
   // 		}
   // }

   // else{

   // 	count++;
   
   // }

   if (data_ready) {

      usart_puts("\n");

      usart_puts("Check \"GetData\" Command\n");
      if (!strcmp((char*)Command_BUF, "GetData\r\n")) {
        usart_puts("GET_DATA: OK\n");
        atparser_flush(&parser);
        data_ready = false;
        count = 0;
      }
      else{
        usart_puts("GET_DATA: Err\n");
      } 


      usart_puts("Check \"WiFierr\" Command\n");
      if (!strcmp((char*)Command_BUF, "WiFierr\r\n")) {
        usart_puts("WiFi_Error: OK\n");
        atparser_flush(&parser);
        data_ready = false;
        count = 0;
      }
      else{
        data_ready = false;
        usart_puts("WiFi_Error: Err\n");
      }

      usart_puts("\n");

   }

   else {
     count++;
   } 



   if(count >= 300){

   // IWDG_ReloadCounter();

   // clear_RXBuffer();
   // clear_dataBuffer(); 
   
   GPIO_ResetBits(GPIOA, GPIO_Pin_8);
   delay(5000);
   GPIO_SetBits(GPIOA, GPIO_Pin_8);
   reset_cnt++;
   count = 0;

   }

   delay(1000);

  }
}
