#ifndef FAT_H
#define FAT_H

//----------------------------------------------------------------------------------------------------
//������������ ����������
//----------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>


//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
bool FAT_Init(void);//������������� FAT
bool FAT_BeginFileSearch(void);//������ ����� ����� � �������
bool FAT_PrevFileSearch(void);//��������� � ����������� ����� � ��������
bool FAT_NextFileSearch(void);//���������� ����� ����� � ��������
bool FAT_GetFileSearch(char *filename,uint32_t *FirstCluster,uint32_t *Size,int8_t *directory,int8_t *hidden,int8_t *system);//�������� ��������� �������� ���������� ����� � ��������
bool FAT_EnterDirectory(uint32_t FirstCluster);//����� � ���������� � ����� ������ ����
bool FAT_WriteBlock(uint16_t *BlockSize,uint16_t Block);//�������� � ��� ���� �����

#endif