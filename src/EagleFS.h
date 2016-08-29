

// ©2016 Peter Orzell

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

    // http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef _EAGLE_FS_H_
#define _EAGLE_FS_H_

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#include "time.h"

#define NUM_FOLDERS                     16
#define NUM_FILE_HEADERS_PER_FOLDER     64

#define SIZE_OF_FOLDER_TABLE    sizeof(DISK_ADDRESS) * NUM_FILE_HEADERS_PER_FOLDER

#define MAX_FILE_HEADERS 16384

#define MAX_FILENAME 16

// Physical Block Size
#define BLOCK_SIZE 512

// Logical Sector Size
#define SECTOR_SIZE 2048
#define SECTOR_DATA_SIZE SECTOR_SIZE - sizeof(Sector_Header)

#define UNALLOCATED 0

//=========================================

#define ROOT                0

#define FOLDER_TABLES_START (DISK_ADDRESS)(ROOT /*+ SIZE_OF_FOLDER_TABLE*/)
#define FOLDER_TABLES_END	(DISK_ADDRESS)(FOLDER_TABLES_START + (SIZE_OF_FOLDER_TABLE * NUM_FOLDERS))
#define FILE_HEADERS_START  (DISK_ADDRESS)(FOLDER_TABLES_END)
#define FILE_HEADERS_END	(DISK_ADDRESS)(FILE_HEADERS_START + (MAX_FILE_HEADERS * sizeof(File_Header)))

// 512 KB worth of 512-byte sectors
#define SECTOR_KB 512
#define NUM_SECTORS	(uint64_t)((SECTOR_KB*1024)/SECTOR_SIZE)
#define SECTORS_START       (DISK_ADDRESS)FILE_HEADERS_END//(DISK_ADDRESS)(SECTOR_TABLE_END)//SECTOR_TABLE_END
#define SECTORS_END			(DISK_ADDRESS)(SECTORS_START + (NUM_SECTORS*SECTOR_SIZE))

#define LAST_SECTOR_OF_FILE_MARKER			1

#define SECTOR_FIRST_DATA_BYTE(sector) (DISK_ADDRESS)(sector + sizeof(Sector_Header))

enum DISK_ERROR_CODE
{
    GENERAL_ERROR = 0,
    ERROR_NO_FILE_ENTRIES_AVAILABLE_IN_FOLDER,
    ERROR_NO_FILE_HEADERS_AVAILABLE
};

typedef uint64_t DISK_ADDRESS;
typedef uint64_t SECTOR_TABLE_INDEX;

typedef enum Header_Type
{
    TYPE_UNALLOCATED = 0,
	TYPE_FILE,
    TYPE_FILE_CONTINUED,
    TYPE_FOLDER
} Header_Type;

typedef struct
{
	DISK_ADDRESS Next_Sector;
	uint64_t Bytes_of_Sector_Filled;
} Sector_Header;

// 64-byte file header.

typedef struct
{
	// Is a root node.
	Header_Type File_Type : 8;

	// File Name
	char File_Name[MAX_FILENAME];

    // Date
	time_t File_Modified_Date;

	uint32_t _rsvd;

	// Used for Parent if File_Type == TYPE_FILE || File_Type == TYPE_FOLDER
	DISK_ADDRESS File_Parent;

	// Data Location.
	// (Points back to the Folder's file table if it's a folder.)
	DISK_ADDRESS File_Data_Location;

} File_Header;

//=======================
// DISK I/O ROUTINES
//=======================

bool disk_init();
int disk_read(DISK_ADDRESS address, void *buffer, uint64_t size);
int disk_write(DISK_ADDRESS address, void *buffer, uint64_t size);

//=======================

DISK_ADDRESS EagleFS_Quick_Format();

DISK_ADDRESS EagleFS_Create_File_Entry(char *filename, Header_Type file_type, DISK_ADDRESS parent_address);

DISK_ADDRESS EagleFS_Find_File(char *filename, DISK_ADDRESS parent, Header_Type filetype);

DISK_ADDRESS EagleFS_Find_File_From_Path(char *pathname, DISK_ADDRESS parent);

uint64_t EagleFS_Write_To_File(DISK_ADDRESS file, uint8_t *data, uint64_t size);

uint64_t EagleFS_Read_From_File(DISK_ADDRESS file_header, uint8_t *buffer, uint64_t size);

bool EagleFS_Remove_File(DISK_ADDRESS file_header);

uint64_t EagleFS_Get_File_Size(DISK_ADDRESS file_header);

void EagleFS_List_Directory(DISK_ADDRESS directory_header);

uint64_t EagleFS_Get_Folder_Contents(DISK_ADDRESS folder, DISK_ADDRESS *entries);

DISK_ADDRESS EagleFS_Find_File_In_Dir(char *filename, DISK_ADDRESS directory_header);

const char* EagleFS_Get_File_Name(DISK_ADDRESS file_header);

bool EagleFS_isFolder(DISK_ADDRESS file_header);
bool EagleFS_isFile(DISK_ADDRESS file_header);
bool EagleFS_isDeleted(DISK_ADDRESS file_header);

#endif //_EAGLE_FS_H_
