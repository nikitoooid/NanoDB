#pragma once
#include <Arduino.h>

#if __has_include(<LittleFS.h>)
  #include <LittleFS.h>
  #define NANOFS LittleFS
#elif __has_include(<SPIFFS.h>)
  #include <SPIFFS.h>
  #define NANOFS SPIFFS
#else
  #error "NanoDB: LittleFS or SPIFFS required"
#endif

#define NANO_MAX_COLS 16
#define NANO_MAX_STR_LEN 128

struct ColumnDef {
  String name;   // column name
  char type;     // 'I','F','S','B'
  uint16_t size; // for 'S' : string length; otherwise ignored
};

class NanoRecord {
public:
  NanoRecord();
  ~NanoRecord();

  // attach/detach to a table schema (called by NanoTable::newRecord)
  void attach(const ColumnDef *cols, uint8_t colCount, uint16_t rowSize);
  void detach(); // free internal buffer

  // Field proxy for rec["col"] = val and conversions
  class FieldProxy {
  public:
    FieldProxy(NanoRecord &rec, int idx) : _rec(rec), _idx(idx) {}
    FieldProxy& operator=(const String &v) { _rec.setString(_idx, v); return *this; }
    FieldProxy& operator=(const char *v) { _rec.setString(_idx, String(v)); return *this; }
    FieldProxy& operator=(int32_t v) { _rec.setInt(_idx, v); return *this; }
    FieldProxy& operator=(float v) { _rec.setFloat(_idx, v); return *this; }
    FieldProxy& operator=(double v) { _rec.setFloat(_idx, (float)v); return *this; }
    FieldProxy& operator=(bool v) { _rec.setBool(_idx, v); return *this; }

    operator String() const { return _rec.getString(_idx); }
    operator const char*() const { return _rec.getCString(_idx); }
    operator int32_t() const { return _rec.getInt(_idx); }
    operator float() const { return _rec.getFloat(_idx); }
    operator double() const { return (double)_rec.getFloat(_idx); }
    operator bool() const { return _rec.getBool(_idx); }

  private:
    NanoRecord &_rec;
    int _idx;
  };

  // access by column name
  FieldProxy operator[](const String &colName);

  // typed getters by index
  int32_t getInt(uint8_t idx) const;
  float   getFloat(uint8_t idx) const;
  bool    getBool(uint8_t idx) const;
  String  getString(uint8_t idx) const;
  const char* getCString(uint8_t idx) const;

  // typed getters by name
  int32_t getInt(const String &colName) const;
  float   getFloat(const String &colName) const;
  bool    getBool(const String &colName) const;
  String  getString(const String &colName) const;
  const char* getCString(const String &colName) const;

  // typed setters by index (used internally)
  void setInt(uint8_t idx, int32_t v);
  void setFloat(uint8_t idx, float v);
  void setBool(uint8_t idx, bool v);
  void setString(uint8_t idx, const String &s);

  // metadata access
  uint8_t columnCount() const { return _colCount; }
  String  columnName(uint8_t idx) const;

  // low-level access (used by NanoTable)
  const ColumnDef* columns() const { return _cols; }
  uint8_t rowColumnCount() const { return _colCount; }
  const uint8_t* rawData() const { return _data; }
  uint8_t* rawData() { return _data; }
  uint16_t rowSize() const { return _rowSize; }

private:
  const ColumnDef* _cols;
  uint8_t _colCount;
  uint16_t _rowSize;
  uint8_t* _data; // raw row buffer

  int colIndexByName(const String &name) const;
  size_t offsetOf(uint8_t idx) const;
};

class NanoTable {
public:
  NanoTable(const String &tableName);

  // FS must be begun by user (LittleFS.begin() / SPIFFS.begin())
  // begin: create or load header
  bool begin(const ColumnDef *cols, uint8_t colCount);

  // drop table file
  bool drop();

  // counts and size
  uint32_t records(); // returns number of non-deleted records (id != 0)
  uint32_t lastId();  // returns max id value used
  size_t size();      // file size in bytes

  // lifecycle for record creation
  // newRecord: attach record to schema but does NOT save
  bool newRecord(NanoRecord &rec);
  // save: append record to file (auto-assign id if id column exists and ==0)
  bool save(NanoRecord &rec);

  // read/update/find/delete
  bool read(int32_t idValue, NanoRecord &outRec);            // fill outRec with record having id==idValue
  bool update(NanoRecord &rec);                              // update record by rec["id"]
  bool find(NanoRecord &outRec, int32_t idValue);            // alias to read
  bool find(NanoRecord &outRec, const String &col, const String &val);
  bool find(NanoRecord &outRec, const String &col, int32_t val);
  bool find(NanoRecord &outRec, const String &col, float val);

  bool findNext(NanoRecord &rec, int32_t id);
  bool findPrevious(NanoRecord &rec, int32_t id);

  // delete record by id (logical delete: id -> 0)
  bool drop(int32_t idValue);

private:
  String _name;
  String _path;

  ColumnDef _cols[NANO_MAX_COLS];
  uint8_t _colCount;
  uint16_t _recordSize; // bytes per record

  bool _exists() const;
  bool _writeHeader(const ColumnDef *cols, uint8_t colCount);
  bool _loadHeader();

  size_t _headerSizeBytes() const;
  uint16_t _typeSize(const ColumnDef &c) const;

  // file record helpers
  bool _readRecordAt(File &f, size_t offset, NanoRecord &outRec);
  bool _writeRecordAt(File &f, size_t offset, const NanoRecord &rec);

  // find offsets
  size_t _findOffsetById(int32_t idValue);
  size_t _findOffsetByColString(const String &col, const String &val);
  size_t _findOffsetByColInt(const String &col, int32_t val);
  size_t _findOffsetByColFloat(const String &col, float val);

  int32_t _nextId();

  // no copy
  NanoTable(const NanoTable&) = delete;
  NanoTable& operator=(const NanoTable&) = delete;
};
