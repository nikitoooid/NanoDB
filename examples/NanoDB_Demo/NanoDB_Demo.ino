#include <Arduino.h>
#include <LittleFS.h>
#include <NanoDB.h>

// === Define table structure ===
ColumnDef userCols[] = {
  {"id",     'I', 4},
  {"name",   'S', 20},
  {"age",    'I', 4},
  {"active", 'B', 1},
  {"rating", 'F', 4}
};

NanoTable users("users");

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== NanoDB Demo ===");

  // Initialize filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to initialize LittleFS");
    return;
  }

  // Initialize or create table
  if (!users.begin(userCols, 5)) {
    Serial.println("Failed to initialize users table");
    return;
  }

  // === Create new record ===
  NanoRecord rec;
  users.new(rec);
  rec["name"] = "Ivan";
  rec["age"] = 28;
  rec["active"] = true;
  rec["rating"] = 4.7;
  users.save(rec);

  Serial.println("Record saved:");
  Serial.println((String)"ID: " + (int)rec["id"]);
  Serial.println((String)"Name: " + (const char*)rec["name"]);
  Serial.println((String)"Age: " + (int)rec["age"]);
  Serial.println((String)"Active: " + (bool)rec["active"]);
  Serial.println((String)"Rating: " + (float)rec["rating"]);

  // === Read record by ID ===
  NanoRecord rec2;
  if (users.read((int)rec["id"], rec2)) {
    Serial.println("\nRecord read from DB:");
    Serial.println((String)"Name: " + (const char*)rec2["name"]);
    Serial.println((String)"Age: " + (int)rec2["age"]);
  }

  // === Update record ===
  rec2["rating"] = 4.9;
  rec2["name"] = "Ivan Updated";
  users.update(rec2);

  Serial.println("\nAfter update:");
  Serial.println((String)"Name: " + (const char*)rec2["name"]);
  Serial.println((String)"Rating: " + (float)rec2["rating"]);

  // === Find record by name ===
  NanoRecord found;
  if (users.find(found, "name", "Ivan Updated")) {
    Serial.println("\nFound record:");
    Serial.println((String)"ID: " + (int)found["id"]);
    Serial.println((String)"Age: " + (int)found["age"]);
  }

  // === Show stats ===
  Serial.println("\n=== Table Info ===");
  Serial.println((String)"Records: " + users.records());
  Serial.println((String)"Size: " + users.size() + " bytes");

  // === Delete record ===
  users.drop((int)found["id"]);
  Serial.println("\nRecord deleted.");

  // === Drop table ===
  users.drop();
  Serial.println("Table dropped.");
}

void loop() {
}
