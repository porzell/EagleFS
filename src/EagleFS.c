#include "EagleFS.h"
#include "string.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define SECTOR_TABLE_CHUNK_SIZE	2048

char *path_tokens = "\\/";

//======================================================================
DISK_ADDRESS EagleFS_Find_Free_Folder_Table();

DISK_ADDRESS EagleFS_Find_Free_File_Header();

DISK_ADDRESS EagleFS_Find_Free_Sector(SECTOR_TABLE_INDEX *index);

//======================================================================

bool disk_init()
{
    
    // *** Driver code goes here ***
    
	return true;
}


///@brief Storage driver interface - read an arbitrary number of bytes from a given disk address.
///@param[in] address A disk address to begin reading at.
///@param[in] buffer A byte buffer to read to.
///@param[in] size A number of bytes to read.
int disk_read(DISK_ADDRESS address, void *buffer, uint64_t size)
{
    
    // *** Driver code goes here ***
    
	return size;
}

///@brief Storage driver interface - write an arbitrary number of bytes from a given disk address.
///@param[in] address A disk address to begin writing at.
///@param[in] buffer A byte buffer to write to disk.
///@param[in] size A number of bytes from the buffer to write.
int disk_write(DISK_ADDRESS address, void *buffer, uint64_t size)
{
    // *** Driver code goes here ***
    
	return size;
}

///@brief A strtok_r clone used by EagleFS for directory parsing.
char* tokenize(char *str, char *tok, char **save)
{
	char *orig = NULL;


	/*if (strlen(*save) == 0)
	return NULL;*/

	if (!**save)
		return NULL;

	size_t num_tokens = strlen(tok);

	if (*save == str)
	{
		if (!*str)
			return NULL;

		orig = str;

		while (*str != '\0')
		{
			size_t i = 0;
			for (; i < num_tokens; ++i)
			{
				if (*str == tok[i])
				{
					*str = '\0';
					*save = str + 1;
					return orig;
				}
			}
			str++;
		}
		return orig;
	}
	else
	{
		orig = *save;

		char *ptr = *save;

		while (true)
		{
			size_t i = 0;
			for (; i < num_tokens; ++i)
			{
				if (*ptr == NULL)
				{
					*save = ptr;
					return orig;
				}
				else if (*ptr == tok[i])
				{
					*ptr = '\0';
					*save = ptr + 1;
					return orig;
				}
			}
			ptr++;
		}
	}
}

///@brief Sets the timestamp internally in a file header structure.
///@param[in] header A pointer to a pre-allocated File_Header structure.
///@param[in] the_time A time_t to be written to the header structure in memory.
void EagleFS_Set_Timestamp(File_Header *header, time_t the_time)
{
	//SetTime
	//memcpy(header->File_Modified_Date, ctime(&the_time), 24);
	header->File_Modified_Date = the_time;
}

///@brief Sets the timestamp internally in a file header structure.
///@param[in] header A pointer to a pre-allocated File_Header structure.
///@param[in] the_time A time_t to be written to the header structure in memory.
DISK_ADDRESS EagleFS_Quick_Format()
{
	DISK_ADDRESS unallocated = TYPE_UNALLOCATED;

	// Here's our seek pointer.
	DISK_ADDRESS seek_pointer = ROOT;

	// =========================
	// Setup Folder Tables
	// =========================

	DISK_ADDRESS zeros[NUM_FILE_HEADERS_PER_FOLDER];
	memset(zeros, 0, sizeof(zeros));

	printf("Formatting folder tables...\n");

	// First, clear the root folder table.

	// Now do it for each folder entry.
	size_t i = 0;

	for (; i < NUM_FOLDERS; ++i, seek_pointer += SIZE_OF_FOLDER_TABLE)
	{
		printf("Formatting folder table %u/%u\n", i, NUM_FOLDERS);
		disk_write(seek_pointer, zeros, SIZE_OF_FOLDER_TABLE);
	}

	printf("Done!\n");

	// Nice!

	printf("Formatting file header table...\n");

	// =========================
	// Setup File Header Table.
	// =========================

	// Clear each File Header entry.
	seek_pointer = FILE_HEADERS_START;

	for (i = 0; i < MAX_FILE_HEADERS; ++i)
	{
		printf("Formatting file header %u/%u\n", i, MAX_FILE_HEADERS);

		File_Header zero_header = {0};

		// Zero the File_Type for each file block.
		disk_write(seek_pointer, &zero_header, sizeof(zero_header));

		seek_pointer += sizeof(File_Header);
	}

	printf("Done!\n");

	// ========================
	// Write Root File Header.
	// ========================

	printf("Formatting Root file header...\n");

	File_Header root = { 0 };

	root.File_Type = TYPE_FOLDER;
	root.File_Data_Location = FOLDER_TABLES_START;
	memcpy(root.File_Name, "Root", strlen("Root"));

	time_t tim;
	time(&tim);

	EagleFS_Set_Timestamp(&root, tim);

	disk_write((DISK_ADDRESS)FILE_HEADERS_START, &root, sizeof(root));

	printf("Done!\n");

	// =========================
	// Set Up Root Folder Table
	// =========================

	printf("Clearing root folder table...\n");

	DISK_ADDRESS folders = FILE_HEADERS_START;

	disk_write((DISK_ADDRESS)FOLDER_TABLES_START, &folders, sizeof(folders));

	printf("Done!\n");

	// ===========================
	// Zero Sector Headers
	// ===========================

	seek_pointer = SECTORS_START;

	DISK_ADDRESS zero_address = 0;

	printf("First sector: %llu\nEnd of last sector: %llu\n", SECTORS_START, SECTORS_END);

	while (seek_pointer < SECTORS_END)
	{
		printf("Formatting %f%% done!\n", (float)((seek_pointer - SECTORS_START) / NUM_SECTORS)/SECTOR_SIZE * 100);//(float)(((uint64_t)seek_pointer-SECTORS_START)/512)/NUM_SECTORS);
		disk_write(seek_pointer, &zero_address, sizeof(zero_address));
		seek_pointer += SECTOR_SIZE;
	}

	printf("Done!\n");

	//Done!
}

///@brief Finds a free file header block.
DISK_ADDRESS EagleFS_Find_Free_File_Header()
{
	// Start with first file header after root.

	DISK_ADDRESS seek_pointer = FILE_HEADERS_START + sizeof(File_Header);

	File_Header header;

	while (seek_pointer < FILE_HEADERS_END)
	{
		//disk_read(seek_pointer, &header, sizeof(header));

		// We only care about the first byte anyway.
		disk_read(seek_pointer, &header, sizeof(header));

		// Check if File Type is unallocated.
		if (header.File_Type == TYPE_UNALLOCATED)
		{
			return seek_pointer;
		}

		seek_pointer += sizeof(File_Header);
	}

	return NULL;
}

///@brief Finds a free folder table entry.
DISK_ADDRESS EagleFS_Find_Free_Folder_Table()
{
	// Start with first folder table after root.

	DISK_ADDRESS seek_pointer = FOLDER_TABLES_START + SIZE_OF_FOLDER_TABLE;

	DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];

	while (seek_pointer < FOLDER_TABLES_END)
	{
		// We only care about the first byte anyway.
		disk_read(seek_pointer, &ftable, sizeof(ftable));

		// Check if Folder Table has owner.
		if (ftable[0] == UNALLOCATED)
		{
			return seek_pointer;
		}

		seek_pointer += SIZE_OF_FOLDER_TABLE;
	}

	return NULL;
}

///@brief Finds a free data sector.
DISK_ADDRESS EagleFS_Find_Free_Sector(SECTOR_TABLE_INDEX *index)
{
	
	DISK_ADDRESS seek_pointer = SECTORS_START;

	Sector_Header sector_header;

	disk_read(seek_pointer, &sector_header, sizeof(sector_header));

	// Start sector index at 0.
	(*index) = 0;

	while (sector_header.Next_Sector)
	{
		seek_pointer += SECTOR_SIZE;

		if (seek_pointer > SECTORS_END)
			return NULL;

		disk_read(seek_pointer, &sector_header, sizeof(sector_header));

		// Increment sector index.
		(*index)++;
	}

	return seek_pointer;
}

///@brief Creates a new File Entry.
///@param[in] filename The name of the file to be created.
///@param[in] file_type The type of file to be created.  This should be TYPE_FILE or TYPE_FOLDER.
///@param[in] parent_address A DISK_ADDRESS containing the file header location of a parent folder.  If set to NULL, the parent automatically becomes ROOT.
///@return Returns a DISK_ADDRESS of the newly created file header, or NULL if unsuccessful.
DISK_ADDRESS EagleFS_Create_File_Entry(char *filename, Header_Type file_type, DISK_ADDRESS parent_address)
{
	// If parent is NULL, choose ROOT.
	if (!parent_address)
		parent_address = FILE_HEADERS_START;

	// Create parent file header.
	File_Header parent;

	// Load parent.
	disk_read(parent_address, &parent, sizeof(parent));

	// Check if parent is valid folder.
	if (parent.File_Type != TYPE_FOLDER)
		return NULL;

	// We're good! Now find the parent's ftable.
	DISK_ADDRESS ftable_address = parent.File_Data_Location;

	// Cache the parent folder's folder table.
	DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];

	// Cache it up!
	disk_read(ftable_address, ftable, sizeof(ftable));


	// Now look through the cached table for a free entry.
	DISK_ADDRESS free_entry = NULL;

	// Don't read the first one. (It should be the disk address of the folder file entry.)
	size_t i = 1;
	for (; i < NUM_FILE_HEADERS_PER_FOLDER; ++i)
	{
		// Is this entry taken?
		if (ftable[i] == UNALLOCATED)
		{
			// Grab it, quickly! >:D
			free_entry = ftable_address + (i * sizeof(DISK_ADDRESS));
			// Make a break for it!
			break;
		}
	}

	// If we couldn't find one, we can't continue.
	if (!free_entry)
		return NULL;

	//-----------------------------------------------------------------

	// Now let's try to get a file header for our new file.
	DISK_ADDRESS header_address = EagleFS_Find_Free_File_Header();

	// If we can't get one, we can't continue.
	if (!header_address)
		return NULL;

	// If we did, we need to set the entry in the previous folder to our new file.
	//disk_write(free_entry, header_address, sizeof(DISK_ADDRESS));

	// Allocate header object.
	File_Header header;

	//static char date[] = "1997-07-16T19:20:30";

	switch (file_type)
	{
		// Treat anything that isn't a folder as a file.
	case TYPE_FOLDER:
	{

		//===============
		// Set file type
		//===============
		header.File_Type = TYPE_FOLDER;

		// Since we are a folder, we need to get a folder table.
		DISK_ADDRESS folder_table = EagleFS_Find_Free_Folder_Table();

		if (!folder_table)
			return NULL;

		header.File_Data_Location = folder_table;

		disk_write(folder_table, &header_address, sizeof(header_address));

		break;

	}

	case TYPE_FILE:
	{
		//===============
		// Set file type
		//===============
		header.File_Type = TYPE_FILE;

		// Create variable to hold a sector table entry if we find a free one.
		SECTOR_TABLE_INDEX free_sector_table_entry = NULL;

		//=======================
		// Set file start sector
		//=======================
		header.File_Data_Location = EagleFS_Find_Free_Sector(&free_sector_table_entry);

		// If we cannot allocate a sector, then don't continue.
		if (!(header.File_Data_Location))
			return NULL;

		Sector_Header sector_header;

		sector_header.Next_Sector = LAST_SECTOR_OF_FILE_MARKER;
		sector_header.Bytes_of_Sector_Filled = 0;
		disk_write(header.File_Data_Location, &sector_header, sizeof(sector_header));

		break;
	}

	default:
		break;
	}

	//============
	// Set parent
	//============

	//header.File_Prev_Data_Location = parent_address;
	//header.File_Next_Data_Location = NULL;

	header.File_Parent = parent_address;

	//===============
	// Set file name
	//===============
	size_t name_length = strlen(filename);

	if (name_length >= MAX_FILENAME)
		name_length = MAX_FILENAME;

	// Pad with nulls.
	memset(header.File_Name, '\0', sizeof(header.File_Name));
	// Copy name.
	memcpy(header.File_Name, filename, name_length);

	//===============
	// Set file date
	//===============

	// Copy date.
	//memcpy(header.File_Modified_Date, date, 20);
	time_t tim;
	time(&tim);

	EagleFS_Set_Timestamp(&header, tim);

	disk_write(header_address, &header, sizeof(header));

	printf("EagleFS: Created new file \"%s\" (File table address: %llu, First sector: %llu\n", filename, header_address, header.File_Data_Location);

	// Write ourselves into the directory table.
	disk_write(free_entry, &header_address, sizeof(header_address));

	// Now return the location of the new file header to the caller.
	return header_address;
}

///@brief Searches for a file given a filename and a parent directory.
///@param[in] filename The file name to search for.
///@param[in] parent The DISK_ADDRESS for a folder file header to search in.
///@param[in] filetype The filetype to search for.
///@return Returns a DISK_ADDRESS to the file header for a found file, or NULL if unsuccessful.
DISK_ADDRESS EagleFS_Find_File(char *filename, DISK_ADDRESS parent, Header_Type filetype)
{
	File_Header parent_header;

	disk_read(parent, &parent_header, sizeof(parent_header));

	if (parent_header.File_Type != TYPE_FOLDER)
	{
		printf("EagleFS: Cannot find file, %s, in a non-folder!\n", filename);
		return NULL;
	}

	// Now get the folder table for the parent.
	DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];

	disk_read(parent_header.File_Data_Location, &ftable, sizeof(ftable));

	// Check if ftable matches parent.
	if (ftable[0] != parent)
	{
		printf("EagleFS: Warning! Folder table at 0x%llx does not match expected address, %llx.\n", parent_header.File_Data_Location, parent);
	}

	// Skip the first entry.
	size_t i = 1;
	for (; i < NUM_FILE_HEADERS_PER_FOLDER; ++i)
	{
		File_Header header;

		disk_read(ftable[i], &header, sizeof(header));

		// Ensure we are dealing with null-terminated strings here, even if the max filename size is used.
		char the_filename[MAX_FILENAME + 1];
		memcpy(the_filename, header.File_Name, MAX_FILENAME);
		the_filename[MAX_FILENAME] = '\0';

		if (!strcmp(the_filename, filename))
		{
			return ftable[i];
		}
	}

	// If we didn't find anything, return null.
	return NULL;
}

///@brief Appends to the end of an existing file.
///@param[in] file A DISK_ADDRESS of a file header.
///@param[in] data A byte buffer to be appended to the file.
///@param[in] size The number of bytes to be written to disk.
///@return Returns the number of bytes successfully written.
uint64_t EagleFS_Write_To_File(DISK_ADDRESS file, uint8_t *data, uint64_t size)
{

	File_Header orig_header;
	disk_read(file, &orig_header, sizeof(orig_header));

	time_t now;
	time(&now);

	EagleFS_Set_Timestamp(&orig_header, now);

	disk_write(file, &orig_header, sizeof(orig_header));

	Sector_Header last_sector_header;

	DISK_ADDRESS last_sector = orig_header.File_Data_Location;

	uint64_t orig_size = size;

	// Read in the sector header.
	if (!disk_read(last_sector, &last_sector_header, sizeof(last_sector_header)))
		return orig_size - size;

	// Find last sector of file.
	while (last_sector_header.Next_Sector != LAST_SECTOR_OF_FILE_MARKER)
	{
		last_sector = last_sector_header.Next_Sector;
		disk_read(last_sector, &last_sector_header, sizeof(last_sector_header));
	}

	if (last_sector_header.Bytes_of_Sector_Filled < SECTOR_DATA_SIZE)
	{
		uint64_t diff = (DISK_ADDRESS)SECTOR_DATA_SIZE - last_sector_header.Bytes_of_Sector_Filled;

		if (size < diff)
			diff = size;

		// Write the new data.
		disk_write(SECTOR_FIRST_DATA_BYTE(last_sector) + last_sector_header.Bytes_of_Sector_Filled, data, diff);

		// Add our written bytes into the header entry.
		last_sector_header.Bytes_of_Sector_Filled += diff;

		// Write new size.
		DISK_ADDRESS newSize = last_sector + offsetof(Sector_Header, Bytes_of_Sector_Filled);

		disk_write(newSize, &(last_sector_header.Bytes_of_Sector_Filled), sizeof(last_sector_header.Bytes_of_Sector_Filled));

		// Remember how much we wrote here.
		data += diff;
		size -= diff;
	}

	// At this point, if we haven't finished our write, we gotta find new sectors.
	while (size > 0)
	{
		SECTOR_TABLE_INDEX index = NULL;


		DISK_ADDRESS new_sector = EagleFS_Find_Free_Sector(&index);
		Sector_Header new_sector_header;

		// If we failed, then we're screwed, sorry!
		if (!new_sector)
			return orig_size - size;

		// Set link on last sector header.
		last_sector_header.Next_Sector = new_sector;

		// Write link.
		disk_write(last_sector, &last_sector_header, sizeof(last_sector_header));

		//disk_write(last_sector + offsetof(Sector_Header, Next_Sector), &new_sector, sizeof(DISK_ADDRESS));

		// Set header as used.
		new_sector_header.Next_Sector = LAST_SECTOR_OF_FILE_MARKER;
		new_sector_header.Bytes_of_Sector_Filled = 0;

		// Now write the data.

		uint64_t diff = (DISK_ADDRESS)SECTOR_DATA_SIZE;

		if (size < diff)
			diff = size;

		// Write the new data.
		disk_write(SECTOR_FIRST_DATA_BYTE(new_sector) + new_sector_header.Bytes_of_Sector_Filled, data, diff);

		// Set the bytes of the sector filled.
		new_sector_header.Bytes_of_Sector_Filled += diff;

		// Write the header.
		disk_write(new_sector, &new_sector_header, sizeof(new_sector_header));

		// Prepare for loop if we have to.
		last_sector = new_sector;
		last_sector_header = new_sector_header;

		// Remember how much we wrote here.
		data += diff;
		size -= diff;

		//printf("EagleFS: Created %llu-byte extension to file %llu at sector address %llu.\n", diff, file, new_sector);
	}

	return orig_size - size;
}

///@brief Reads in an existing file.
///@param[in] file_header A DISK_ADDRESS of a file header.
///@param[in] buffer A byte buffer to be appended to the file.
///@param[in] size The number of bytes to be written to disk.
///@return Returns the number of bytes successfully read from the specified file.
uint64_t EagleFS_Read_From_File(DISK_ADDRESS file_header, uint8_t *buffer, uint64_t size)
{
	File_Header header;
	disk_read(file_header, &header, sizeof(header));

	DISK_ADDRESS sector = header.File_Data_Location;
	Sector_Header sector_header;

	if (!disk_read(sector, &sector_header, sizeof(sector_header)))
		return 0;

	uint32_t amount = 0;

	while (size > 0)
	{
		uint32_t block_size = sector_header.Bytes_of_Sector_Filled;

		if (size < block_size)
			block_size = size;

		DISK_ADDRESS address = SECTOR_FIRST_DATA_BYTE(sector);

		disk_read(address, buffer, block_size);

		size -= block_size;
		buffer += block_size;
		amount += block_size;

		if (sector_header.Next_Sector == LAST_SECTOR_OF_FILE_MARKER)
			return amount;

		sector = sector_header.Next_Sector;

		disk_read(sector, &sector_header, sizeof(sector_header));
	}

	return amount;
}

///@brief Removes a file from the disk. (Non-destructive)
///@param[in] file_header The DISK_ADDRESS pointing to the file header of a file to be deleted.
///@retval true The file was successfully deleted.
///@retval false The file could not be deleted.
bool EagleFS_Remove_File(DISK_ADDRESS file_header)
{
	File_Header header;

	if (!disk_read(file_header, &header, sizeof(header)))
		return false;

	char filename[MAX_FILENAME + 1];
	memcpy(filename, header.File_Name, MAX_FILENAME);
	filename[MAX_FILENAME] = 0;

	printf("Deleting %s...\n", filename);

	switch (header.File_Type)
	{
	case TYPE_FOLDER:
	{
		DISK_ADDRESS folder_table = header.File_Data_Location;

		DISK_ADDRESS zero = 0;

		// Invalidate our folder table by removing the pointer to ourselves.
		disk_write(folder_table, &zero, sizeof(zero));

		// Buffer up the entries in the folder table.
		DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];
		disk_read(folder_table, ftable, sizeof(ftable));

		// We don't care about that first entry we just zeroed.
		DISK_ADDRESS *entry = ftable;

		entry++;

		DISK_ADDRESS a_file = NULL;

		size_t i = 1;

		// Look through and call remove on anything inside that isn't NULL.

		while (entry < ftable + NUM_FILE_HEADERS_PER_FOLDER)
		{
			//printf("Checking folder table entry %u at %llu... It's %llu...\n", i, folder_table + (i * sizeof(DISK_ADDRESS)), *entry);

			if (a_file = *entry)
			{
				// TODO: Note: RECURSION!  (should probably use a stack to do this other than the program stack)
				EagleFS_Remove_File(a_file);
			}
			entry++;
			i++;
		}

		break;
	}

	case TYPE_FILE:
	{
		DISK_ADDRESS sector = header.File_Data_Location;

		Sector_Header sector_header;

		if (!disk_read(sector, &sector_header, sizeof(sector_header)))
			return false;

		DISK_ADDRESS next_sector = sector_header.Next_Sector;

		sector_header.Next_Sector = 0;

		if (!disk_write(sector, &sector_header, sizeof(sector_header)))
			return false;

		while (next_sector != LAST_SECTOR_OF_FILE_MARKER)
		{
			sector = next_sector;

			if (!disk_read(sector, &sector_header, sizeof(sector_header)))
				return false;

			next_sector = sector_header.Next_Sector;

			sector_header.Next_Sector = 0;

			if (!disk_write(sector, &sector_header, sizeof(sector_header)))
				return false;
		}

		break;
	}
	default:
		break;
	}

	header.File_Type = TYPE_UNALLOCATED;

	// Mark the header as unallocated.

	if (!disk_write(file_header, &header, sizeof(header)))
		return false;

	// Now remove our entry in our parent's ftable.

	// First, get parent header.
	DISK_ADDRESS parent = header.File_Parent;

	File_Header parent_header;

	// Load the parent header.
	if (!disk_read(parent, &parent_header, sizeof(parent_header)))
		return false;

	// Now make an ftable buffer.

	DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];

	// Cache the ftable in RAM.
	if (!disk_read(parent_header.File_Data_Location, ftable, sizeof(ftable)))
		return false;

	// Find our entry and zero it.
	DISK_ADDRESS *entry = ftable + 1;

	while (entry < ftable + NUM_FILE_HEADERS_PER_FOLDER)
	{
		if (*entry == file_header)
		{
			DISK_ADDRESS zero = 0;

			DISK_ADDRESS entry_location = parent_header.File_Data_Location + ((DISK_ADDRESS)entry - (DISK_ADDRESS)ftable);

			// Zero the entry!
			disk_write(entry_location, &zero, sizeof(zero));
			break;
		}

		entry++;
	}

	return true;
}

///@brief Returns the size in bytes of a given file.
uint64_t EagleFS_Get_File_Size(DISK_ADDRESS file_header)
{
	uint64_t length = 0;

	File_Header header;
	disk_read(file_header, &header, sizeof(header));

	DISK_ADDRESS sector = header.File_Data_Location;

	Sector_Header sector_header;
	disk_read(sector, &sector_header, sizeof(sector_header));

	length += sector_header.Bytes_of_Sector_Filled;

	while (sector_header.Next_Sector != LAST_SECTOR_OF_FILE_MARKER)
	{
		sector = sector_header.Next_Sector;
		disk_read(sector, &sector_header, sizeof(sector_header));

		length += sector_header.Bytes_of_Sector_Filled;
	}

	return length;
}

///@brief Acts like DIR/ls.
void EagleFS_List_Directory(DISK_ADDRESS directory_header)
{
	DISK_ADDRESS seek_pointer = directory_header;

	if (!seek_pointer)
		seek_pointer = FILE_HEADERS_START;

	File_Header header;

	disk_read(seek_pointer, &header, sizeof(header));

	char name[MAX_FILENAME + 1];

	memcpy(name, header.File_Name, MAX_FILENAME);

	name[MAX_FILENAME] = 0;

	printf("-------------------------\nListing directory %s\n---------------------------\n", name);

	DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];

	disk_read(header.File_Data_Location, &ftable, sizeof(ftable));

	DISK_ADDRESS *ptr = ftable + 1;

	uint64_t size_in_bytes;

	while (*ptr < (DISK_ADDRESS)ftable + sizeof(ftable))
	{
		if (*ptr)
		{
			File_Header file_header;

			disk_read(*ptr, &file_header, sizeof(file_header));

			memcpy(name, file_header.File_Name, MAX_FILENAME);
			name[MAX_FILENAME] = 0;

			if (file_header.File_Type == TYPE_FILE)
				size_in_bytes = EagleFS_Get_File_Size(*ptr);

			switch (file_header.File_Type)
			{
			case TYPE_FILE:
			{
				printf("%s\t%llu BYTES\t%s\n", name, size_in_bytes, ctime(&file_header.File_Modified_Date));
				break;
			}
			case TYPE_FOLDER:
			{
				printf("%s\tDIR\t%s\n", name, ctime(&file_header.File_Modified_Date));
				break;
			}
			default:
			{
				printf("Should not have gotten here! Got file type %u\n", file_header.File_Type);
				break;
			}
			}
		}
		*ptr++;
	}
}

DISK_ADDRESS EagleFS_Find_File_In_Dir(char *filename, DISK_ADDRESS directory_header)
{
	// If we get a null file header, we want ROOT.
	if (!directory_header)
		directory_header = FILE_HEADERS_START;

	DISK_ADDRESS seek_pointer = directory_header;

	File_Header header;

	disk_read(seek_pointer, &header, sizeof(header));

	DISK_ADDRESS ftable[NUM_FILE_HEADERS_PER_FOLDER];

	disk_read(header.File_Data_Location, &ftable, sizeof(ftable));

	DISK_ADDRESS *ptr = ftable + 1;

	char name[MAX_FILENAME + 1];

	while (*ptr < (DISK_ADDRESS)ftable + sizeof(ftable))
	{
		if (*ptr)
		{
			File_Header file_header;

			disk_read(*ptr, &file_header, sizeof(file_header));

			memcpy(name, file_header.File_Name, MAX_FILENAME);
			name[MAX_FILENAME] = 0;

			if (!strcmp(name, filename))
				return *ptr;
		}
		*ptr++;
	}

	return NULL;
}

DISK_ADDRESS EagleFS_Find_File_From_Path(char *filepath, DISK_ADDRESS parent)
{
	char *path = (char*)malloc(strlen(filepath) + 1);

	strcpy(path, filepath);

	// If no parent is given, use ROOT.
	if (parent == NULL)
		parent = FILE_HEADERS_START;

	char *saveptr = path;

	char *path_part = tokenize(path, path_tokens, &saveptr);

	while (path_part)
	{
		parent = EagleFS_Find_File_In_Dir(path_part, parent);

		if (!parent)
		{
			free(path);
			return NULL;
		}

		path_part = tokenize(NULL, path_tokens, &saveptr);
	}

	free(path);

	return parent;
}

bool EagleFS_isFolder(DISK_ADDRESS file_header)
{
	File_Header header;
	disk_read(file_header, &header, sizeof(header));

	return header.File_Type == TYPE_FOLDER;
}

bool EagleFS_isFile(DISK_ADDRESS file_header)
{
	File_Header header;
	disk_read(file_header, &header, sizeof(header));

	return header.File_Type == TYPE_FILE;
}

bool EagleFS_isDeleted(DISK_ADDRESS file_header)
{
	File_Header header;
	disk_read(file_header, &header, sizeof(header));

	return header.File_Type == TYPE_UNALLOCATED;
}

int file_header_comparator(DISK_ADDRESS *file1, DISK_ADDRESS *file2)
{
	if (!*file1)
		return 1;

	if (!*file2)
		return -1;

	// Get the headers
	File_Header header1, header2;

	if (!disk_read(*file1, &header1, sizeof(header1)))
		return 0;

	if (!disk_read(*file2, &header2, sizeof(header2)))
		return 0;

	char name1[MAX_FILENAME + 1];
	char name2[MAX_FILENAME + 1];

	memcpy(name1, header1.File_Name, MAX_FILENAME);
	memcpy(name2, header2.File_Name, MAX_FILENAME);

	return strcmp(name1, name2);

	//return 1;

	/*if (*file1 > *file2)
	return 1;
	else if (*file1 < *file2)
	return -1;
	else
	return 0;*/
}

uint64_t EagleFS_Get_Folder_Contents(DISK_ADDRESS folder, DISK_ADDRESS *entries)
{
	DISK_ADDRESS folder_entries[NUM_FILE_HEADERS_PER_FOLDER];

	// If we get a null folder, make it ROOT.
	if (!folder)
		folder = FILE_HEADERS_START;

	// This is our file count;
	uint64_t num_files = 0;

	DISK_ADDRESS seek_pointer = folder;

	// Load the folder file header.
	File_Header header;
	disk_read(seek_pointer, &header, sizeof(header));

	// Load the folder ftable into our static ftable buffer.
	disk_read(header.File_Data_Location, &folder_entries, sizeof(folder_entries));

	// Sort the folder entries.
	//qsort(folder_entries + 1, NUM_FILE_HEADERS_PER_FOLDER - 1, sizeof(DISK_ADDRESS), file_header_comparator);

	//while (folder_entries[num_files] && num_files < NUM_FILE_HEADERS_PER_FOLDER - 1)

	uint64_t entry = 0;

	uint64_t i = 1;

	// Iterate through and count.
	for (; i < NUM_FILE_HEADERS_PER_FOLDER; ++i)
	{
		if (folder_entries[i + 1])
		{
			if(entries)
			{
				entries[num_files] = folder_entries[i+1];
			}
			num_files++;
		}
	}

	// Does user want the list of files?
	/*if (entries)
	{
		memcpy(entries, folder_entries+1, sizeof(folder_entries)-sizeof(DISK_ADDRESS));
		//*entries = folder_entries + 1;
	}*/

	// Return the number we found.
	return num_files;
}

const char* EagleFS_Get_File_Name(DISK_ADDRESS file_header)
{
	static char name[MAX_FILENAME + 1];

	// If we get a null file header, set it to ROOT.
	if (!file_header)
		file_header = FILE_HEADERS_START;

	name[0] = NULL;

	// Load file header.
	File_Header header;

	// Read file header in.
	if (!disk_read(file_header, &header, sizeof(header)))
		return name;

	// Now get name.
	memcpy(name, header.File_Name, MAX_FILENAME);
	name[MAX_FILENAME] = NULL;

	return name;
}
