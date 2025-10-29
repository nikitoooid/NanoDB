#include <Arduino.h>
#include <LittleFS.h>
#include <NanoDB.h>

// ---------------------- Table Definition ----------------------
ColumnDef userCols[] = {
  {"id", 'I', 4},
  {"name", 'S', 20},
  {"age", 'I', 4},
  {"active", 'B', 1},
  {"rating", 'F', 4}
};

NanoTable users("users");

// ---------------------- Setup ----------------------
void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  // LittleFS.format();

  // Initialize table
  if (!users.begin(userCols, 5)) {
    Serial.println("❌ Failed to initialize table");
    return;
  }

  Serial.println("✅ Table initialized");

  // ---------------------- Create Records ----------------------
  NanoRecord rec;

  users.newRecord(rec);
  rec["name"] = "Ivan";
  rec["age"] = 25;
  rec["active"] = true;
  rec["rating"] = 4.9f;
  users.save(rec);

  users.newRecord(rec);
  rec["name"] = "Oleh";
  rec["age"] = 30;
  rec["active"] = false;
  rec["rating"] = 3.5f;
  users.save(rec);

  users.newRecord(rec);
  rec["name"] = "Anna";
  rec["age"] = 22;
  rec["active"] = true;
  rec["rating"] = 4.7f;
  users.save(rec);

  Serial.printf("📄 Records: %lu\n", users.records());
  Serial.printf("🔢 Last ID: %lu\n", users.lastId());

  // ---------------------- Read & Display ----------------------
  NanoRecord r;
  if (users.read(2, r)) {
    Serial.println("👉 Record #2:");
    Serial.println(String("  Name: ") + String((const char*)r["name"]));
    Serial.println("  Age: " + String((int)r["age"]));
    Serial.println("  Rating: " + String((float)r["rating"]));
  }

  // ---------------------- findNext / findPrevious ----------------------
  NanoRecord next;
  if (users.findNext(next, 2)) {
    Serial.println("\n➡️ Next after #2:");
    Serial.println("  ID: " + String((int)next["id"]));
    Serial.println(String("  Name: ") + String((const char*)next["name"]));
  }

  NanoRecord prev;
  if (users.findPrevious(prev, 2)) {
    Serial.println("\n⬅️ Previous before #2:");
    Serial.println("  ID: " + String((int)prev["id"]));
    Serial.println(String("  Name: ") + String((const char*)prev["name"]));
  }

  // ---------------------- Drop a Record ----------------------
  users.drop(2);
  Serial.println("\n🗑️ Record #2 deleted");

  Serial.printf("📄 Records after delete: %lu\n", users.records());
  Serial.printf("🔢 Last ID (unchanged): %lu\n", users.lastId());

  // ---------------------- findNext after delete ----------------------
  if (users.findNext(next, 1)) {
    Serial.println("\n➡️ Next after #1:");
    Serial.println("  ID: " + String((int)next["id"]));
    Serial.println(String("  Name: ") + String((const char*)next["name"]));
  } else {
    Serial.println("No next record after #1");
  }

  // ---------------------- Drop a Record ----------------------
  users.drop(2);
  Serial.println("\n🗑️ Record #2 deleted");

  Serial.printf("📄 Records after delete: %lu\n", users.records());
  Serial.printf("🔢 Last ID (unchanged): %lu\n", users.lastId());

  // ---------------------- Drop a Table ----------------------
  if (!users.drop()) {
    Serial.println("❌ Failed to drop a table");
    LittleFS.format();
    return;
  }

  Serial.println("✅ Table dropped");
}

void loop() {}
