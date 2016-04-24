/*
 * StoneshipMain.cpp
 *
 *  Created on: 26.08.2015
 *      Author: Zalasus
 */

#include <sstream>
#include <iostream>

#include "Stoneship.h"

using namespace Stoneship;

struct RecName {Record::Type type; String name;};
static const RecName RECORD_TYPE_NAMES[] =
{
        {0x0,    "Group"},
        {0xC5,   "Dungeon"},
        {0xD0,   "Entity"},
        {0x820,  "Book"},
        {0x821,  "Stuff"},
        {0xFFF0, "Modify"},
};
static const uint32_t RECORD_TYPE_NAMES_COUNT = sizeof(RECORD_TYPE_NAMES) / sizeof(RecName);

struct SubrecName {Record::Subtype type; String name;};
static const RecName SUBRECORD_TYPE_NAMES[] =
{
        {0x5,    "Model"},
        {0x6,    "Display"},
        {0x7,    "Description"},
        {0x8,    "Trading"},
        {0xA,    "Inventory"},
        {0xB,    "Icon"},
        {0xC,    "Identification"},
        {0xD,    "Count"},
        {0x10,   "Position"},
        {0x11,   "Scale"},
        {0xF0,   "Entity base"},
        {0x105,  "Text"},
        {0xFFF0, "Editor"},
        {0xFFF9, "Modify meta"},
};
static const uint32_t SUBRECORD_TYPE_NAMES_COUNT = sizeof(SUBRECORD_TYPE_NAMES) / sizeof(SubrecName);


static bool listSubrecords = false;
static bool showEdata = false;


static String recordTypeName(Record::Type type)
{
    String pre;

    for(uint32_t i = 0; i < RECORD_TYPE_NAMES_COUNT; ++i)
    {
        if(RECORD_TYPE_NAMES[i].type == type)
        {
            return RECORD_TYPE_NAMES[i].name;

            //pre = RECORD_TYPE_NAMES[i].name + " ";
            //break;
        }
    }

    std::ostringstream s;
    s << pre << "0x" << std::hex << type;
    return s.str();
}

static String subrecordTypeName(Record::Subtype subtype)
{
    String pre;

    for(uint32_t i = 0; i < SUBRECORD_TYPE_NAMES_COUNT; ++i)
    {
        if(SUBRECORD_TYPE_NAMES[i].type == subtype)
        {
            return SUBRECORD_TYPE_NAMES[i].name;

            //pre = SUBRECORD_TYPE_NAMES[i].name + " ";
            //break;
        }
    }

    std::ostringstream s;
    s << pre << "0x" << std::hex << subtype;
    return s.str();
}

static String decodeFlags(RecordHeader::FlagType flags)
{
    std::ostringstream s;

    s << "<";

    if(flags & RecordHeader::FLAG_DELETED)
    {
        s << "DEL ";
    }

    if(flags & RecordHeader::FLAG_ID_PRESENT)
    {
        s << "ID ";
    }

    if(flags & RecordHeader::FLAG_EDATA_PRESENT)
    {
        s << "EDATA ";
    }

    if(flags & RecordHeader::FLAG_BLOB)
    {
        s << "BLOB ";
    }

    if(flags & RecordHeader::FLAG_ATTACHMENT)
    {
        s << "ATACH ";
    }

    if(flags & RecordHeader::FLAG_TOP_GROUP)
    {
        s << "TOP ";
    }


    // just overwrite last space if written any flags to the string
    if(s.tellp() > 1)
    {
        s.seekp(static_cast<uint32_t>(s.tellp()) - 1);
    }

    s << ">";

    return s.str();
}

static void printRecordAndChildren(RecordAccessor record, uint32_t indent)
{
    for(uint32_t i = 0; i < indent; ++i)
    {
        std::cout << "  |";

        if(i == indent - 1)
        {
            std::cout << "-";
        }
    }

    if(record.getHeader().type == Record::TYPE_GROUP)
    {
        std::cout << "[Group] Type=" << recordTypeName(record.getHeader().groupType);
        std::cout << " Children=" << record.getHeader().recordCount << " " << decodeFlags(record.getHeader().flags) << std::endl;

        if(record.hasChildren())
        {
            RecordAccessor child = record.getFirstChildRecord();

            for(uint32_t i = 0; i < record.getHeader().recordCount; ++i)
            {
                printRecordAndChildren(child, indent + 1);

                if(i < record.getHeader().recordCount - 1)
                {
                    child = child.getNextRecord();
                }
            }
        }

    }else
    {
        std::cout << "[" << recordTypeName(record.getHeader().type) << "] ID=0x" << std::hex << record.getHeader().id << std::dec;
        std::cout << " " << decodeFlags(record.getHeader().flags);

        if(showEdata && record.getSubrecordCountForType(Record::SUBTYPE_EDITOR))
        {
            String ename;
            record.getReaderForSubrecord(Record::SUBTYPE_EDITOR) >> ename >> MGFDataReader::endr;
            std::cout << " EName=" << ename;
        }

        std::cout << std::endl;

        if(listSubrecords)
        {
            const SimpleArray<SubrecordHeader> &headers = record.getSubrecordHeaders();
            for(uint32_t i = 0; i < headers.size(); ++i)
            {

                for(uint32_t i = 0; i < indent+1; ++i)
                {
                    std::cout << "  |";

                    if(i == indent)
                    {
                        std::cout << "-";
                    }
                }

                std::cout << "<" << subrecordTypeName(headers[i].type) << "> " << headers[i].dataSize << " bytes" << std::endl;
            }
        }
    }
}

static void usage()
{
    std::cout << "Usage: demgf [Options] <MGF filename>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "    -s    List subrecords" << std::endl;
    std::cout << "    -e    Show editor names if present" << std::endl;
    std::cout << "    -i    Let Stoneship build index before printing" << std::endl;
    std::cout << "    -h    Show this message" << std::endl;
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        usage();

        return 0;
    }

    bool indexFile = false;

    int32_t fileIndex = -1;
    for(int32_t i = 1; i < argc; ++i)
    {
        String s(argv[i]);

        if(s[0] == '-')
        {
            // this is an option

            if(StringUtils::indexOf(s, 's') >= 0)
            {
                listSubrecords = true;
            }

            if(StringUtils::indexOf(s, 'e') >= 0)
            {
                showEdata = true;
            }

            if(StringUtils::indexOf(s, 'i') >= 0)
            {
                indexFile = true;
            }

            if(StringUtils::indexOf(s, 'h') >= 0)
            {
                usage();

                return 0;
            }

        }else
        {
            // this is an MGF path
            fileIndex = i;
        }
    }

    if(fileIndex == -1)
    {
        usage();
        std::cout << "Error: No input file given" << std::endl;

        return 0;
    }

    // we don't need the logger here
    Logger::getDefaultLogger().setOutputStream(nullptr);

    try
	{
        MasterGameFile mgf(String(argv[fileIndex]), 0);
        mgf.load(true, indexFile);

        //std::cout << "Registered Entities: " << EntityBaseFactory::getRegisteredFactoryCount() << std::endl;

        std::cout << std::endl;

        std::cout << "Author:        " << mgf.getAuthor() << std::endl;
        std::cout << "Description:   " << mgf.getDescription() << std::endl;
        std::cout << "Created:       " << std::asctime(mgf.getTimestamp()); // CRLF is included
        std::cout << "Depends on:    ";
        for(uint32_t i = 0; i < mgf.getDependencyCount(); i++)
        {
            std::cout << "[" <<  mgf.getDependencies()[i].ordinal << "]" << mgf.getDependencies()[i].filename;

            if(i+1 < mgf.getDependencyCount())
            {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        std::cout << "Record groups: " << mgf.getRecordGroupCount() << std::endl;

        std::cout << std::endl;

        RecordAccessor record = mgf.getFirstRecord();

        for(uint32_t i = 0; i < mgf.getRecordGroupCount(); ++i)
        {
            printRecordAndChildren(record, 0);

            if(i < mgf.getRecordGroupCount() - 1)
            {
                record = record.getNextRecord();
            }
        }


	}catch(Stoneship::StoneshipException &e)
	{
	    std::cerr << std::endl << "Error: " << e.getMessage() << std::endl;

	    return -1;
	}


	return 0;
}


