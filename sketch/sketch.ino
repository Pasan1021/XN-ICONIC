// Minimal Pulse log-latency test. No Wi-Fi, no Ethernet, no pins.
// Prints "seq=N" exactly once per second, 100 lines per run, 3 runs.

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("boot");
}

void loop() {
  for (int run = 1; run <= 3; run++) {
    Serial.println(String("run_start ") + run);
    unsigned long next = millis();
    for (int n = 1; n <= 100; n++) {
      while ((long)(millis() - next) < 0) delay(1);
      Serial.println(String("seq=") + n);
      next += 1000;
    }
    Serial.println(String("run_done ") + run);
    delay(30000);
  }
  Serial.println("all_runs_done");
  while (true) delay(1000);
}
