/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file FmtDefinitionTable.cpp
 * @brief The table of definitions of FMTs
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>


#include "FmtDefinitionTable.h"

#include <fstream>
#include <sstream>

using std::stringstream;

/// The maximum entries number in FMT definitions table
#define MAX_FMT 32


// Returns false if the string contains any non-whitespace characters
inline bool isSpace(string str)
{
    string::iterator it = str.begin();
    while(it != str.end())
    {
    	if(!std::isspace(*it))
    	{
    		return false;
		}
		++it;
	}
    return true;
}

/**
 * @brief Create a table of FMT definitions
 */
FmtDefinitionTable::FmtDefinitionTable():
	definitions()
{
}


/**
 * @brief Destroy a table of FMT definitions
 */
FmtDefinitionTable::~FmtDefinitionTable()
{
	this->clear();
}


bool FmtDefinitionTable::load(const string filename)
{
	std::ifstream file;
	unsigned int lines_count;
	int nb_fmt;
	bool is_nb_fmt_found = false;
	unsigned int nb_fmt_read = 0;

	// first, clear all the current FMT definitions
	this->clear();

	// open the FMT definition file
	file.open(filename.c_str());
	if(!file.is_open())
	{
		UTI_ERROR("failed to open the FMT definition file '%s'\n",
		          filename.c_str());
		goto error;
	}

	// read every line of the file
	lines_count = 0;
	while(!file.eof())
	{
		string line;
		stringstream line_stream;
		string token;

		// get the full line
		getline(file, line);

		// skip line if empty
		if(isSpace(line))
		{
			continue;
		}

		lines_count++;

		// get the first keyword of the line
		line_stream.str(line);
		line_stream >> token;
		if(token == "/*")
		{
			// the line starts with a comment, skip it
		}
		else if(token == "nb_fmt")
		{
			// the line starts with the 'nb_fmt' keyword
			if(is_nb_fmt_found)
			{
				// this is not the first line that starts with
				// the 'nb_fmt' keyword
				UTI_ERROR("bad syntax at line %u (%s): "
				          "multiple lines starting "
				          "with the 'nb_fmt' keyword\n",
				          lines_count, line.substr(0,16).c_str());
				goto malformed;
			}
			else
			{
				// this is the first line that starts with
				// the 'nb_fmt' keyword
				string equal;

				// get the equal symbol and the number of FMTs
				line_stream >> equal >> nb_fmt;

				// some checks on read values
				if(equal != "=")
				{
					UTI_ERROR("bad syntax at line %u (%s): "
					          "the 'nb_fmt' "
					          "keyword should be followed by an "
					          "equal symbol\n", lines_count,
					          line.substr(0,16).c_str());
					goto malformed;
				}
				if(nb_fmt <= 0 || nb_fmt > MAX_FMT)
				{
					UTI_ERROR("bad syntax at line %u (%s): "
					          "the number of FMTs "
					          "should be a non-zero positive "
					          "value under %d\n",
					          lines_count,
					          line.substr(0,16).c_str(), MAX_FMT);
					goto malformed;
				}

				// line format is valid
				UTI_INFO("%d FMTs present in definition "
				         "file\n", nb_fmt);
				is_nb_fmt_found = true;
				nb_fmt_read = 0;
			}
		}
		else
		{
			// the first keyword should be a positive integer
			stringstream token_stream;
			unsigned int scheme_number;
			string modulation;
			string coding_rate;
			float spectral_efficiency;
			float required_es_n0;
			int ret;

			// convert the string token to integer
			token_stream.str(token);
			token_stream >> scheme_number;
			if(scheme_number <= 0 || scheme_number > MAX_FMT)
			{
				UTI_ERROR("bad syntax at line %u (%s): FMT "
				          "definition should start with a "
				          "non-zero positive integer up to %u\n",
				          lines_count, line.substr(0,16).c_str(),
				          MAX_FMT);
				goto malformed;
			}

			// check if the number of FMTs was found
			if(!is_nb_fmt_found)
			{
				// the 'nb_fmt' keyword should be
				// specified before any FMTs line
				UTI_ERROR("bad syntax at line %u (%s): FMT "
				          "definition before the 'nb_fmt' "
				          "keyword\n", lines_count,
				          line.substr(0,16).c_str());
				goto malformed;
			}

			// one more FMT found
			nb_fmt_read++;
			if(nb_fmt_read > ((unsigned int) nb_fmt))
			{
				UTI_ERROR("bad syntax at line %u (%s): %u or more "
				          "FMTs definitions found, but "
				          "only %d specified with the "
				          "'nb_fmt' keyword\n",
				          lines_count, line.substr(0,16).c_str(),
				          nb_fmt_read, nb_fmt);
				goto malformed;
			}

			// get all the parameters of the FMT
			line_stream >> modulation
			            >> coding_rate
			            >> spectral_efficiency
			            >> required_es_n0;

			// FMT definition is OK, record it in the table
			ret = this->add(scheme_number, modulation, coding_rate,
			                spectral_efficiency, required_es_n0);
			if(ret != true)
			{
				UTI_ERROR("failed to add new FMT definition: "
				          "%u, %s, %s, %f, %f\n",
				          scheme_number, modulation.c_str(),
				          coding_rate.c_str(),
				          spectral_efficiency,
				          required_es_n0);
				goto malformed;
			}

			UTI_INFO("FMT definition: %u, %s, %s, %f, %f\n",
			         scheme_number, modulation.c_str(),
			         coding_rate.c_str(), spectral_efficiency,
			         required_es_n0);
		}
	}

	// check the number of FMTs read in definition file
	if(nb_fmt_read != ((unsigned int) nb_fmt))
	{
		UTI_ERROR("too few FMTs definitions: "
		          "%u found while %d specified with the "
		          "'nb_fmt' keyword\n",
		          nb_fmt_read, nb_fmt);
		goto malformed;
	}
	UTI_INFO("%d FMTs found in definition file\n",
	         nb_fmt);

	// close the definition file
	file.close();

	return true;

malformed:
	UTI_ERROR("malformed FMT definition file\n");
	file.close();
error:
	return false;
}


bool FmtDefinitionTable::add(const unsigned int id,
                             const string modulation,
                             const string coding_rate,
                             const float spectral_efficiency,
                             const float required_Es_N0)
{
	map<unsigned int, FmtDefinition *>::iterator it;
	FmtDefinition *new_def;

	// check that the table does not already own a FMT definition
	// with the same identifier
	if(this->doFmtIdExist(id))
	{
		return false;
	}

	// create the new FMT definition
	new_def = new FmtDefinition(id, modulation, coding_rate,
	                            spectral_efficiency, required_Es_N0);
	if(new_def == NULL)
	{
		return false;
	}

	this->definitions[id] = new_def;

	return true;
}


bool FmtDefinitionTable::doFmtIdExist(unsigned int id) const
{
	return (this->definitions.find(id) != this->definitions.end());
}


void FmtDefinitionTable::clear()
{
	map<unsigned int, FmtDefinition *>::iterator it;

	// delete all stored FMT definitions
	for(it = this->definitions.begin(); it != this->definitions.end(); it++)
	{
		delete it->second;
	}

	// now clear the map itself
	this->definitions.clear();
}


modulation_type_t FmtDefinitionTable::getModulation(unsigned int id) const
{
	FmtDefinition *def = this->getFmtDef(id);
	if(!def)
	{
		return MODULATION_UNKNOWN;
	}
	return def->getModulation();
}


string FmtDefinitionTable::getCodingRate(unsigned int id) const
{
	FmtDefinition *def = this->getFmtDef(id);
	if(!def)
	{
		return "";
	}
	return def->getCodingRate();
}


float FmtDefinitionTable::getSpectralEfficiency(unsigned int id) const
{
	FmtDefinition *def = this->getFmtDef(id);
	if(!def)
	{
		return 0.0;
	}
	return def->getSpectralEfficiency();
}


float FmtDefinitionTable::getRequiredEsN0(unsigned int id) const
{
	FmtDefinition *def = this->getFmtDef(id);
	if(!def)
	{
		return 0.0;
	}
	return def->getRequiredEsN0();
}

FmtDefinition *FmtDefinitionTable::getFmtDef(unsigned int id) const
{
	fmt_def_table_pos_t it;
	FmtDefinition *def = NULL;

	it = this->definitions.find(id);
	if(it != this->definitions.end())
	{
		def = it->second;
	}
	if(!def)
	{
		UTI_ERROR("cannot find FMT definition for ID %u\n", id);
	}

	return def;
}


unsigned int FmtDefinitionTable::symToKbits(unsigned int id,
                                            unsigned int val_sym) const
{
	FmtDefinition *def = this->getFmtDef(id);
	modulation_type_t modulation;
	string coding_rate;
	unsigned int mod = 0;
	float cod = 0.0;

	if(!def)
	{
		return 0;
	}
	modulation = def->getModulation();
	coding_rate = def->getCodingRate();

	switch(modulation)
	{
		case MODULATION_BPSK:
			mod = 1;
			break;
		case MODULATION_QPSK:
			mod = 2;
			break;
		case MODULATION_8PSK:
			mod = 3;
			break;
		case MODULATION_16APSK:
			mod = 4;
			break;
		case MODULATION_32APSK:
			mod = 5;
			break;
		default:
			return 0;
	}

	if(coding_rate == "1/4")
	{
		cod = 1.0/4.0;
	}
	else if(coding_rate == "1/3")
	{
		cod = 1.0/3.0;
	}
	else if(coding_rate == "2/5")
	{
		cod = 2.0/5.0;
	}
	else if(coding_rate == "1/2")
	{
		cod = 1.0/2.0;
	}
	else if(coding_rate == "3/5")
	{
		cod = 3.0/5.0;
	}
	else if(coding_rate == "2/3")
	{
		cod = 2.0/3.0;
	}
	else if(coding_rate == "3/4")
	{
		cod = 3.0/4.0;
	}
	else if(coding_rate == "4/5")
	{
		cod = 4.0/5.0;
	}
	else if(coding_rate == "5/6")
	{
		cod = 5.0/6.0;
	}
	else if(coding_rate == "6/7")
	{
		cod = 6.0/7.0;
	}
	else if(coding_rate == "8/9")
	{
		cod = 8.0/9.0;
	}
	else if(coding_rate == "9/10")
	{
		cod = 9.0/10.0;
	}

	return ceil(val_sym * mod * cod/1000);
}