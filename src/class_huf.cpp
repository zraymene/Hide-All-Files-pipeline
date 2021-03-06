/**
 *  Copyright 2020 by Zeroual Aymene <ayemenzeroual@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 * FILE: class_huf.cpp
 *
 * DECRP:
 *	- Functions defintions
 *
 */

#include "class_huf.h"

void HUF_Bank::M_DisplayError()
{
	printf("HUF Bank %s : %s", this->file_name, HUF_GetError());
}

bool HUF_Bank::Init(const char* file_name)
{

	this->file_name = file_name;
	this->chunks_size = 0;
	
	if (!HUF_LoadHeader(file_name, &this->m_header))
	{
		this->M_DisplayError();
		return false;
	}

	if (!HUF_LoadDirsTable(this->file_name, this->m_header, &this->m_dirs_table, &this->chunks_size))
	{
		this->M_DisplayError();
		return false;
	}



	printf("HUF Bank %s : Loaded ! - %d \n",this->file_name , HUF_GetExcutionTime());

	printf("Directories Type : %s \nDirectories Version : %d \nDirectories Number : %d\nDirectories Pointer : %x\n\n",
		this->m_header->bank_type, this->m_header->bank_version,
		this->m_header->dirs_num, this->m_header->dirs_pointer);

	return true;
}

bool HUF_Bank::Add(
					const char* name,
					char*		buffer,
					uint32_t		buffer_size
				  )
{
	if (!HUF_AddChunk(
			this->file_name, this->m_header,
			name           , buffer,
			buffer_size    , &this->m_dirs_table
	))
	{
		this->M_DisplayError();
		return false;
	}

	printf("HUF Bank %s : Chunk %s Added ! - %i \n",
		this->file_name,
		name,
		HUF_GetExcutionTime());

	return true;
}

bool HUF_Bank::Get(
					const char* dir_name,
					char* out_buffer,
					uint32_t* out_buffer_size
				  )
{
	if (!HUF_LoadChunk(
			this->file_name, this->m_header,
			dir_name       , this->m_dirs_table,
			out_buffer     , out_buffer_size
	))
	{
		this->M_DisplayError();
		return false;
	}

	printf("HUF Bank %s : Chunk %s Loaded ! - %i \n",
		this->file_name,
		dir_name,
		HUF_GetExcutionTime());

	return true;
}

bool HUF_Bank::Remove(const char* desired_dir_name)
{

	if (!HUF_DeleteChunk(
		this->file_name , this->m_header,
		desired_dir_name, this->m_dirs_table
	))
	{
		this->M_DisplayError();
		return false;
	}

	printf("HUF Bank %s : Chunk %s Removed ! - %i \n",
		this->file_name,
		desired_dir_name,
		HUF_GetExcutionTime());

	return true;
}

void HUF_Bank::PrintDirectories()
{

	for (int i = 0; i < this->m_header->dirs_num; i++)
	{
		printf("File : %s , Size : %d Byte.\n", (this->m_dirs_table + i)->chunk_name, (this->m_dirs_table + i)->chunk_size);
	}
	printf("\0");
}

void HUF_Bank::Destory()
{
	if (this->m_header != nullptr)
		delete this->m_header;

	if (this->m_dirs_table != nullptr)
		delete[] this->m_dirs_table;
}
