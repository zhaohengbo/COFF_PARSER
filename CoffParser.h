#ifndef   _COFFPARSER_H
#define   _COFFPARSER_H
#include "stdio.h"
#include "vcl.h"
#include "Coff.h"

class TCoffParser
{
public:
	TCoffParser();
	~TCoffParser();
	
	//Parser COFF file, get information, prepare for generating boot miage
	int Parser(AnsiString CoffFileName); 	
	//generate binary boot image;
	int GenerateBinBootFile(AnsiString BinFileName); 
	//generate C header file includeing boot image
	int GenerateCBootFile(AnsiString CFileName);
	//API for providing the information of the program
	//Total memery size should be consumed by the program
	unsigned int GetTotalSize() {return uiCodeSize+uiInitializedDataSize+uiUninitializedDataSize;}
	//Initialized sections, including codes and initialized data, which should be copy during boot
	unsigned int GetInitializedSectionSize() {return uiCodeSize+uiInitializedDataSize;}
	unsigned int GetCodeSize() {return uiCodeSize;}
	unsigned int GetInitializedDataSize() {return uiInitializedDataSize;}
	unsigned int GetUninitializedDataSize() {return uiUninitializedDataSize;}
	unsigned int GetInitializedSectionNumber() {return uiCodeSectionNumber+uiInitializedDataSectionNumber;}
	unsigned int GetCodeSectionNumber() {return uiCodeSectionNumber;}
	unsigned int GetInitializedDataSectionNumber() {return uiInitializedDataSectionNumber;}
	unsigned int GetUninitializedDataSectionNumber() {return uiUninitializedDataSectionNumber;}

	TCoffHeader tCoffHeader;
	TCoffOptionalHeader tCoffOptionalHeader;
    TStringList* IncludeSections;        //The sections must include in the boot table
    TStringList* ExcludeSections;        //The section must NOT include in the boot table
    TStringList* SectionsList;
    bool bGenerateSeperateCInitTable;  //Create Seperate CInitTable for boot time initialization
    bool bSwapRawData;      //Swap Raw Data for different endian mode
    bool bSwapInfo;         //Swap Infomation(Address and Size) for different endian mode
private:
    void Initialize();
	int CreateMemoryMap(AnsiString FileName);
    void UnMapCoffFile();
	int GenerateBootFile(AnsiString FileName, bool bBinFile);
    char *UintTo4Hex(unsigned int uiData, char *sLine);
	unsigned int Swap(unsigned int uiData);
	void __fastcall WriteInfoToFile(unsigned int uiData, FILE * fpFile);
    void __fastcall OutputRawSectionData(FILE *fpFile, TSectionHeader *SectionHeader, bool bBinFile);

	unsigned char * cpCoffData;
    LPVOID lpMemoryMapData;
    HANDLE hFileMapHandle;
    unsigned char * cpStringTableAddress;

	TSectionHeader tSectionHeader;
	TSectionHeader tCInitHeader;
	unsigned int uiCodeSize; 	//Total codes size in bytes
	unsigned int uiInitializedDataSize; 	//Total Initialized Data Size
	unsigned int uiUninitializedDataSize; 	//Totol uninitialized data size
	unsigned int uiCodeSectionNumber; 	
	unsigned int uiInitializedDataSectionNumber;
	unsigned int uiUninitializedDataSectionNumber;
};



#endif
