#include <stdio.h>
#include <string.h>
#include <rapidjson/document.h>
#include <beta/mmap_file.h>
#include <low/low.h>
#include <stdlib.h>

using namespace z::beta;

static void do_simple_parse(const char * json);

int main(int argc, char * argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <json file>\n", argv[0]);
        return -1;
    }

    ZLOG(LOG_INFO, "mmaping.");
    MMapFile file(argv[1]);
    if (!file.mmap()) {
        ZLOG(LOG_FATAL, "read file [%s] failed.", argv[1]);
        return -2;
    }

    ZLOG(LOG_INFO, "mmaping done. read content.");
    
    char * json = new char[1 + file.size()];
    memcpy(json, file.data(), file.size() );
    json[file.size()] = 0;
    
    ZLOG(LOG_INFO, "read done. parsing...");
    
    do_simple_parse(json);
    
    ZLOG(LOG_INFO, "parsing done.");

    return 0;
}

static void do_simple_parse(const char * json)
{
    rapidjson::Document doc;
    doc.Parse<0>(json);
    if (doc.HasParseError() ) {
        ZLOG(LOG_WARN, "parse error at[%d: %s] message[%s]", 
            doc.GetErrorOffset(), json + doc.GetErrorOffset(), doc.GetParseError() );
        return ;
    }
    ZLOG(LOG_INFO, "Parse Done.");
    ZLOG(LOG_INFO, "read id: [%d]", doc["id"].GetInt() );
    ZLOG(LOG_INFO, "string: %s", doc["list_a"][3].GetString() );
}


