/**
 *  Copyright 2020 by Zeroual Aymene <ayemenzeroual@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 * FILE: haf_util.cpp
 *
 * DECRP:
 *	- Functions difintions
 *  - tiny error handling system
 *	- Execution timer
 *
 * NOTE:
 *	- M_ prefixes means it's member of this unit and can't be acces from outside
 *
 */

#include "haf_util.h"

#include <cstring>
#include <chrono>
#include <memory>

#define HEADER_SIZE sizeof(header_t)

std::chrono::steady_clock::time_point ELAPSED_TIME_BEGIN;
long								  ELAPSED_TIME;

char* ERROR_STR;

void M_StartChrono()
{
	ELAPSED_TIME_BEGIN = std::chrono::steady_clock::now();
}

void M_EndChrono()
{
	
	ELAPSED_TIME = (long) std::chrono::duration_cast<std::chrono::milliseconds>
						(std::chrono::steady_clock::now() - ELAPSED_TIME_BEGIN).count();
}

void M_PushError(const char* error_msg, int line)
{
	if (ERROR_STR != nullptr)
		delete[] ERROR_STR;

	ERROR_STR = new char[strlen(error_msg) + 7];

	*ERROR_STR = std::sprintf(ERROR_STR, "%s-%d", error_msg, line);
}

bool M_FetchDirectory(
						directory_t* dir_table,        uint32_t        size,
						const char*  wanted_dir_name , directory_t** out_wanted_dir,
						uint32_t*         out_dir_positon
					)
{

	M_StartChrono();

	for (int i = 0; i < size; i++)
	{
		if (!strcmp((dir_table + i)->chunk_name, wanted_dir_name))
		{
			
			if(out_wanted_dir != nullptr)
				*out_wanted_dir = (dir_table + i);

			if(out_dir_positon != nullptr)
				*out_dir_positon = i ;

			return true;
		}
	}

	M_EndChrono();

	return false;

}

FILE* HUF_CreateEmpty(const char* filename , header_t ** out_header)
{
	FILE* file;

	if ((file = std::fopen(filename, "wb")) == nullptr)
	{
		M_PushError("HUF Erro r : Can't create empty bank !", __LINE__);
		return nullptr;
	}

	header_t* header = new header_t();
	header->dirs_num     = 0;
	header->dirs_pointer = HEADER_SIZE;
	header->bank_version = 1;
	std::strcpy(header->bank_type, "G");

	if ( std::fwrite(header, sizeof(header_t) , 1 , file) != 1 )
	{
		M_PushError("HUF Error : Can't create empty bank !", __LINE__);

		fclose(file);
		delete header;

		return nullptr;
	}

	if( out_header != nullptr )
		*out_header = header;

	return file;
}

//
// Opens and loades the header file
//
// TODO:
//  - Check file opening permissions (w , r, ...) if it rewrite 
//    exisintg file or something 
//
bool HUF_LoadHeader(const char* filename, header_t** out_header )
{

	M_StartChrono();

	FILE* file;

	if ( (file = std::fopen(filename, "rb+")) == nullptr)
	{
		if ((file = HUF_CreateEmpty(filename, out_header)) == nullptr)
		{
			M_PushError("HUF Error : Can't create empety The Bank File !", __LINE__);
			return false;
		}
	}
	else {

		if (*out_header == nullptr)
			*out_header = new header_t();


		// Read The Bank header in header_t struct
		if (std::fread(*out_header, sizeof(header_t), 1, file) == 0)
		{
			M_PushError("HUF Error : Error while reading Bank header !", __LINE__);
			return false;
		}

	}

	std::fclose(file);

	M_EndChrono();

	return true;
}

//
// Loads Directories Table  + calculate total chunks size
//
// TODO :
//  - Check for the best way of reading directories table ( commented )
//
bool HUF_LoadDirsTable(
				const char*   filename,       header_t* header,
				directory_t** out_dirs_table, uint32_t* toatl_chunk_size)
{

	std::FILE * file;
	*toatl_chunk_size		 = 0;
	uint32_t dirs_table_size = 0;

	M_StartChrono();

	if (header->dirs_num != 0)
	{

		if ((file = std::fopen(filename, "rb+")) == nullptr)
		{
			M_PushError("HUF Error : Can't open The Bank File !", __LINE__);
			return false;
		}

		dirs_table_size = sizeof(directory_t) * header->dirs_num;

		// Alloc array to hold all directories
		*out_dirs_table = new directory_t[dirs_table_size];

		// Or you this 
		
		std::fseek(file, header->dirs_pointer, SEEK_SET);

		std::fread(*out_dirs_table, sizeof(directory_t), header->dirs_num, file);
		

		std::fclose(file);
	
	}
	M_EndChrono();

	return true;
}

//
// Load a desired chunk by directory name , return chunk buffer + chunk size
//
bool HUF_LoadChunk(
					const char* filename,          header_t*    header ,
					const char* wanted_chunk_name, directory_t* dirs_table,
					char*       out_chunk_buffer,  uint32_t*    out_chunk_size
				  )
{

	std::FILE* file;
	directory_t* dir_wanted;

	M_StartChrono();

	if ((file = std::fopen(filename, "rb")) == nullptr)
	{
		M_PushError("HUF Error : Can't open The Bank File !", __LINE__);
		return false;
	}

	if (!M_FetchDirectory(dirs_table, header->dirs_num, wanted_chunk_name , &dir_wanted , nullptr))
	{
		M_PushError("HUF Error : Can't find desired chunk/directory !", __LINE__);

		std::fclose(file);

		return false;
	}

	out_chunk_buffer = new char[dir_wanted->chunk_size];
	*out_chunk_size = dir_wanted->chunk_size;

	if (std::fseek(file, (long) dir_wanted->chunk_pointer, SEEK_SET) != 0)
	{
		M_PushError("HUF Error : Can't seek to chunk position !", __LINE__);
		
		delete[] out_chunk_buffer;
		std::fclose(file);

		return false;
	}

	if (fread(out_chunk_buffer, dir_wanted->chunk_size, 1, file) == 0)
	{
		M_PushError("HUF Error : Can't read chunk position !", __LINE__);

		delete[] out_chunk_buffer;

		std::fclose(file);

		return false;
	}

	fclose(file);
	
	M_EndChrono();

	return true;
}

//
// Add new chunk to The Bank File 
// Return new header , dirs_table (As args of course)
// 
// TODO : Memory leaks is highly possible hir , recheck !
//
bool HUF_AddChunk(
					const char* filename,		 header_t*	  header,
					const char* chunk_name,		 char*		  chunk_buffer,
					uint32_t         chunk_size, directory_t** dirs_table
				 )
{

	std::FILE* file;

	M_StartChrono();

	if ((file = std::fopen(filename, "rb+")) == nullptr)
	{
		M_PushError("HUF Error : Can't open The Bank File !", __LINE__);
		return false;
	}

	// Look if directory with same name but we don't need any return value !
	if (M_FetchDirectory(*dirs_table, header->dirs_num, chunk_name, nullptr , nullptr))
	{
		M_PushError("HUF Error : Chunk with same name already exists ! !", __LINE__);

		std::fclose(file);

		return false;
	}

	// New directory 
	directory_t* dir_tmp = new directory_t();
	strcpy(dir_tmp->chunk_name, chunk_name);
	dir_tmp->chunk_size = chunk_size;
	dir_tmp->chunk_pointer = header->dirs_pointer;

	// Buffer to hold all existing directories plus a place for new directory
	uint32_t dirs_tmp_buff_size = (header->dirs_num + 1) * sizeof(directory_t);
	directory_t* dirs_tmp_buffer = new directory_t[dirs_tmp_buff_size];

	// Copy old table to the new !
	for (int i = 0; i < header->dirs_num; i++)
	{
		*(dirs_tmp_buffer + i) = **(dirs_table + i);
	}

	// Add new directory to then end of dirs table
	*(dirs_tmp_buffer + header->dirs_num) = *dir_tmp;

	//###################################################
	// Go to new chunk position
	//###################################################
	if (std::fseek(file, (long) dir_tmp->chunk_pointer, SEEK_SET) != 0)
	{
		M_PushError("HUF Error : Can't seek to new chunk position !", __LINE__);

		delete dir_tmp; delete[] dirs_tmp_buffer;
		std::fclose(file);

		return false;
	}


	// Write the new chunk !
	if ( std::fwrite(chunk_buffer, sizeof(char), chunk_size, file) != chunk_size)
	{
		M_PushError("HUF Error : Error while writing new chunk ! !", __LINE__);

		delete dir_tmp; delete[] dirs_tmp_buffer;
		std::fclose(file);

		return false;
	}

	//#####################################################
	// New directories tabke turn , seek to the new positon
	//#####################################################
	if (std::fseek(file, (long) (dir_tmp->chunk_pointer + ( dir_tmp->chunk_size - 1)), SEEK_SET) != 0)
	{
		M_PushError("HUF Error : Can't seek to new directories table position !", __LINE__);

		delete dir_tmp; delete[] dirs_tmp_buffer;
		std::fclose(file);

		return false;
	}
	// Write the new table !
	if (std::fwrite(dirs_tmp_buffer, sizeof(directory_t), (size_t) (header->dirs_num) + 1, file) <= 0)
	{
		M_PushError("HUF Error : Error while writing new table ! !", __LINE__);

		delete dir_tmp; delete[] dirs_tmp_buffer;
		std::fclose(file);

		return false;
	}

	//#####################################################
	// New header turn !!
	//#####################################################
	header->dirs_num += 1;
	header->dirs_pointer += (chunk_size - 1);
	// Go to the begining !
	if (std::fseek(file, 0, SEEK_SET) != 0)
	{
		M_PushError("HUF Error : Can't seek to the header positon !", __LINE__);

		delete dir_tmp; delete[] dirs_tmp_buffer;
		std::fclose(file);

		return false;
	}
	// Now overwrite !
	if (std::fwrite(header, sizeof(header_t) , 1, file) <= 0)
	{
		M_PushError("HUF Error : Error while writing new header ! !", __LINE__);

		delete dir_tmp; delete[] dirs_tmp_buffer;
		std::fclose(file);

		return false;
	}

	// Free old table
	delete *dirs_table;
	// Now it's the new table !
	*dirs_table = dirs_tmp_buffer;

	std::fclose(file);

	M_EndChrono();

	return true;
}

//
//	Delete a desired chunk by directory name , returns updated header , directories table 
//
// TODO: Leeks hir are huge whene there is an error !
//
bool HUF_DeleteChunk(
						const char* filename,   header_t* header,
						const char* chunk_name, directory_t* dirs_table
					)
{

	std::FILE* file;
	std::FILE* file2;

	M_StartChrono();

	directory_t*  wanted_dir ;
	uint32_t		 wanted_dir_pos;

	if ((file = std::fopen(filename, "ab+")) == nullptr)
	{
		M_PushError("HUF Error : Can't open The Bank File !", __LINE__);
		return false;
	}

	// Look if directory exists !
	if (!M_FetchDirectory(dirs_table, header->dirs_num, chunk_name, &wanted_dir , &wanted_dir_pos))
	{
		M_PushError("HUF Error : Chunk desired to delete doesn't exists ! !", __LINE__);
		return false;
	}

	//#####################################################
	// First part of chunks , before desired chunk!!
	//#####################################################
	// Calc first buffer size  from end of header -> start of desired chunk 
	// Then allocate 
	uint32_t buff1_size = wanted_dir->chunk_pointer - sizeof(header_t);
	char* buff1     = new char[buff1_size];
	//-----------------------------------------------------
	// No seek to end of header part 
	if (std::fseek(file, sizeof(header_t) , SEEK_SET) != 0)
	{
		M_PushError("HUF Error - Delete : Can't seek to the end of header part !", __LINE__);

		delete[] buff1;

		return false;
	}
	// Read until start of desired chunk !
	if (fread(buff1 , sizeof(char) , buff1_size , file) == 0)
	{
		M_PushError("HUF Error - Delete : Can't read chunks to buffer1 !", __LINE__);

		delete[] buff1;

		return false;
	}

	//#####################################################
	// Second part of data - chunks , after desired chunk!!
	//#####################################################
	// Calc second buffer size  from end of desired chunk -> start of desired chunk directory
	// Then allocate 
	uint32_t buff2_begin = wanted_dir->chunk_pointer + wanted_dir->chunk_size;
	uint32_t buff2_end   = header->dirs_pointer + (wanted_dir_pos * ( sizeof(directory_t) - 1 ) );
	uint32_t buff2_size  = buff2_end - buff2_begin;
	char* buff2      = new char[buff2_size];
	//-----------------------------------------------------
	// Now seek to end of header part 
	if (std::fseek(file, (long) buff2_begin , SEEK_SET) != 0)
	{
		M_PushError("HUF Error - Delete : Can't seek to the end of desired chunk  !", __LINE__);

		delete[] buff1;
		delete[] buff2;

		return false;
	}
	// Read until start of desired chunk directory !
	if (fread(buff2, sizeof(char), buff2_size, file) == 0)
	{
		M_PushError("HUF Error - Delete : Can't read chunks to buffer2 !", __LINE__);

		delete[] buff1;
		delete[] buff2;

		return false;
	}
	
	// Both parts are done , good now write them to the new file
	// Before edit header 
	// Set the new directires table position
	header->dirs_num -= 1;
	header->dirs_pointer -= wanted_dir->chunk_size;
	
	char tmp_file_name[13] = "tmp-bank.bnk";

	if ( (file2 = std::fopen(tmp_file_name, "w")) == nullptr)
	{
		M_PushError("HUF Error - Delete : Can't open temporary bank file !", __LINE__);

		delete[] buff1;
		delete[] buff2;

		return false;
	}

	//-----------------------------------------------------
	// Write new header
	//-----------------------------------------------------
	/*if (std::fseek(file2, 0, SEEK_SET) != 0)
	{
		M_PushError("HUF Error - Delete : Can't seek to the start of file!");
		return false;
	}
	*/
	if (std::fwrite(header, sizeof(header_t), 1, file2) <= 0)
	{
		M_PushError("HUF Error - Delete : Can't write header to new bank file!", __LINE__);

		delete[] buff1;
		delete[] buff2;
		std::remove(tmp_file_name);

		return false;
	}

	//-----------------------------------------------------
	// Write first buffer
	//-----------------------------------------------------
	/*if (std::fseek(file2, sizeof(header_t) , buff1_size , SEEK_SET) != 0)
	{
		M_PushError("HUF Error - Delete : Can't seek to the end of header - writing to TMP!");
		return false;
	}
	*/
	if (std::fwrite(buff1, sizeof(char), buff1_size, file2) <= 0)
	{
		M_PushError("HUF Error - Delete : Can't write first buffer - writing to TMP!", __LINE__);

		delete[] buff1;
		delete[] buff2;
		std::remove(tmp_file_name);

		return false;
	}
	delete[] buff1;

	//-----------------------------------------------------
	// Write second buffer
	//-----------------------------------------------------
	/*if (std::fseek(file2, sizeof(header_t) + buff1_size - 1, buff2_size , SEEK_SET) != 0)
	{
		M_PushError("HUF Error - Delete : Can't seek to the end of header - writing to TMP!");
		return false;
	}
	*/
	if (std::fwrite(buff2, sizeof(char), buff2_size, file2) <= 0)
	{
		M_PushError("HUF Error - Delete : Can't write second buffer - writing to TMP!", __LINE__);

		delete[] buff2;
		std::remove(tmp_file_name);

		return false;
	}
	delete[] buff2;

	//#####################################################
	// Third part of data - chunks , after desired chunk directory!!
	//#####################################################
	// First check if desired chunk directory isn't the last or exit
	if (wanted_dir_pos != (header->dirs_num - 1))
	{

		if (std::fseek(file2, 0, SEEK_SET) != 0)
		{
			M_PushError("HUF Error - Delete : Can't seek to the end of header - writing to TMP!", __LINE__);
			return false;
		}

		uint32_t buff3_end   = std::ftell(file2); // Get the end of the file
		uint32_t buff3_begin = buff2_end + sizeof(directory_t);
		uint32_t buff3_size = buff3_end - buff3_begin;

		char* buff3 = new char[buff3_size];

		//-----------------------------------------------------
		// Seek to end of desired chunk directory
		if (std::fseek(file, (long) buff2_begin, SEEK_SET) != 0)
		{
			M_PushError("HUF Error - Delete : Can't seek to the end of desired chunk directory !", __LINE__);

			delete[] buff3;
			std::remove(tmp_file_name);

			return false;
		}
		// Read until start of desired chunk directory !
		if (fread(buff3, sizeof(char), buff3_size, file) == 0)
		{
			M_PushError("HUF Error - Delete : Can't read chunks to buffer3 !", __LINE__);

			delete[] buff3;
			std::remove(tmp_file_name);

			return false;
		}

		//-----------------------------------------------------
		// Write third buffer
		//-----------------------------------------------------
		/*if (std::fseek(file2, sizeof(header_t) + buff1_size - 1, buff2_size , SEEK_SET) != 0)
		{
			M_PushError("HUF Error - Delete : Can't seek to the end of header - writing to TMP!");
			return false;
		}
		*/
		if (std::fwrite(buff3, sizeof(char), buff3_size, file2) <= 0)
		{
			M_PushError("HUF Error - Delete : Can't write third buffer - writing to TMP!", __LINE__);

			delete[] buff3;
			std::remove(tmp_file_name);

			return false;
		}

		delete[] buff3;
	}

	// Phewww , finaly!
	// Now delete original file
	// Then rename the tmp file to the original name
	fclose(file);
	fclose(file2);

	std::remove(filename);
	std::rename(tmp_file_name, filename);

	M_EndChrono();

	return true;
}

const char* HUF_GetError()
{
	return ERROR_STR;
}

long HUF_GetExcutionTime()
{
	return ELAPSED_TIME;
}

void HUF_Exit()
{
	if (ERROR_STR != nullptr)
		delete[] ERROR_STR;

}

