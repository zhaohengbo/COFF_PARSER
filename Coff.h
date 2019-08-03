/*Define TI DSP COFF (.out) file format structure*/

#ifndef   _COFF_H
#define   _COFF_H

#define HEADER_SIZE 	22
#define OPTIONAL_HEADER_SIZE 	28
#define SECTION_HEADER_SIZE 	48

//define of Header flag
#define	HEADER_FLAG_RELFLG 	0x001
//Relocation information was stripped from the file.
#define	HEADER_FLAG_EXEC 		0x002
//The file is relocatable (it contains no unresolved external	references).
#define	HEADER_FLAG_LSYMS 		0x008
//Local symbols were stripped from the file.
#define	HEADER_FLAG_LITTLE 		0x100
//The target is a little-endian device.
#define	HEADER_FLAG_BIG 		0x200
//The target is a big-endian device. 

//define of section type
#define	SECTION_TYPE_REG 	0x0000000
//Regular section (allocated, relocated, loaded)
#define	SECTION_TYPE_DSECT 	0x0000001
//Dummy section (relocated, not allocated, not loaded)
#define	SECTION_TYPE_NOLOAD 	0x0000002
//Noload section (allocated, relocated, not loaded)
#define	SECTION_TYPE_COPY 	0x0000010
//Copy section (relocated, loaded, but not allocated; relocation entries are processed normally)
#define	SECTION_TYPE_TEXT 	0x0000020
//Section contains executable code
#define	SECTION_TYPE_DATA 	0x0000040
//Section contains initialized data
#define	SECTION_TYPE_BSS 	0x0000080
//Section contains uninitialized data
#define	SECTION_TYPE_BLOCK 	0x0001000
//Alignment used as a blocking factor
#define	SECTION_TYPE_PASS 	0x0002000
//Section should pass through uncxhanged
#define	SECTION_TYPE_CLINK 	00004000
//Section requires conditional linking
#define	SECTION_TYPE_VECTOR 	0x0008000
//Section contains vector table
#define	SECTION_TYPE_PADDED 	0x0010000
//Section has been padded	

typedef struct _tCoffHeader
{
	unsigned short usVersionID; 	//indicates version of COFF file structure
	unsigned short usSectionNumber; 	//Number of section headers
	int iTimeStamp; 	//indicates when the file was created
	unsigned int uiSymbolPointer; 	//contains the symbol table's starting address
	unsigned int uiSymbolEntryNumber; 	//Number of entries in the symbol table
	unsigned short uiOptionalHeaderBytes;	//Number of bytes in the optional header. 
		//This field is either 0 or 28; if it is 0, there is no optional file header.
	unsigned short uiFlags;
/*		Mnemonic 	Flag 		Description
		F_RELFLG 	0001h 	Relocation information was stripped from the file.
		F_EXEC 		0002h 	The file is relocatable (it contains no unresolved external	references).
					0004h 	Reserved
		F_LSYMS 		0008h 	Local symbols were stripped from the file.
		F_LITTLE 		0100h 	The target is a little-endian device.
		F_BIG 		0200h 	The target is a big-endian device. */
	unsigned short uiTargetID; 	//magic number (0099h) indicates the file can be executed in a C6000 system
}TCoffHeader;

typedef struct _tCoffOptionalHeader
{
	unsigned short usOptionalHeaderID;	//Optional file header magic number (0108h) indicates the Optional header for C6000
	unsigned short usVersionStamp;
	unsigned int uiTextSize; 	//Integer Size (in bytes) of .text section
	unsigned  int uiDataSize; 	//Integer Size (in bytes) of .data section
	unsigned  int uiBssSize; 	//Integer Size (in bytes) of .bss section
	unsigned  int uiEntryPoint;
	unsigned int uiTextAddress; 	//Integer Beginning address of .text section
	unsigned  int uiDataAddress; 	//Integer Beginning address of .data section
}TCoffOptionalHeader;

typedef struct _tSectionHeader
{
	union
	{
		char sName[8];	//An 8-character section name padded with nulls
		unsigned int uiPointer[2];	//A pointer into the string table if the symbol name is longer than eight characters
	}SectionName;
	unsigned int uiPysicalAddress;	//Section's physical address (Run Address)
	unsigned int uiVirtalAddress; 	//Section's virtual address (Load Address)
	unsigned int uiSectionSize; 	//Section size in bytes
	unsigned int uiRawDataPointer; 	//File pointer to raw data
	unsigned int uiRelocationEntryPointer;	//File pointer to relocation entries
	unsigned int uiReserved0;
	unsigned int uiRelocationEntryNumber;	//Number of relocation entries
	unsigned int uiReserved1;
	unsigned int uiFlags;
/*		STYP_REG 	00000000h Regular section (allocated, relocated, loaded)
		STYP_DSECT 	00000001h Dummy section (relocated, not allocated, not loaded)
		STYP_NOLOAD 	00000002h Noload section (allocated, relocated, not loaded)
		STYP_COPY 	00000010h Copy section (relocated, loaded, but not allocated; relocation entries are processed normally)
		STYP_TEXT 	00000020h Section contains executable code
		STYP_DATA 	00000040h Section contains initialized data
		STYP_BSS 	00000080h Section contains uninitialized data
		STYP_BLOCK 	00001000h Alignment used as a blocking factor
		STYP_PASS 	00002000h Section should pass through unchanged
		STYP_CLINK 	00004000h Section requires conditional linking
		STYP_VECTOR 	00008000h Section contains vector table
		STYP_PADDED 	00010000h Section has been padded */
	unsigned short usReserved;
	unsigned short usMemoryPageNumber;
}TSectionHeader;

#endif

