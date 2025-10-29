# NanoDB

**NanoDB** is a lightweight binary database for **ESP32/Arduino**, designed to work with **LittleFS** or **SPIFFS**.  
It provides table-based structured data storage with typed columns, minimal RAM usage, and fast access to records by name or ID.

---

## ⚙️ Features

- Works with **LittleFS** or **SPIFFS**
- Supports **multiple tables**
- **Binary storage** for minimal flash and RAM usage
- **Fixed-length fields** for fast random access
- **Strongly typed columns** (integer, float, string, bool)
- Simple and clear API

---

## 🧩 Column Definition

```cpp
ColumnDef cols[] = {
  {"id",'I',4},
  {"name",'S',20},
  {"age",'I',4},
  {"active",'B',1},
  {"rating",'F',4}
};
```

| Type | Code | Description     | Example       |
|------|------|-----------------|----------------|
| `'I'` | Integer (4 bytes) | 123 |
| `'F'` | Float (4 bytes) | 3.14 |
| `'S'` | String (fixed size) | "John" |
| `'B'` | Boolean (1 byte) | true/false |

---

## 🚀 Basic Usage

### Initialization

```cpp
#include <LittleFS.h>
#include <NanoDB.h>

ColumnDef cols[] = {
  {"id",'I',4},
  {"name",'S',20},
  {"age",'I',4},
  {"active",'B',1},
  {"rating",'F',4}
};

NanoTable users("users");

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  users.begin(cols, 5); // creates the table if it doesn't exist
}
```

---

## 📂 Table Management

```cpp
users.drop();        // Delete the table file
users.records();     // Get number of records in the table
users.lastId();      // returns the highest used ID, even if some records were deleted
users.size();        // Get table size in bytes
```

---

## 🧱 Record Operations

### Create a Record

```cpp
NanoRecord rec;
users.new(rec);       // Assigns table column structure
rec["name"] = "Alice";
rec["age"] = 25;
rec["rating"] = 4.8;
rec["active"] = true;

users.save(rec);      // Automatically assigns next ID
```

### Read / Update

```cpp
NanoRecord rec;
users.read(1, rec);
Serial.println((const char*)rec["name"]); // Print name

rec["name"] = "Updated Name";
users.update(rec);
```

### Find Records

```cpp
NanoRecord rec;

if (users.find(rec, 1)) { /* find by ID */ }
if (users.find(rec, "name", "Alice")) { /* find by field */ }
if (users.find(rec, "age", 30)) { /* find by int */ }

if (users.findNext(rec, lastId)) { /* find next after lastId */ }
if (users.findPrev(rec, lastId)) { /* find next before lastId */ }
```

### Delete

```cpp
users.drop(3); // delete record with ID = 3
rec.detach();  // clear record data
```

---

## 🔍 Reading Data

```cpp
Serial.println(rec["name"]);               // as String
Serial.println((const char*)rec["name"]);  // as const char*
Serial.println((bool)rec["active"]);       // as bool
Serial.println((int)rec["age"]);           // as int
Serial.println((float)rec["rating"]);      // as float
Serial.println((double)rec["rating"]);     // as double
```

---

## 🧠 Design Goals

- **Low memory footprint** (suitable for microcontrollers)
- **Fast random access** without full-table loading
- **Persistent storage** via LittleFS/SPIFFS
- **Clean and minimal API**

---

## 🪶 License

This project is released under the **MIT License**.

---

## 📖 Example Project

See `examples/NanoDB_Demo/NanoDB_Demo.ino` for a complete working example.

---

**NanoDB** — tiny binary database for your ESP32 and Arduino projects.
