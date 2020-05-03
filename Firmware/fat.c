//----------------------------------------------------------------------------------------------------
//������������ ����������
//----------------------------------------------------------------------------------------------------
#include "fat.h"
#include "based.h"
#include "sd.h"
#include "dram.h"
#include "wh1602.h"

//----------------------------------------------------------------------------------------------------
//����������������
//----------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//�������� � FAT
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//BOOT-������ � ��������� BPB
#define BS_jmpBoot			0
#define BS_OEMName			3
#define BPB_BytsPerSec		11
#define BPB_SecPerClus		13
#define BPB_ResvdSecCnt		14
#define BPB_NumFATs			16
#define BPB_RootEntCnt		17
#define BPB_TotSec16		19
#define BPB_Media			21
#define BPB_FATSz16			22
#define BPB_SecPerTrk		24
#define BPB_NumHeads		26
#define BPB_HiddSec			28
#define BPB_TotSec32		32
#define BS_DrvNum			36
#define BS_Reserved1		37
#define BS_BootSig			38
#define BS_VolID			39
#define BS_VolLab			43
#define BS_FilSysType		54
#define BPB_FATSz32			36
#define BPB_ExtFlags		40
#define BPB_FSVer			42
#define BPB_RootClus		44
#define BPB_FSInfo			48
#define BPB_BkBootSec		50
#define BPB_Reserved		52

//��� �������� �������
#define FAT12 0
#define FAT16 1
#define FAT32 2

//��������� �������
#define FAT16_EOC 0xFFF8UL

//�������� �����
#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN 		0x02
#define ATTR_SYSTEM 		0x04
#define ATTR_VOLUME_ID 		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE  		0x20
#define ATTR_LONG_NAME 		(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

//��������� MBR

//��������� ������� ��������
#define MBR_TABLE_OFFSET 446
//������ ������
#define MBR_SIZE_OF_PARTITION_RECORD 16
//�������� ������� �������
#define MBR_START_IN_LBA 8

//----------------------------------------------------------------------------------------------------
//���������
//----------------------------------------------------------------------------------------------------

static const char Text_FAT_Type[] PROGMEM =  "��� �. �������  \0";
static const char Text_FAT32[] PROGMEM =     "FAT32- ������!  \0";
static const char Text_FAT16[] PROGMEM =     "FAT16- ��.      \0";
static const char Text_FAT12[] PROGMEM =     "FAT12- ������!  \0";
static const char Text_NoFAT[] PROGMEM =     " FAT  �� �������\0";
static const char Text_MBR_Found[] PROGMEM = "   ������ MBR   \0";

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------

extern char String[25];//������



uint8_t Sector[256];//������ ��� �������
uint32_t LastReadSector=0xffffffffUL;//��������� ��������� ������
uint32_t FATOffset=0;//�������� FAT

//��������� ���� ������ ������ ������ ��������
struct SFATRecordPointer
{
 uint32_t BeginFolderAddr;//��������� ����� ��� ������ ������ ����������
 uint32_t CurrentFolderAddr;//������� ����� ��� ������ ������ ����������
 uint32_t BeginFolderCluster;//��������� ������� ����� ����� ������ ����������
 uint32_t CurrentFolderCluster;//������� ������� ����� ����� ������ ����������
 uint32_t BeginFolderClusterAddr;//��������� ����� �������� ��������
 uint32_t EndFolderClusterAddr;//�������� ����� ��� ������ ������ ���������� (��� ��������)
} sFATRecordPointer;

uint32_t FirstRootFolderAddr;//��������� ����� �������� ����������

uint32_t SecPerClus;//���������� �������� � ��������
uint32_t BytsPerSec;//���������� ���� � �������
uint32_t ResvdSecCnt;//������ ��������� �������

uint32_t FATSz;//������ ������� FAT
uint32_t DataSec;//���������� �������� � ������� ������ �����
uint32_t RootDirSectors;//���������� ��������, ������� �������� ����������� 
uint32_t CountofClusters;//���������� ��������� ��� ������ (������� ���������� � ������ 2! ��� ����������, � �� ����� ���������� ��������)
uint32_t FirstDataSector;//������ ������ ������
uint32_t FirstRootFolderSecNum;//������ �������� ���������� (��� FAT16 - ��� ������ � ��������� �������, ��� FAT32 - ��� ���� � ������� ������ � ��������� BPB_RootClus)
uint32_t ClusterSize;//������ �������� � ������

uint8_t FATType=FAT12;//��� �������� �������

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
uint32_t GetByte(uint32_t offset);//������� ����
uint32_t GetShort(uint32_t offset);//������� ��� �����
uint32_t GetLong(uint32_t offset);//������� 4 �����
bool FAT_RecordPointerStepForward(struct SFATRecordPointer *sFATRecordPointerPtr);//������������� �� ������ �����
bool FAT_RecordPointerStepReverse(struct SFATRecordPointer *sFATRecordPointerPtr);//������������� �� ������ �����

//----------------------------------------------------------------------------------------------------
//������� ����
//----------------------------------------------------------------------------------------------------
uint32_t GetByte(uint32_t offset)
{
 offset+=FATOffset;
 uint32_t s=offset>>8UL;//����� �� 256
 if (s!=LastReadSector)
 {
  LastReadSector=s;
  bool first=true;
  if ((offset&0x1ffUL)>=256) first=false;
  SD_ReadBlock(s>>1UL,Sector,first);
  //������ �� ���������, �� ����� ������ ������� �� ������ - ���� ��������, ���� ���
 }
 return(Sector[offset&0xFFUL]);
}
//----------------------------------------------------------------------------------------------------
//������� ��� �����
//----------------------------------------------------------------------------------------------------
uint32_t GetShort(uint32_t offset)
{
 uint32_t v=GetByte(offset+1UL);
 v<<=8UL;
 v|=GetByte(offset);
 return(v);
}
//----------------------------------------------------------------------------------------------------
//������� 4 �����
//----------------------------------------------------------------------------------------------------
uint32_t GetLong(uint32_t offset)
{
 uint32_t v=GetByte(offset+3UL);
 v<<=8UL;
 v|=GetByte(offset+2UL);
 v<<=8UL;
 v|=GetByte(offset+1UL);
 v<<=8UL;
 v|=GetByte(offset);
 return(v);
}

//----------------------------------------------------------------------------------------------------
//������������� �� ������ �����
//----------------------------------------------------------------------------------------------------
bool FAT_RecordPointerStepForward(struct SFATRecordPointer *sFATRecordPointerPtr)
{
 sFATRecordPointerPtr->CurrentFolderAddr+=32UL;//��������� � ��������� ������ 
 if (sFATRecordPointerPtr->CurrentFolderAddr>=sFATRecordPointerPtr->EndFolderClusterAddr)//����� �� ������� �������� ��� ����������
 {
  if (sFATRecordPointerPtr->BeginFolderAddr==FirstRootFolderAddr)//���� � ��� ����������� �������� ����������
  {
   return(false);
  }  
  else//��� �� �������� ���������� ����� ����� ����� ��������
  {
   uint32_t FATClusterOffset=0;//�������� �� ������� FAT � ������ (� FAT32 ��� 4-� �������, � � FAT16 - �����������)
   if (FATType==FAT16) FATClusterOffset=sFATRecordPointerPtr->CurrentFolderCluster*2UL;//����� �������� � ������� FAT
   uint32_t NextClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//����� ���������� ��������
   //��������� ����� ���������� �������� �����
   uint32_t NextCluster=0;
   if (FATType==FAT16) NextCluster=GetShort(NextClusterAddr);
   if (NextCluster==0 || NextCluster>=CountofClusters+2UL || NextCluster>=FAT16_EOC)//������ �������� ���
   {
    return(false);        	
   }
   sFATRecordPointerPtr->CurrentFolderCluster=NextCluster;//��������� � ���������� ��������
   uint32_t FirstSectorofCluster=((sFATRecordPointerPtr->CurrentFolderCluster-2UL)*SecPerClus)+FirstDataSector; 
   sFATRecordPointerPtr->CurrentFolderAddr=FirstSectorofCluster*BytsPerSec; 
   sFATRecordPointerPtr->BeginFolderClusterAddr=sFATRecordPointerPtr->CurrentFolderAddr;
   sFATRecordPointerPtr->EndFolderClusterAddr=sFATRecordPointerPtr->CurrentFolderAddr+SecPerClus*BytsPerSec;
  }
 }
 return(true);
}
//----------------------------------------------------------------------------------------------------
//������������� �� ������ �����
//----------------------------------------------------------------------------------------------------
bool FAT_RecordPointerStepReverse(struct SFATRecordPointer *sFATRecordPointerPtr)
{
 sFATRecordPointerPtr->CurrentFolderAddr-=32UL;//������������ �� ������ ����� 
 if (sFATRecordPointerPtr->CurrentFolderAddr<sFATRecordPointerPtr->BeginFolderClusterAddr)//����� �� ������ ������� ��������
 {
  if (sFATRecordPointerPtr->BeginFolderAddr==FirstRootFolderAddr)//���� � ��� �������� ����������
  {
   return(false);//����� �� ������� ����������   
  }  
  else//��� �� �������� ���������� ����� ����� �����
  {
   uint32_t PrevCluster=sFATRecordPointerPtr->BeginFolderCluster;//���������� �������   
   while(1)
   {
    uint32_t FATClusterOffset=0;//�������� �� ������� FAT � ������ (� FAT32 ��� 4-� �������, � � FAT16 - �����������)
    if (FATType==FAT16) FATClusterOffset=PrevCluster*2UL;
    uint32_t ClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//����� ����������� ��������
	uint32_t cluster=GetShort(ClusterAddr);
    if (cluster<=2 || cluster>=FAT16_EOC)//������ �������� ���
	{
     return(false);//����� �� ������� ����������        	
	}
	if (cluster==sFATRecordPointerPtr->CurrentFolderCluster) break;//�� ����� �������������� �������
	PrevCluster=cluster;
   }
   if (PrevCluster<=2 || PrevCluster>=FAT16_EOC)//������ �������� ���
   {
    return(false);//����� �� ������� ����������        	
   }
   sFATRecordPointerPtr->CurrentFolderCluster=PrevCluster;//��������� � ����������� ��������
   uint32_t FirstSectorofCluster=((sFATRecordPointerPtr->CurrentFolderCluster-2UL)*SecPerClus)+FirstDataSector; 
   sFATRecordPointerPtr->BeginFolderClusterAddr=FirstSectorofCluster*BytsPerSec; 	
   sFATRecordPointerPtr->EndFolderClusterAddr=sFATRecordPointerPtr->BeginFolderClusterAddr+SecPerClus*BytsPerSec;
   sFATRecordPointerPtr->CurrentFolderAddr=sFATRecordPointerPtr->EndFolderClusterAddr-32UL;//�� ������ �����
  }
 }
 return(true);
}

//----------------------------------------------------------------------------------------------------
//������������� FAT
//----------------------------------------------------------------------------------------------------
void FAT_Init(void)
{
 WH1602_SetTextUpLine("");
 WH1602_SetTextDownLine("");
 //���� ������ � FAT16
 LastReadSector=0xffffffffUL; 
 FATOffset=0;
 
 uint8_t b;
 b=GetByte(0); 
 if (b==0xEB || b==0xE9)//��� ����� �� MBR
 {
  if (GetByte(510UL)==0x55 && GetByte(511UL)==0xAA)//������ ������ � �� ������������
  {
  }
  else//������ �� ������
  {
   WH1602_SetTextProgmemDownLine(Text_NoFAT);
   while(1);  
  }
 }
 else//���, ��������, MBR
 {
  if (GetByte(510UL)==0x55 && GetByte(511UL)==0xAA)//��� MBR, ��������� ������ ������
  {  
   WH1602_SetTextProgmemUpLine(Text_MBR_Found);
   _delay_ms(2000);  
  
   uint8_t offset[4];
   size_t partition;
   for(partition=0;partition<4;partition++)
   {
    FATOffset=0;	
    offset[0]=GetByte(MBR_TABLE_OFFSET+MBR_SIZE_OF_PARTITION_RECORD*partition+MBR_START_IN_LBA+0);
    offset[1]=GetByte(MBR_TABLE_OFFSET+MBR_SIZE_OF_PARTITION_RECORD*partition+MBR_START_IN_LBA+1);
    offset[2]=GetByte(MBR_TABLE_OFFSET+MBR_SIZE_OF_PARTITION_RECORD*partition+MBR_START_IN_LBA+2);
    offset[3]=GetByte(MBR_TABLE_OFFSET+MBR_SIZE_OF_PARTITION_RECORD*partition+MBR_START_IN_LBA+3);
    FATOffset=*((uint32_t*)offset);
    FATOffset*=512UL;
    b=GetByte(0);
    if ((b==0xEB || b==0xE9) && GetByte(510UL)==0x55 && GetByte(511UL)==0xAA) break;//������ ������
   }
   if (partition==4)//������ �� ������
   {
    WH1602_SetTextProgmemDownLine(Text_NoFAT);
    while(1);  
   }   
  }
  else//��� �� MBR
  {
   WH1602_SetTextProgmemDownLine(Text_NoFAT);
   while(1);  
  }
 } 
 
 LastReadSector=0xffffffffUL;
 
 SecPerClus=GetByte(BPB_SecPerClus);//���������� �������� � ��������
 BytsPerSec=GetShort(BPB_BytsPerSec);//���������� ���� � �������
 ResvdSecCnt=GetShort(BPB_ResvdSecCnt);//������ ��������� �������
 
 //���������� ���������� ��������, ������� �������� ����������� 
 RootDirSectors=(uint32_t)(ceil((GetShort(BPB_RootEntCnt)*32UL+(BytsPerSec-1UL))/BytsPerSec));
 //���������� ������ ������� FAT
 FATSz=GetShort(BPB_FATSz16);//������ ����� ������� FAT � ��������
 if (FATSz==0) FATSz=GetLong(BPB_FATSz32);
 //���������� ���������� �������� � ������� ������ �����
 uint32_t TotSec=GetShort(BPB_TotSec16);//����� ���������� �������� �� �����
 if (TotSec==0) TotSec=GetLong(BPB_TotSec32);
 DataSec=TotSec-(ResvdSecCnt+GetByte(BPB_NumFATs)*FATSz+RootDirSectors);
 //���������� ���������� ��������� ��� ������ (������� ���������� � ������ 2! ��� ����������, � �� ����� ���������� ��������)
 CountofClusters=(uint32_t)floor(DataSec/SecPerClus);
 //���������� ������ ������ ������
 FirstDataSector=ResvdSecCnt+(GetByte(BPB_NumFATs)*FATSz)+RootDirSectors;
 //��������� ��� �������� �������

 FATType=FAT12;
 WH1602_SetTextProgmemUpLine(Text_FAT_Type);
 if (CountofClusters<4085UL)
 {
  WH1602_SetTextProgmemDownLine(Text_FAT12);
  while(1);
 }
 else
 {
  if (CountofClusters<65525UL)
  {
   WH1602_SetTextProgmemDownLine(Text_FAT16);
   _delay_ms(2000);
   FATType=FAT16;
  }
  else
  {
   WH1602_SetTextProgmemDownLine(Text_FAT32);
   FATType=FAT32;
   while(1);   
  }
 }
 if (FATType==FAT12) return;//�� ������������
 if (FATType==FAT32) return;//�� ������������
 //���������� ������ �������� ���������� (��� FAT16 - ��� ������ � ��������� �������, ��� FAT32 - ��� ���� � ������� ������ � ��������� BPB_RootClus)
 FirstRootFolderSecNum=ResvdSecCnt+(GetByte(BPB_NumFATs)*FATSz);
 ClusterSize=SecPerClus*BytsPerSec;//������ �������� � ������

 //������ �������� ����������
 FirstRootFolderAddr=FirstRootFolderSecNum*BytsPerSec;//��������� ����� �������� ����������
 //����������� ��������� ��� ������ ������ ����������
 sFATRecordPointer.BeginFolderAddr=FirstRootFolderAddr;//��������� ����� ��� ������ ������ ����������
 sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;//������� ����� ��� ������ ������ ����������
 sFATRecordPointer.BeginFolderCluster=0;//��������� ������� ����� ����� ������ ����������
 sFATRecordPointer.CurrentFolderCluster=0;//������� ������� ����� ����� ������ ����������
 sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//�������� ����� ��� ������ ������ ���������� (��� ��������)
 sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.CurrentFolderAddr;//����� ���������� �������� ����������
}
//----------------------------------------------------------------------------------------------------
//������ ����� ����� � �������
//----------------------------------------------------------------------------------------------------
bool FAT_BeginFileSearch(void)
{
 uint32_t FirstCluster;//������ ������� �����
 uint32_t Size;//������ �����
 int8_t Directory;//�� ���������� �� ����
 
 sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;
 sFATRecordPointer.CurrentFolderCluster=sFATRecordPointer.BeginFolderCluster;
 sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.CurrentFolderAddr;
 if (sFATRecordPointer.BeginFolderAddr!=FirstRootFolderAddr)//��� �� �������� ����������
 {
  sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+SecPerClus*BytsPerSec;
 }
 else sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//�������� ����� ��� ������ ������ ���������� (��� ��������)
 //��������� � ������� ������� ��� �����
 while(1)
 {
  if (FAT_GetFileSearch(NULL,&FirstCluster,&Size,&Directory)==false)
  {
   if (FAT_NextFileSearch()==false) return(false);
  } 
  else return(true);
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//������� � ����������� ����� � ��������
//----------------------------------------------------------------------------------------------------
bool FAT_PrevFileSearch(void)
{
 struct SFATRecordPointer sFATRecordPointer_Copy=sFATRecordPointer;
 while(1)
 {
  if (FAT_RecordPointerStepReverse(&sFATRecordPointer_Copy)==false) return(false);  
  //����������� ��� �����
  uint8_t n;
  bool res=true;
  for(n=0;n<11;n++)
  {
   uint8_t b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+(uint32_t)(n));
   if (n==0)
   {
    if (b==0x20 || b==0xE5)
	{
     res=false;
     break;	
	}
   }
   if (b<0x20)
   {
    res=false;
    break;
   }
   if (n==1)
   {
    uint8_t a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr);
    uint8_t b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+1UL);
    if (a==(uint8_t)'.' && b!=(uint8_t)'.')
    {
     res=false; 
	 break;
    }	
   }   
  }
  //������� ����������
  if (res==true)
  {  
   uint8_t type=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+11UL);
   if (type&ATTR_VOLUME_ID) continue;//���� ���� - ��� �����     
   if ((type&ATTR_DIRECTORY)==0)//��� ����
   {
    uint8_t a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+10UL);
    uint8_t b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+9UL);
    uint8_t c=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+8UL);
    if (!(a=='P' && b=='A' && c=='T')) continue;//�������� ����������
   }
   sFATRecordPointer=sFATRecordPointer_Copy;
   return(true);
  }
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//������� � ���������� ����� � ��������
//----------------------------------------------------------------------------------------------------
bool FAT_NextFileSearch(void)
{
 struct SFATRecordPointer sFATRecordPointer_Copy=sFATRecordPointer;
 while(1)
 {
  if (FAT_RecordPointerStepForward(&sFATRecordPointer_Copy)==false) return(false); 
  uint8_t n;
  bool res=true;
  for(n=0;n<11;n++)
  {
   uint8_t b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+(uint32_t)(n));
   if (n==0)
   {
    if (b==0x20 || b==0xE5)
	{
     res=false;
     break;	
	}
   }
   if (b<0x20)
   {
    res=false;
    break;
   }
   if (n==1)
   {
    uint8_t a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr);
    uint8_t b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+1UL);
    if (a==(uint8_t)'.' && b!=(uint8_t)'.')
    {
     res=false; 
	 break;
    }	
   }     
  }
  if (res==true)
  {
   uint8_t type=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+11UL);
   if (type&ATTR_VOLUME_ID) continue;//���� ���� - ��� �����     
   if ((type&ATTR_DIRECTORY)==0)//��� ����
   {
    uint8_t a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+10UL);
    uint8_t b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+9UL);
    uint8_t c=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+8UL);
    if (!(a=='P' && b=='A' && c=='T')) continue;//�������� ����������
   }
   sFATRecordPointer=sFATRecordPointer_Copy;
   return(true);
  }
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//�������� ��������� �������� ���������� ����� � ��������
//----------------------------------------------------------------------------------------------------
bool FAT_GetFileSearch(char *filename,uint32_t *FirstCluster,uint32_t *Size,int8_t *directory)
{
 uint8_t n;
 bool res=true;
 *directory=0;
 if (filename!=NULL)
 {
  for(n=0;n<11;n++) filename[n]=32;
 }
 for(n=0;n<11;n++)
 {    
  uint8_t b=GetByte(sFATRecordPointer.CurrentFolderAddr+(uint32_t)(n));
  if (n==0)
  {
   if (b==0x20 || b==0xE5)
   {
    res=false;
    break;	
   }
  }
  if (b<0x20)
  {
   res=false;
   break;
  }
  if (filename!=NULL)
  {
   if (n<8) filename[n]=b;
       else filename[n+1]=b;
  }
  if (n==1)
  {
   uint8_t a=GetByte(sFATRecordPointer.CurrentFolderAddr);
   uint8_t b=GetByte(sFATRecordPointer.CurrentFolderAddr+1UL);
   if (a==(uint8_t)'.' && b!=(uint8_t)'.')
   {
    res=false; 
    break;
   }	
  }     
 }
 if (res==true)
 {
  uint8_t type=GetByte(sFATRecordPointer.CurrentFolderAddr+11UL);  
  if (type&ATTR_VOLUME_ID) return(false);//���� ���� - ��� �����  
  if ((type&ATTR_DIRECTORY)==0)//��� ����
  {
   uint8_t a=GetByte(sFATRecordPointer.CurrentFolderAddr+10UL);
   uint8_t b=GetByte(sFATRecordPointer.CurrentFolderAddr+9UL);
   uint8_t c=GetByte(sFATRecordPointer.CurrentFolderAddr+8UL);
   if (!(a=='P' && b=='A' && c=='T')) return(false);//�������� ����������
  }
  else//���� ��� ����������
  {
   uint8_t a=GetByte(sFATRecordPointer.CurrentFolderAddr);
   uint8_t b=GetByte(sFATRecordPointer.CurrentFolderAddr+1UL);  
   if (a==(uint8_t)'.' && b==(uint8_t)'.') *directory=-1;//�� ���������� ����
                                        else *directory=1;//�� ���������� ����
  } 
  //������ ������� �����  
  *FirstCluster=(GetShort(sFATRecordPointer.CurrentFolderAddr+20UL)<<16)|GetShort(sFATRecordPointer.CurrentFolderAddr+26UL);
  //����� ������ ����� � ������
  *Size=GetLong(sFATRecordPointer.CurrentFolderAddr+28UL);
  if (filename!=NULL)
  {
   if ((type&ATTR_DIRECTORY)==0) filename[8]='.';//����� ��������� �����    
   filename[12]=0;   
   //������ ������� ��� �����   
   struct SFATRecordPointer sFATRecordPointer_Local=sFATRecordPointer;
   uint8_t long_name_length=0;
   while(1)
   {
    if (FAT_RecordPointerStepReverse(&sFATRecordPointer_Local)==false) break;
    uint8_t attr=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+11UL);
    if ((attr&ATTR_LONG_NAME)==ATTR_LONG_NAME)//��� ������� ���
    {
     //�������� ������ ���
     uint8_t name_index=GetByte(sFATRecordPointer_Local.CurrentFolderAddr);
     for(n=0;n<10 && long_name_length<=16;n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+1UL);
     for(n=0;n<12 && long_name_length<=16;n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+14UL);
	 for(n=0;n<4 && long_name_length<=16;n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+28UL);
	 if (long_name_length>16) break;
     if (name_index&0x40) break;//��������� ����� �����
    }
    else break;//��� �� ������� ���
   }
   if (long_name_length>16) long_name_length=16;
   if (long_name_length>0) filename[long_name_length]=0;
  }
  return(true);
 }
 return(false); 
}
//----------------------------------------------------------------------------------------------------
//����� � ���������� � ����� ������ ����
//----------------------------------------------------------------------------------------------------
bool FAT_EnterDirectory(uint32_t FirstCluster)
{ 
 if (FirstCluster==0UL)//��� �������� ���������� (����� ������� ��������, ��������������� ����������)
 {
  sFATRecordPointer.BeginFolderAddr=FirstRootFolderAddr; 
  sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//�������� ����� ��� ������ ������ ���������� (��� ��������)
 }
 else
 {
  uint32_t FirstSectorofCluster=((FirstCluster-2UL)*SecPerClus)+FirstDataSector; 
  sFATRecordPointer.BeginFolderAddr=FirstSectorofCluster*BytsPerSec;//��������� ����� ��� ������ ������ ����������
  sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+SecPerClus*BytsPerSec;
 }
 sFATRecordPointer.BeginFolderCluster=FirstCluster;//��������� ������� ����� ����� ������ ����������
 sFATRecordPointer.CurrentFolderCluster=sFATRecordPointer.BeginFolderCluster;//������� ������� ����� ����� ������ ����������
 sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;//������� ����� ��� ������ ������ ����������
 sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.BeginFolderAddr; 
 return(FAT_BeginFileSearch());
}
//----------------------------------------------------------------------------------------------------
//�������� � ��� ���� �����
//----------------------------------------------------------------------------------------------------
bool FAT_WriteBlock(uint16_t *BlockSize,uint16_t Block)
{
 uint32_t CurrentCluster;
 uint32_t Size;

 uint32_t i=0;//����� ������������ ����� �����
 uint16_t dram_addr=0;//����� � ������������ ������
 uint16_t current_block=0;//������� ����� �����
 uint16_t block_size=0;//������ �����
 int8_t Directory;//�� ���������� �� ����
 *BlockSize=0;
 if (FAT_GetFileSearch(String,&CurrentCluster,&Size,&Directory)==false) return(false); 
 uint8_t mode=0;
 while(i<Size)
 {
  DRAM_Refresh();//���������� ����������� ������ 
  //��������� ������
  uint32_t length=ClusterSize;
  if (length+i>=Size) length=Size-i;
  //�������� ������ ������ ��������
  uint32_t FirstSectorofCluster=((CurrentCluster-2UL)*SecPerClus)+FirstDataSector; 
  uint32_t addr=FirstSectorofCluster*BytsPerSec;
  for(uint32_t m=0;m<length;m++,i++)
  {
   DRAM_Refresh();//���������� ����������� ������
   uint8_t b=GetByte(addr+m);
   if (mode==0)//������ �������� ����� �����
   {
    block_size=b;
	mode=1;
	continue;
   }
   if (mode==1)//������ �������� ����� �����
   {
    block_size|=((uint16_t)b)<<8;
	mode=2;
	dram_addr=0;
	continue;
   }
   if (mode==2)//������ ������
   {
    if (current_block==Block) DRAM_WriteByte(dram_addr,b);//��� ��������� ����
	dram_addr++;
	if (dram_addr>=block_size)//���� ��������
	{
	 if (current_block==Block)//���������� ��������� ����
	 {
	  *BlockSize=block_size;  
	  return(true);
	 }
	 //��������� � ���������� �����
	 block_size=0;
	 current_block++;
	 mode=0;
	}
   } 
  }
  //��������� � ���������� �������� �����
  uint32_t FATClusterOffset=0;//�������� �� ������� FAT � ������ (� FAT32 ��� 4-� �������, � � FAT16 - �����������)
  if (FATType==FAT16) FATClusterOffset=CurrentCluster*2UL;
  uint32_t NextClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//����� ���������� ��������
  //��������� ����� ���������� �������� �����
  uint32_t NextCluster=0;
  if (FATType==FAT16) NextCluster=GetShort(NextClusterAddr);
  if (NextCluster==0) break;//�������������� �������
  if (NextCluster>=CountofClusters+2UL) break;//����� ������ ����������� ���������� ������ �������� - ����� ����� ��� ����
  CurrentCluster=NextCluster;
 }
 //����� �����
 return(false);
}