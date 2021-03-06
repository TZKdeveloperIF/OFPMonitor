//---------------------------------------------------------------------------

#ifndef ConfigReaderH
#define ConfigReaderH

#include <vcl.h>
#include <list.h>

String getValue(String in);
bool checkBool2(String in);

enum DataType { dtInt, dtFloat, dtBool, dtString, dtStringQuoted, dtServerItem};

class ConfigEntry {
        public:
                DataType dt;
                void *data;
                String ident;

                ConfigEntry(String ident, DataType dt, void *dataPointer);
                bool check(String line);
        private:
                String getValue(String in);
                String extractQuotedValue(String value);
};

class ConfigSection {
        private:
                list<ConfigEntry*> items;
                String name;
        public:
                ConfigSection(String name);
                ~ConfigSection();
                void add(ConfigEntry *ce);
                int scan(TStringList *list, int lineIndex);
};


//---------------------------------------------------------------------------
#endif
