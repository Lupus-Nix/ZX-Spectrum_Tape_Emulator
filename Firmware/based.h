#ifndef BASED_H
#define BASED_H

//----------------------------------------------------------------------------------------------------
//����
//----------------------------------------------------------------------------------------------------
#define LANGUAGE_EN
//#define LANGUAGE_RU

//----------------------------------------------------------------------------------------------------
//������� �����������
//----------------------------------------------------------------------------------------------------
#define F_CPU 16000000UL

//----------------------------------------------------------------------------------------------------
//������������ ����������
//----------------------------------------------------------------------------------------------------

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//��������� ������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define BUTTON_UP_DDR      DDRD
#define BUTTON_UP_PORT     PORTD
#define BUTTON_UP_PIN      PIND
#define BUTTON_UP          3

#define BUTTON_CENTER_DDR  DDRD
#define BUTTON_CENTER_PORT PORTD
#define BUTTON_CENTER_PIN  PIND
#define BUTTON_CENTER      2

#define BUTTON_DOWN_DDR    DDRD
#define BUTTON_DOWN_PORT   PORTD
#define BUTTON_DOWN_PIN    PIND
#define BUTTON_DOWN        1

#define BUTTON_SELECT_DDR  DDRD
#define BUTTON_SELECT_PORT PORTD
#define BUTTON_SELECT_PIN  PIND
#define BUTTON_SELECT      4


//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void PressAnyKey(void);//�������� ������� ����� ������ ��� �����������


#endif

