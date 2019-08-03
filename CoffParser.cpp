/******************************************************************************
* This class is used to generate DSP boot table                               *
*******************************************************************************
Boot Table format as following:
|----------------------------------|
| Entry Point (4 bytes)            |
|----------------------------------|
| Section 1 Size (4 bytes)         |
| Section 1 Load Address (4 bytes) |
| Section 1 Run  Address (4 bytes) |
| Section 1 Data (4*n bytes)       |
|----------------------------------|
| Section 2 Size (4 bytes)         |
| Section 2 Load Address (4 bytes) |
| Section 2 Run  Address (4 bytes) |
| Section 2 Data (4*n bytes)       |
|----------------------------------|
| ..............                   |
|----------------------------------|
| Section N Size (4 bytes)         |
| Section N Load Address (4 bytes) |
| Section N Run  Address (4 bytes) |
| Section N Data (4*n bytes)       |
|----------------------------------|
| 0x00000000 (End flag)            |
|----------------------------------|
Notes: If the raw section data is not multiple of 4 bytes. pad will be add
       Don't copy the pad to DSP memory!
******************************************************************************/
#include "CoffParser.h"
#include "stdio.h"
#include "time.h"
//---------------------------------------------------------------------------
//Construction, create objects
TCoffParser::TCoffParser()
{
        SectionsList=new TStringList();
        IncludeSections=new TStringList();
        ExcludeSections=new TStringList();
	//Clear all value
        Initialize();
}
//---------------------------------------------------------------------------
//Destruction, release objects
TCoffParser::~TCoffParser()
{
	UnMapCoffFile();
    delete SectionsList;
    delete IncludeSections;
    delete ExcludeSections;
}
//---------------------------------------------------------------------------
//Initialization
void TCoffParser::Initialize()
{
	//Clear all value
    lpMemoryMapData=NULL;
    hFileMapHandle=NULL;
	cpCoffData=NULL;

	uiCodeSize=0;
	uiInitializedDataSize=0;
	uiUninitializedDataSize=0;
	uiCodeSectionNumber=0;
	uiInitializedDataSectionNumber=0;
	uiUninitializedDataSectionNumber=0;
    bGenerateSeperateCInitTable= false;  //Create Seperate CInitTable for boot time initialization
    bSwapRawData= false;      //Swap Raw Data for different endian mode
    bSwapInfo= false;         //Swap Infomation(Address and Size) for different endian mode

	memset(&tCoffHeader,0,HEADER_SIZE);
	memset(&tCoffOptionalHeader,0,OPTIONAL_HEADER_SIZE);
	memset(&tSectionHeader,0,SECTION_HEADER_SIZE);

    SectionsList->Clear();
    IncludeSections->Clear();
    ExcludeSections->Clear();
}
//---------------------------------------------------------------------------
//Map the .out file into memory image
int TCoffParser::CreateMemoryMap(AnsiString FileName)
{
    HANDLE hFile=INVALID_HANDLE_VALUE;
    hFileMapHandle=NULL;
    lpMemoryMapData=NULL;
    AnsiString MapFileName=ExtractFileName(FileName);
    MapFileName=MapFileName.SubString(1,MapFileName.Length()-4)+"_COFF_Parser_by_FHL";
    hFileMapHandle=OpenFileMapping(FILE_MAP_READ,true,MapFileName.c_str());
	if(hFileMapHandle==NULL)
    {
		hFile=CreateFile(FileName.c_str(),GENERIC_READ,FILE_SHARE_READ,
			NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if(hFile==INVALID_HANDLE_VALUE)	//Can't open file
        {
			return 1;
        }
                hFileMapHandle=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,MapFileName.c_str());
                CloseHandle(hFile);
                if(hFileMapHandle==NULL)  	//Can't create memory mapping file
                    return 2;
    }
	lpMemoryMapData=MapViewOfFile(hFileMapHandle,FILE_MAP_READ,0,0,0);
	cpCoffData=(unsigned char *)lpMemoryMapData;
        return 0;
}
//---------------------------------------------------------------------------
//unmap .out file
void TCoffParser::UnMapCoffFile()
{
	if(lpMemoryMapData!=NULL)
	{
        UnmapViewOfFile(lpMemoryMapData);
	    lpMemoryMapData=NULL;
	}
	if(hFileMapHandle!=NULL)
	{
		CloseHandle(hFileMapHandle);
        hFileMapHandle=NULL;
	}
}
//---------------------------------------------------------------------------
//Parse .out file, get statistics information
int TCoffParser::Parser(AnsiString CoffFileName)
{
        char sSectionName[80];
	UnMapCoffFile();
	Initialize();
	if(CreateMemoryMap(CoffFileName))
        return 1;       //Can't creat Memory map file

        memcpy(&tCoffHeader, cpCoffData, HEADER_SIZE);  //Get file header
        cpCoffData+= HEADER_SIZE;
        if(tCoffHeader.uiOptionalHeaderBytes)   //Have a optional header
        {
            //Get file optional header
            memcpy(&tCoffOptionalHeader, cpCoffData, tCoffHeader.uiOptionalHeaderBytes);
            cpCoffData+=OPTIONAL_HEADER_SIZE;
        }
		
	//Get String table address
        cpStringTableAddress=(unsigned char *)lpMemoryMapData+tCoffHeader.uiSymbolPointer+18*tCoffHeader.uiSymbolEntryNumber;

        for(int i=0; i<tCoffHeader.usSectionNumber; i++)
        {
            //Get section header
            memcpy(&tSectionHeader, cpCoffData, SECTION_HEADER_SIZE);
            cpCoffData+=SECTION_HEADER_SIZE;        //jamp to next section
            //Record Section Name
            if(tSectionHeader.SectionName.uiPointer[0])
            {       //Section name is not longer than 8
                strncpy(sSectionName, tSectionHeader.SectionName.sName, 8);
                sSectionName[8]=0;     //make sure string terminated
            }
            else
            {       //Section name is longer than 8, get section name form string table
                strncpy(sSectionName, cpStringTableAddress+tSectionHeader.SectionName.uiPointer[1], 79);
                sSectionName[79]=0;     //make sure string terminated
            }
            SectionsList->Add(sSectionName);

            if(tSectionHeader.uiSectionSize==0)
                continue;
            if(tSectionHeader.uiFlags&(SECTION_TYPE_DSECT|SECTION_TYPE_NOLOAD|SECTION_TYPE_COPY))
                continue;
		    //Code sections
                if(tSectionHeader.uiFlags&(SECTION_TYPE_TEXT|SECTION_TYPE_VECTOR))
                {
                        uiCodeSize+=tSectionHeader.uiSectionSize;
                        uiCodeSectionNumber++;
                }
                else if(tSectionHeader.uiFlags&SECTION_TYPE_DATA)
                { 	//Initialized Data section
                    uiInitializedDataSize+=tSectionHeader.uiSectionSize;
                    uiInitializedDataSectionNumber++;
                }
                else if(tSectionHeader.uiFlags&SECTION_TYPE_BSS)
                {	//Uninitialized Data section
                    uiUninitializedDataSize+=tSectionHeader.uiSectionSize;
                    uiUninitializedDataSectionNumber++;
                }
        }
        SectionsList->Sort();
	return 0;
}
//---------------------------------------------------------------------------
//Generate C Header file
int TCoffParser::GenerateCBootFile(AnsiString CFileName)
{
	return GenerateBootFile(CFileName, false);
}
//---------------------------------------------------------------------------
//Generate Binary file
int TCoffParser::GenerateBinBootFile(AnsiString BinFileName)
{
	return GenerateBootFile(BinFileName, true);
}
//---------------------------------------------------------------------------
//Generate boot image, and save it to a file, bBinFile determine it is binary formati or C header format
int TCoffParser::GenerateBootFile(AnsiString FileName, bool bBinFile)
{
	FILE *fpBootTable;
    AnsiString BootArrayName, CInitArrayName;
    char sLine[200], sSectionName[80];
    struct tm *CoffTime;
    bool bMustInclude;
	unsigned int uiTail=0;
    int i;

	//Output entry point and information
	if(bBinFile)
	{
		if((fpBootTable=fopen(FileName.c_str(),"wb"))==NULL)
                        return 1;
        //Output Entry point
        WriteInfoToFile(tCoffOptionalHeader.uiEntryPoint,fpBootTable);
	}
	else
	{
		if((fpBootTable=fopen(FileName.c_str(),"wt"))==NULL)
                        return 1;
        BootArrayName=ExtractFileName(FileName);
        BootArrayName=BootArrayName.SubString(1,BootArrayName.Length()-2);
        CInitArrayName=BootArrayName+"CInitTable";
        BootArrayName=BootArrayName+"BootTable";
        fputs(("#ifndef   "+BootArrayName+"_H\n").c_str(),fpBootTable);
        fputs(("#define   "+BootArrayName+"_H\n").c_str(),fpBootTable);
        fputs("\n",fpBootTable);
        fputs("/*******************************************************************************\n",fpBootTable);
        fputs("*                                   COFF Parser                               *\n",fpBootTable);
		fputs("*******************************************************************************\n",fpBootTable);
		fputs("Boot Table format as following:\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("| Entry Point (4 bytes)            |\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("| Section 1 Size (4 bytes)         |\n",fpBootTable);
		fputs("| Section 1 Load Address (4 bytes) |\n",fpBootTable);
		fputs("| Section 1 Run  Address (4 bytes) |\n",fpBootTable);
		fputs("| Section 1 Data (4*n bytes)       |\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("| Section 2 Size (4 bytes)         |\n",fpBootTable);
		fputs("| Section 2 Load Address (4 bytes) |\n",fpBootTable);
		fputs("| Section 2 Run  Address (4 bytes) |\n",fpBootTable);
		fputs("| Section 2 Data (4*n bytes)       |\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("| ..............                   |\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("| Section N Size (4 bytes)         |\n",fpBootTable);
		fputs("| Section N Load Address (4 bytes) |\n",fpBootTable);
		fputs("| Section N Run  Address (4 bytes) |\n",fpBootTable);
		fputs("| Section N Data (4*n bytes)       |\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("| 0x00000000 (End flag)            |\n",fpBootTable);
		fputs("|----------------------------------|\n",fpBootTable);
		fputs("Notes: If the raw section data is not multiple of 4 bytes, pad will be added,\n",fpBootTable);
        fputs("       Don't copy the pad to DSP memory!\n",fpBootTable);
        fputs("*******************************************************************************\n",fpBootTable);
        fputs(("COFF file format version                   "+IntToHex(tCoffHeader.usVersionID, 2)+"\n").c_str(),fpBootTable);

	    strcpy(sLine,"Code creation date                         ");
	    CoffTime= localtime(&((const long)tCoffHeader.iTimeStamp));
            strcat(sLine,asctime(CoffTime));
            fputs(sLine,fpBootTable);

	    if(tCoffHeader.uiTargetID==0x99)
	        fputs("Targeted architecture                      C6000(0x99)\n",fpBootTable);
	    else if(tCoffHeader.uiTargetID==0x98)
	        fputs("Targeted architecture                      C54x(0x98)\n",fpBootTable);
	    else if(tCoffHeader.uiTargetID==0x9c)
	        fputs("Targeted architecture                      C55x(0x9c)\n",fpBootTable);
	    else
	        fprintf(fpBootTable, "Targeted architecture                      Unknown(0x%x)\n",tCoffHeader.uiTargetID);

	    if((tCoffHeader.uiFlags)&HEADER_FLAG_LITTLE)
	        fputs("Endianess of object code                   Little Endian\n",fpBootTable);
	    else
	        fputs("Endianess of object code                   Big Endian\n",fpBootTable);

        fputs(("Number of initialized data sections        "+IntToStr(GetInitializedSectionNumber())+"\n").c_str(),fpBootTable);
        fputs(("Initialized section size (bytes)           "+IntToStr(GetInitializedSectionSize())+"\n").c_str(),fpBootTable);
        fputs("*******************************************************************************/\n",fpBootTable);
        fputs(("const char "+BootArrayName+"[]={\n").c_str(),fpBootTable);

        //Output Entry point
        fputs(" /*Program Entry point*/\n", fpBootTable);
        UintTo4Hex(tCoffOptionalHeader.uiEntryPoint, sLine);
		fputs(sLine,fpBootTable);
                fprintf(fpBootTable, "\n");
	}
        
	cpCoffData=(char *)lpMemoryMapData+HEADER_SIZE;
    if(tCoffHeader.uiOptionalHeaderBytes)   //Have a optional header
        cpCoffData+=OPTIONAL_HEADER_SIZE; 	//skip optional header

	//Output every section's data
    for(i=0; i<tCoffHeader.usSectionNumber; i++)
    {
        memcpy(&tSectionHeader, cpCoffData, SECTION_HEADER_SIZE);
        cpCoffData+=SECTION_HEADER_SIZE;
        if(tSectionHeader.uiSectionSize==0)
            continue;
        if(tSectionHeader.SectionName.uiPointer[0])
        { 	//Section name not longer than 8 bytes
            strncpy(sSectionName, tSectionHeader.SectionName.sName, 8);
            sSectionName[8]=0;     //make sure string terminated
        }
        else
        {	//Section name longer than 8 bytes, get it from string table
            strncpy(sSectionName, cpStringTableAddress+tSectionHeader.SectionName.uiPointer[1], 79);
            sSectionName[79]=0;     //make sure string terminated
        }
        if(bGenerateSeperateCInitTable&&strcmp(sSectionName,".cinit")==0)
        {
		    memcpy(&tCInitHeader, &tSectionHeader, SECTION_HEADER_SIZE);
            continue;
        }
        if(ExcludeSections->IndexOf(sSectionName)!=-1)
            continue;       //This section should not be included in the boot table
        if(IncludeSections->IndexOf(sSectionName)!=-1)
            bMustInclude=true;
        else
            bMustInclude=false;      //this section must be included in the boot table
        if((tSectionHeader.uiFlags&(SECTION_TYPE_DSECT|SECTION_TYPE_NOLOAD|SECTION_TYPE_COPY))&&!bMustInclude)
            continue;
        if((tSectionHeader.uiFlags&(SECTION_TYPE_TEXT|SECTION_TYPE_VECTOR|SECTION_TYPE_DATA))||bMustInclude)
        {
            if(!bBinFile)
                fprintf(fpBootTable, " /*Section %s begin*/\n", sSectionName);
		    OutputRawSectionData(fpBootTable, &tSectionHeader, bBinFile);
        }
    }

    //Output the end of the file
	if(bBinFile)
	{
		//Output four zero as ending flag
		fwrite(&uiTail,4,1,fpBootTable);
		if(bGenerateSeperateCInitTable)
		{ 	//Output .cinit section at the end of the file
			OutputRawSectionData(fpBootTable, &tCInitHeader, bBinFile);
		}
		fclose(fpBootTable);
	}
	else
	{
		//Output four zero as ending flag
		fputs(" /*ending flag*/\n", fpBootTable);
		fputs(" 0x00, 0x00, 0x00, 0x00\n", fpBootTable);
		fputs("};\n",fpBootTable);
		fputs("\n",fpBootTable);

		if(bGenerateSeperateCInitTable)
		{
			fputs(("const char "+CInitArrayName+"[]={\n").c_str(),fpBootTable);

			//Output Section name
			fputs(" /*Section .cinit begin*/\n", fpBootTable);
			OutputRawSectionData(fpBootTable, &tCInitHeader, bBinFile);
			fputs(" /*ending flag*/\n", fpBootTable);
			fputs(" 0x00, 0x00, 0x00, 0x00\n", fpBootTable);
			fputs("};\n",fpBootTable);
			fputs("\n",fpBootTable);
		}

		fputs("#endif\n",fpBootTable);
		fputs("\n",fpBootTable);
		fclose(fpBootTable);
	}

	return 0;
}
//---------------------------------------------------------------------------
//Output Raw Section data
void __fastcall TCoffParser::OutputRawSectionData(FILE *fpFile, TSectionHeader *SectionHeader, bool bBinFile)
{
	int j, k, n, iLines, iRoundSectionSize;

	unsigned int uiRawData, * ipRawSectionData;
	unsigned char * cpRawSectionData;
    char sLine[200];

	cpRawSectionData=(unsigned char *)lpMemoryMapData+SectionHeader->uiRawDataPointer;
	ipRawSectionData= (unsigned int *)cpRawSectionData;
	if(SectionHeader->uiSectionSize%4)
	    iRoundSectionSize=(SectionHeader->uiSectionSize/4+1)*4;
	else
	    iRoundSectionSize=SectionHeader->uiSectionSize;
	if(bBinFile)
	{
		//Output Section size
		WriteInfoToFile(SectionHeader->uiSectionSize,fpFile);
		//Output load address
		WriteInfoToFile(SectionHeader->uiVirtalAddress,fpFile);
		//Output run address
		WriteInfoToFile(SectionHeader->uiPysicalAddress,fpFile);
		//Output raw Section data
		for(j=0; j<iRoundSectionSize; j+=4)
		{
			uiRawData= *ipRawSectionData++;
			if(bSwapRawData) 	//Swap Raw data for different endian mode
				uiRawData=Swap(uiRawData);
			fwrite(&uiRawData,4,1,fpFile);
		}
	}
	else
	{
		//Output Section size
		UintTo4Hex(SectionHeader->uiSectionSize, sLine);
		strcat(sLine, " \t/*Size in bytes*/\n");
		fputs(sLine, fpFile);

		//Output load address
		UintTo4Hex(SectionHeader->uiVirtalAddress, sLine);
		strcat(sLine, " \t/*Load address*/\n");
		fputs(sLine, fpFile);

		//Output run address
		UintTo4Hex(SectionHeader->uiPysicalAddress, sLine);
		strcat(sLine, " \t/*Run address*/\n");
		fputs(sLine, fpFile);

		//Output raw Section data
		fputs(" /*Raw section Data*/\n", fpFile);
		cpRawSectionData= (unsigned char *)lpMemoryMapData+SectionHeader->uiRawDataPointer;
		iLines=iRoundSectionSize/16;
		n=0;
		for(j=0; j<iLines; j++)
		{
			for(k=0; k<16; k+=4)
			{
				if(bSwapRawData)
					fprintf(fpFile, " 0x%02x, 0x%02x, 0x%02x, 0x%02x,", 
							cpRawSectionData[3], cpRawSectionData[2], cpRawSectionData[1], cpRawSectionData[0]);
				else
					fprintf(fpFile, " 0x%02x, 0x%02x, 0x%02x, 0x%02x,", 
							cpRawSectionData[0], cpRawSectionData[1], cpRawSectionData[2], cpRawSectionData[3]);
				cpRawSectionData+=4;
				n+=4;
			}
			fprintf(fpFile,"\n");
		}
		if(n<iRoundSectionSize)
		{
			//Output the last line
			for(; n<iRoundSectionSize; n++)
			{
				if(bSwapRawData)
					fprintf(fpFile, " 0x%02x, 0x%02x, 0x%02x, 0x%02x,", 
							cpRawSectionData[3], cpRawSectionData[2], cpRawSectionData[1], cpRawSectionData[0]);
				else
					fprintf(fpFile, " 0x%02x, 0x%02x, 0x%02x, 0x%02x,", 
							cpRawSectionData[0], cpRawSectionData[1], cpRawSectionData[2], cpRawSectionData[3]);
				cpRawSectionData+=4;
				n+=4;
			}
			fprintf(fpFile,"\n");
		}
	}
}
//---------------------------------------------------------------------------
//Convert address or size into string including 4 Hex
char * TCoffParser::UintTo4Hex(unsigned int uiData, char *sLine)
{
        unsigned char *cData= (char *)&uiData;
        if(bSwapInfo) 	//Swap Information for different endian mode
            sprintf(sLine," 0x%02x, 0x%02x, 0x%02x, 0x%02x,", cData[3], cData[2], cData[1], cData[0]);
        else
            sprintf(sLine," 0x%02x, 0x%02x, 0x%02x, 0x%02x,", cData[0], cData[1], cData[2], cData[3]);
        return sLine;
}
//---------------------------------------------------------------------------
unsigned int TCoffParser::Swap(unsigned int uiData)
{
	unsigned int uiResult;
	unsigned char *ucpData= (unsigned char *)&uiData;
	unsigned char *ucpResult= (unsigned char *)&uiResult;
	ucpResult[0]= ucpData[3];
	ucpResult[1]= ucpData[2];
	ucpResult[2]= ucpData[1];
	ucpResult[3]= ucpData[0];
    return uiResult;
}
//---------------------------------------------------------------------------
//Save address or size information into File
void __fastcall TCoffParser::WriteInfoToFile(unsigned int uiData, FILE * fpFile)
{
	if(bSwapInfo) 	//Swap Information for different endian mode
		uiData= Swap(uiData);
	fwrite(&uiData,4,1,fpFile);
}
//---------------------------------------------------------------------------

