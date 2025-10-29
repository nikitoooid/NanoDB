#include <Arduino.h>
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

void test(uint32_t records = 100) {
  Serial.println("------- Testing NanoDB -------");
  Serial.println("creating record...");
  NanoRecord rec;
  users.newRecord(rec);          // прив'язує rec до таблиці (але не зберігає)

  Serial.println("setting record params...");
  rec["name"] = "Ivan";
  rec["age"] = 30;
  rec["rating"] = 4.5;
  rec["active"] = true;

  Serial.println("reading record params...");
  Serial.println((const char*)rec["name"]);
  Serial.println((int)rec["age"]);
  Serial.println((float)rec["age"]);
  Serial.println((bool)rec["active"] ? "true" : "false");

  Serial.println(String("records in the table: ") + users.records());
  Serial.println("saving record...");
  users.save(rec);               // додає запис (id автоматично присвоюється)
  Serial.println(String("records in the table: ") + users.records());

  Serial.println("finding record by name...");
  NanoRecord got;
  if (users.find(got, "name", "Ivan")) {
    Serial.println((const char*)got["name"]);
    Serial.println((int)got["age"]);
  } else Serial.println("record not found");

  Serial.println("finding record by id...");
  if (users.find(got, 1)) {
    Serial.println((const char*)got["name"]);
    Serial.println((int)got["age"]);
  } else Serial.println("record not found");

  Serial.println("updating record...");
  got["rating"] = 4.8;
  users.update(got);

  Serial.println("finding updated record by id...");
  if (users.find(got, 1)) {
    Serial.println((const char*)got["name"]);
    Serial.println((float)got["rating"]);
  } else Serial.println("record not found");

  Serial.println("deleting record by id...");
  Serial.println(String("records in the table: ") + users.records());
  users.drop(1);
  Serial.println(String("records in the table: ") + users.records());

  Serial.println("deleting record by record...");
  Serial.println(" > creating record...");
  users.save(rec);
  Serial.println(" > trying to get record...");
  if (users.find(got, 1)) {
    Serial.println(" > deleting record...");
    Serial.println(String("records in the table: ") + users.records());
    users.drop(1);
    Serial.println(String("records in the table: ") + users.records());
  } else Serial.println(" > record not found");
  

  Serial.println("------- Test complete -------");
  Serial.println();
  Serial.println("------- Stress testing ------");

  uint32_t start = millis();
  Serial.println(String("adding ") + records + " records...");

  for (uint32_t i=0;i<records;i++) {
    NanoRecord r;
    users.newRecord(r);
    r["name"] = "User" + String(i);
    r["age"] = 20 + (i % 30);
    r["rating"] = (float)(i % 5) + 0.5;
    r["active"] = (i % 2) == 0;
    users.save(r);
  }

  Serial.println(String("added ") + records + " records in " + (millis() - start) + " ms");
  Serial.println(String("total records now: ") + users.records());

  Serial.println("searching records...");
  if (recods > 5) {
    Serial.println("searching for record id=5...");
    start = millis();
    NanoRecord r;
    users.find(r, 5);
    Serial.println(String("found record id=5: name=") + (const char*)r["name"] + String(", age=") + (int)r["age"] + " in " + (millis() - start) + " ms");
  }

  if (recods > 10) {
    Serial.println(String("searching for record id=") + (records - 1) + "...");
    start = millis();
    NanoRecord r;
    users.find(r, records - 1);
    Serial.println(String("found record id=") + (records - 1) + ": name=" + (const char*)r["name"] + String(", age=") + (int)r["age"] + " in " + (millis() - start) + " ms");
  }

  Serial.println("deleting records...");
  if (recods > 5) {
    Serial.println("deleting record with id=5...");
    start = millis();
    users.drop(5);
    Serial.println(String("deleted record with id=5 in ") + (millis() - start) + " ms");
  }

  if (recods > 10) {
    Serial.println(String("deleting record with id=") + (records - 1) + "...");
    start = millis();
    users.drop(records - 1);
    Serial.println(String("deleted record with id=") + (records - 1) + " in " + (millis() - start) + " ms");
  }

  Serial.println("------- Stress test complete -------");
}

void setup(){
  Serial.begin(115200);
  delay(1000);
  LittleFS.begin(true);
  users.begin(cols, 5);          // завантажує/створює таблицю

  NanoRecord rec;
  users.newRecord(rec);          // прив'язує rec до таблиці (але не зберігає)
  rec["name"] = "Ivan";
  rec["age"] = 30;
  rec["rating"] = 4.5;
  rec["active"] = true;

  users.save(rec);               // додає запис (id автоматично присвоюється)

  NanoRecord got;
  if (users.find(got, "name", "Ivan")) {
    Serial.println((const char*)got["name"]);
    Serial.println((int32_t)got["age"]);
  }

  test(1000);                    // проводить стрес-тест з 1000 записів (~110 секунд на ESP32)
}

void loop(){

}
