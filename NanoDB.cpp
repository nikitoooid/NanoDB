#include "NanoDB.h"

// ---------------- NanoRecord ----------------

NanoRecord::NanoRecord() : _cols(nullptr), _colCount(0), _rowSize(0), _data(nullptr) {}
NanoRecord::~NanoRecord() { detach(); }

void NanoRecord::attach(const ColumnDef *cols, uint8_t colCount, uint16_t rowSize) {
  detach();
  _cols = cols;
  _colCount = colCount;
  _rowSize = rowSize;
  _data = (uint8_t*)malloc(_rowSize);
  if (_data) memset(_data, 0, _rowSize);
}

void NanoRecord::detach() {
  if (_data) {
    free(_data);
    _data = nullptr;
  }
  _cols = nullptr;
  _colCount = 0;
  _rowSize = 0;
}

int NanoRecord::colIndexByName(const String &name) const {
  if (!_cols) return -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name == name) return i;
  return -1;
}

size_t NanoRecord::offsetOf(uint8_t idx) const {
  size_t off = 0;
  for (int i=0;i<idx && i < _colCount; i++) {
    switch (_cols[i].type) {
      case 'I': off += 4; break;
      case 'F': off += 4; break;
      case 'B': off += 1; break;
      case 'S': off += _cols[i].size; break;
      default: break;
    }
  }
  return off;
}

NanoRecord::FieldProxy NanoRecord::operator[](const String &colName) {
  int idx = colIndexByName(colName);
  if (idx < 0) idx = 0;
  return FieldProxy(*this, idx);
}

// getters by index
int32_t NanoRecord::getInt(uint8_t idx) const {
  if (!_data || idx >= _colCount) return 0;
  int32_t v;
  memcpy(&v, _data + offsetOf(idx), 4);
  return v;
}
float NanoRecord::getFloat(uint8_t idx) const {
  if (!_data || idx >= _colCount) return 0.0f;
  float v;
  memcpy(&v, _data + offsetOf(idx), 4);
  return v;
}
bool NanoRecord::getBool(uint8_t idx) const {
  if (!_data || idx >= _colCount) return false;
  uint8_t b = *(_data + offsetOf(idx));
  return b != 0;
}
String NanoRecord::getString(uint8_t idx) const {
  if (!_data || idx >= _colCount) return String();
  size_t len = _cols[idx].size;
  if (len >= NANO_MAX_STR_LEN) len = NANO_MAX_STR_LEN - 1;
  char tmp[NANO_MAX_STR_LEN];
  memset(tmp,0,sizeof(tmp));
  memcpy(tmp, _data + offsetOf(idx), _cols[idx].size);
  return String(tmp);
}
const char* NanoRecord::getCString(uint8_t idx) const {
  if (!_data || idx >= _colCount) return "";
  static thread_local char tmpStatic[NANO_MAX_STR_LEN];
  size_t len = _cols[idx].size;
  if (len >= NANO_MAX_STR_LEN) len = NANO_MAX_STR_LEN - 1;
  memset(tmpStatic,0,sizeof(tmpStatic));
  memcpy(tmpStatic, _data + offsetOf(idx), _cols[idx].size);
  return tmpStatic;
}

// getters by name
int32_t NanoRecord::getInt(const String &colName) const { int i = colIndexByName(colName); return (i>=0) ? getInt(i) : 0; }
float   NanoRecord::getFloat(const String &colName) const { int i = colIndexByName(colName); return (i>=0) ? getFloat(i) : 0.0f; }
bool    NanoRecord::getBool(const String &colName) const { int i = colIndexByName(colName); return (i>=0) ? getBool(i) : false; }
String  NanoRecord::getString(const String &colName) const { int i = colIndexByName(colName); return (i>=0) ? getString(i) : String(); }
const char* NanoRecord::getCString(const String &colName) const { int i = colIndexByName(colName); return (i>=0) ? getCString(i) : ""; }

// setters by index
void NanoRecord::setInt(uint8_t idx, int32_t v) {
  if (!_data || idx >= _colCount) return;
  memcpy(_data + offsetOf(idx), &v, 4);
}
void NanoRecord::setFloat(uint8_t idx, float v) {
  if (!_data || idx >= _colCount) return;
  memcpy(_data + offsetOf(idx), &v, 4);
}
void NanoRecord::setBool(uint8_t idx, bool v) {
  if (!_data || idx >= _colCount) return;
  _data[offsetOf(idx)] = v ? 1 : 0;
}
void NanoRecord::setString(uint8_t idx, const String &s) {
  if (!_data || idx >= _colCount) return;
  size_t maxlen = _cols[idx].size;
  size_t copylen = min((size_t)maxlen, (size_t)s.length());
  // zero pad full field
  memset(_data + offsetOf(idx), 0, maxlen);
  if (copylen) memcpy(_data + offsetOf(idx), s.c_str(), copylen);
}

// ---------------- NanoTable ----------------

NanoTable::NanoTable(const String &tableName) {
  _name = tableName;
  _path = "/" + tableName + ".tbl";
  _colCount = 0;
  _recordSize = 0;
}

bool NanoTable::_exists() const {
  return NANOFS.exists(_path);
}

uint16_t NanoTable::_typeSize(const ColumnDef &c) const {
  switch (c.type) {
    case 'I': return 4;
    case 'F': return 4;
    case 'B': return 1;
    case 'S': return c.size;
    default: return 0;
  }
}

size_t NanoTable::_headerSizeBytes() const {
  // 1 byte: colCount
  // for each column: 1 byte nameLen + name +1 byte type +2 bytes size
  size_t s = 1;
  for (int i=0;i<_colCount;i++) {
    s += 1;
    s += _cols[i].name.length();
    s += 1;
    s += 2;
  }
  return s;
}

bool NanoTable::_writeHeader(const ColumnDef *cols, uint8_t colCount) {
  File f = NANOFS.open(_path, "w");
  if (!f) return false;
  uint8_t cc = colCount;
  f.write(&cc,1);
  for (int i=0;i<colCount;i++) {
    String nm = cols[i].name;
    uint8_t nl = (uint8_t)min((size_t)255, nm.length());
    f.write(&nl,1);
    f.write((const uint8_t*)nm.c_str(), nl);
    f.write((uint8_t*)&cols[i].type,1);
    uint16_t s = cols[i].size;
    f.write((const uint8_t*)&s,2);
  }
  f.close();
  // load header into memory
  return _loadHeader();
}

bool NanoTable::_loadHeader() {
  File f = NANOFS.open(_path,"r");
  if (!f) return false;
  uint8_t cc;
  if (f.read(&cc,1) != 1) { f.close(); return false; }
  _colCount = cc;
  for (int i=0;i<_colCount;i++) {
    uint8_t nl;
    f.read(&nl,1);
    char nb[256];
    if (nl > 0) f.read((uint8_t*)nb, nl);
    nb[nl]=0;
    char t; f.read((uint8_t*)&t,1);
    uint16_t s; f.read((uint8_t*)&s,2);
    _cols[i].name = String(nb);
    _cols[i].type = t;
    _cols[i].size = s;
  }
  _recordSize = 0;
  for (int i=0;i<_colCount;i++) _recordSize += _typeSize(_cols[i]);
  f.close();
  return true;
}

bool NanoTable::begin(const ColumnDef *cols, uint8_t colCount) {
  // FS must be already begun by user
  if (_exists()) {
    return _loadHeader();
  } else {
    if (!cols || colCount == 0) return false;
    return _writeHeader(cols, colCount);
  }
}

bool NanoTable::drop() {
  if (_exists()) return NANOFS.remove(_path);
  return true;
}

uint32_t NanoTable::records() {
  if (!_exists()) return 0;
  if (_colCount==0) _loadHeader();
  File f = NANOFS.open(_path,"r");
  if (!f) return 0;
  size_t hs = _headerSizeBytes();
  size_t pos = hs;
  uint32_t cnt = 0;
  while (pos + _recordSize <= f.size()) {
    // check id != 0 if id exists
    int idIdx = -1;
    for (int i=0;i<_colCount;i++) if (_cols[i].name=="id" && _cols[i].type=='I') { idIdx=i; break; }
    if (idIdx < 0) { // no id column -> count all
      cnt++;
    } else {
      size_t idOff = pos + 0;
      for (int j=0;j<idIdx;j++) idOff += _typeSize(_cols[j]);
      int32_t v; f.seek(idOff);
      if (f.read((uint8_t*)&v,4)==4 && v != 0) cnt++;
    }
    pos += _recordSize;
  }
  f.close();
  return cnt;
}

size_t NanoTable::size() {
  if (!_exists()) return 0;
  File f = NANOFS.open(_path,"r");
  if (!f) return 0;
  size_t s = f.size();
  f.close();
  return s;
}

bool NanoTable::newRecord(NanoRecord &rec) {
  if (_colCount==0 && !_loadHeader()) return false;
  rec.attach(_cols, _colCount, _recordSize);
  return true;
}

int32_t NanoTable::_nextId() {
  if (_colCount==0) _loadHeader();
  int idIdx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name=="id" && _cols[i].type=='I') { idIdx=i; break; }
  if (idIdx < 0) return 1;
  File f = NANOFS.open(_path,"r");
  if (!f) return 1;
  size_t pos = _headerSizeBytes();
  int32_t maxId = 0;
  while (pos + _recordSize <= f.size()) {
    size_t idOff = pos;
    for (int j=0;j<idIdx;j++) idOff += _typeSize(_cols[j]);
    int32_t v;
    f.seek(idOff);
    if (f.read((uint8_t*)&v,4) != 4) break;
    if (v > maxId) maxId = v;
    pos += _recordSize;
  }
  f.close();
  return maxId + 1;
}

bool NanoTable::_writeRecordAt(File &f, size_t offset, const NanoRecord &rec) {
  if (!f) return false;
  if (!rec.columns()) return false;
  if (offset + _recordSize > (size_t)f.size() && f.position() != offset) {
    // ensure seek
    f.seek(offset);
  } else {
    f.seek(offset);
  }
  // write each field
  size_t base = f.position();
  for (int i=0;i<_colCount;i++) {
    switch (_cols[i].type) {
      case 'I': {
        int32_t v = rec.getInt(i);
        f.write((const uint8_t*)&v, 4);
        break;
      }
      case 'F': {
        float v = rec.getFloat(i);
        f.write((const uint8_t*)&v, 4);
        break;
      }
      case 'B': {
        uint8_t b = rec.getBool(i) ? 1 : 0;
        f.write(&b, 1);
        break;
      }
      case 'S': {
        const char* s = rec.getCString(i);
        size_t len = strlen(s);
        size_t toWrite = min((size_t)_cols[i].size, len);
        if (toWrite) f.write((const uint8_t*)s, toWrite);
        if (_cols[i].size > toWrite) {
          uint8_t zero = 0;
          for (size_t z=0; z < (_cols[i].size - toWrite); z++) f.write(&zero,1);
        }
        break;
      }
      default: break;
    }
  }
  // ensure total bytes written == recordSize (no-op)
  return true;
}

bool NanoTable::save(NanoRecord &rec) {
  if (_colCount==0 && !_loadHeader()) return false;
  if (!rec.columns()) rec.attach(_cols, _colCount, _recordSize);
  // set id if exists and zero
  int idIdx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name=="id" && _cols[i].type=='I') { idIdx=i; break; }
  if (idIdx >= 0) {
    int32_t cur = rec.getInt(idIdx);
    if (cur == 0) rec.setInt(idIdx, _nextId());
  }
  File f = NANOFS.open(_path,"a");
  if (!f) return false;
  bool ok = _writeRecordAt(f, f.size(), rec);
  f.close();
  return ok;
}

bool NanoTable::_readRecordAt(File &f, size_t offset, NanoRecord &outRec) {
  if (_colCount==0 && !_loadHeader()) return false;
  if (!outRec.columns()) outRec.attach(_cols, _colCount, _recordSize);
  if (offset + _recordSize > f.size()) return false;
  f.seek(offset);
  // read each field
  for (int i=0;i<_colCount;i++) {
    switch (_cols[i].type) {
      case 'I': {
        int32_t v; if (f.read((uint8_t*)&v,4) != 4) return false;
        outRec.setInt(i, v); break;
      }
      case 'F': {
        float v; if (f.read((uint8_t*)&v,4) != 4) return false;
        outRec.setFloat(i, v); break;
      }
      case 'B': {
        uint8_t b; if (f.read((uint8_t*)&b,1) != 1) return false;
        outRec.setBool(i, b != 0); break;
      }
      case 'S': {
        uint16_t len = _cols[i].size;
        char buf[NANO_MAX_STR_LEN];
        memset(buf,0,sizeof(buf));
        if (len > 0) {
          size_t toRead = len;
          if (toRead > sizeof(buf)-1) toRead = sizeof(buf)-1;
          if (f.read((uint8_t*)buf, len) != len) return false;
        }
        outRec.setString(i, String(buf));
        break;
      }
      default: return false;
    }
  }
  return true;
}

size_t NanoTable::_findOffsetById(int32_t idValue) {
  if (_colCount==0) _loadHeader();
  int idIdx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name=="id" && _cols[i].type=='I') { idIdx=i; break; }
  if (idIdx < 0) return SIZE_MAX;
  File f = NANOFS.open(_path,"r");
  if (!f) return SIZE_MAX;
  size_t pos = _headerSizeBytes();
  while (pos + _recordSize <= f.size()) {
    size_t idOff = pos;
    for (int j=0;j<idIdx;j++) idOff += _typeSize(_cols[j]);
    int32_t v;
    f.seek(idOff);
    if (f.read((uint8_t*)&v,4) != 4) break;
    if (v == idValue) { f.close(); return pos; }
    pos += _recordSize;
  }
  f.close();
  return SIZE_MAX;
}

size_t NanoTable::_findOffsetByColString(const String &col, const String &val) {
  if (_colCount==0) _loadHeader();
  int idx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name==col) { idx=i; break; }
  if (idx < 0) return SIZE_MAX;
  if (_cols[idx].type != 'S') return SIZE_MAX;
  File f = NANOFS.open(_path,"r");
  if (!f) return SIZE_MAX;
  size_t pos = _headerSizeBytes();
  while (pos + _recordSize <= f.size()) {
    size_t fieldOff = pos;
    for (int j=0;j<idx;j++) fieldOff += _typeSize(_cols[j]);
    char buf[NANO_MAX_STR_LEN];
    memset(buf,0,sizeof(buf));
    f.seek(fieldOff);
    if (f.read((uint8_t*)buf, _cols[idx].size) != _cols[idx].size) break;
    if (String(buf) == val) { f.close(); return pos; }
    pos += _recordSize;
  }
  f.close();
  return SIZE_MAX;
}

size_t NanoTable::_findOffsetByColInt(const String &col, int32_t val) {
  if (_colCount==0) _loadHeader();
  int idx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name==col) { idx=i; break; }
  if (idx < 0) return SIZE_MAX;
  if (_cols[idx].type != 'I') return SIZE_MAX;
  File f = NANOFS.open(_path,"r");
  if (!f) return SIZE_MAX;
  size_t pos = _headerSizeBytes();
  while (pos + _recordSize <= f.size()) {
    size_t fieldOff = pos;
    for (int j=0;j<idx;j++) fieldOff += _typeSize(_cols[j]);
    int32_t v;
    f.seek(fieldOff);
    if (f.read((uint8_t*)&v,4) != 4) break;
    if (v == val) { f.close(); return pos; }
    pos += _recordSize;
  }
  f.close();
  return SIZE_MAX;
}

size_t NanoTable::_findOffsetByColFloat(const String &col, float val) {
  if (_colCount==0) _loadHeader();
  int idx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name==col) { idx=i; break; }
  if (idx < 0) return SIZE_MAX;
  if (_cols[idx].type != 'F') return SIZE_MAX;
  File f = NANOFS.open(_path,"r");
  if (!f) return SIZE_MAX;
  size_t pos = _headerSizeBytes();
  while (pos + _recordSize <= f.size()) {
    size_t fieldOff = pos;
    for (int j=0;j<idx;j++) fieldOff += _typeSize(_cols[j]);
    float v;
    f.seek(fieldOff);
    if (f.read((uint8_t*)&v,4) != 4) break;
    if (fabs(v - val) < 1e-6f) { f.close(); return pos; }
    pos += _recordSize;
  }
  f.close();
  return SIZE_MAX;
}

bool NanoTable::read(int32_t idValue, NanoRecord &outRec) {
  size_t off = _findOffsetById(idValue);
  if (off == SIZE_MAX) return false;
  File f = NANOFS.open(_path,"r");
  if (!f) return false;
  bool ok = _readRecordAt(f, off, outRec);
  f.close();
  return ok;
}

bool NanoTable::update(NanoRecord &rec) {
  if (_colCount==0 && !_loadHeader()) return false;
  // require id
  int idIdx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name=="id" && _cols[i].type=='I') { idIdx=i; break; }
  if (idIdx < 0) return false;
  int32_t idv = rec.getInt(idIdx);
  if (idv == 0) return false;
  size_t off = _findOffsetById(idv);
  if (off == SIZE_MAX) return false;
  File f = NANOFS.open(_path,"r+");
  if (!f) return false;
  bool ok = _writeRecordAt(f, off, rec);
  f.close();
  return ok;
}

bool NanoTable::find(NanoRecord &outRec, int32_t idValue) { return read(idValue, outRec); }
bool NanoTable::find(NanoRecord &outRec, const String &col, const String &val) {
  size_t off = _findOffsetByColString(col, val);
  if (off == SIZE_MAX) return false;
  File f = NANOFS.open(_path,"r");
  if (!f) return false;
  bool ok = _readRecordAt(f, off, outRec);
  f.close();
  return ok;
}
bool NanoTable::find(NanoRecord &outRec, const String &col, int32_t val) {
  size_t off = _findOffsetByColInt(col, val);
  if (off == SIZE_MAX) return false;
  File f = NANOFS.open(_path,"r");
  if (!f) return false;
  bool ok = _readRecordAt(f, off, outRec);
  f.close();
  return ok;
}
bool NanoTable::find(NanoRecord &outRec, const String &col, float val) {
  size_t off = _findOffsetByColFloat(col, val);
  if (off == SIZE_MAX) return false;
  File f = NANOFS.open(_path,"r");
  if (!f) return false;
  bool ok = _readRecordAt(f, off, outRec);
  f.close();
  return ok;
}

bool NanoTable::drop(int32_t idValue) {
  // logical delete: set id to zero
  size_t off = _findOffsetById(idValue);
  if (off == SIZE_MAX) return false;
  // find id offset within record
  int idIdx = -1;
  for (int i=0;i<_colCount;i++) if (_cols[i].name=="id" && _cols[i].type=='I') { idIdx=i; break; }
  if (idIdx < 0) return false;
  File f = NANOFS.open(_path,"r+");
  if (!f) return false;
  size_t idOff = off;
  for (int j=0;j<idIdx;j++) idOff += _typeSize(_cols[j]);
  f.seek(idOff);
  int32_t zero = 0;
  f.write((const uint8_t*)&zero,4);
  f.close();
  return true;
}
