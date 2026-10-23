#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// NORVI-XN Ethernet pins, per system-diagnostics.md.
static constexpr uint8_t ETH_CS = 1;
static constexpr uint8_t DISPLAY_CS = 45;
static constexpr uint8_t SD_CARD_CS = 42;
static constexpr uint8_t PORT_RST = 38;
static constexpr uint8_t SPI_MISO = 13;
static constexpr uint8_t SPI_MOSI = 11;
static constexpr uint8_t SPI_SCLK = 12;

static constexpr char NTP_SERVER[] = "time.nist.gov";
static constexpr uint16_t NTP_LOCAL_PORT = 8888;
static constexpr uint8_t NTP_PACKET_SIZE = 48;
static constexpr uint32_t NTP_UNIX_OFFSET = 2208988800UL;
static constexpr uint8_t LINE_COUNT = 100;
static constexpr uint32_t LINE_INTERVAL_MS = 1000;

byte macAddress[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ntpPacket[NTP_PACKET_SIZE];
EthernetUDP ntpUdp;

uint64_t ntpEpochBaseMs = 0;
uint32_t ntpMonotonicBaseMs = 0;

static uint32_t readNtpWord(const byte *bytes) {
  return (static_cast<uint32_t>(bytes[0]) << 24)
       | (static_cast<uint32_t>(bytes[1]) << 16)
       | (static_cast<uint32_t>(bytes[2]) << 8)
       | static_cast<uint32_t>(bytes[3]);
}

static bool syncNtpClock() {
  SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);
  Ethernet.init(ETH_CS);
  if (Ethernet.begin(macAddress) == 0) return false;
  if (ntpUdp.begin(NTP_LOCAL_PORT) == 0) return false;

  memset(ntpPacket, 0, sizeof(ntpPacket));
  ntpPacket[0] = 0b11100011; // NTP client request, version 4.
  ntpPacket[2] = 6;
  ntpPacket[3] = 0xEC;
  ntpPacket[12] = 49;
  ntpPacket[13] = 0x4E;
  ntpPacket[14] = 49;
  ntpPacket[15] = 52;

  if (ntpUdp.beginPacket(NTP_SERVER, 123) != 1) return false;
  if (ntpUdp.write(ntpPacket, sizeof(ntpPacket)) != sizeof(ntpPacket)) return false;
  if (ntpUdp.endPacket() != 1) return false;

  const uint32_t waitStarted = millis();
  while (millis() - waitStarted < 5000) {
    const int packetSize = ntpUdp.parsePacket();
    if (packetSize >= NTP_PACKET_SIZE) {
      if (ntpUdp.read(ntpPacket, sizeof(ntpPacket)) != sizeof(ntpPacket)) return false;

      // NTP transmit timestamp is bytes 40..47 (seconds, then fraction).
      const uint32_t ntpSeconds = readNtpWord(&ntpPacket[40]);
      const uint32_t ntpFraction = readNtpWord(&ntpPacket[44]);
      if (ntpSeconds < NTP_UNIX_OFFSET) return false;
      const uint64_t unixSeconds = ntpSeconds - NTP_UNIX_OFFSET;
      const uint16_t fractionMs = static_cast<uint16_t>((static_cast<uint64_t>(ntpFraction) * 1000ULL) >> 32);

      ntpEpochBaseMs = unixSeconds * 1000ULL + fractionMs;
      ntpMonotonicBaseMs = millis();
      return true;
    }
    delay(10);
  }
  return false;
}

static uint64_t currentEpochMs() {
  return ntpEpochBaseMs + static_cast<uint32_t>(millis() - ntpMonotonicBaseMs);
}

static void printTimestampedLine(uint8_t sequence) {
  const uint64_t epochMs = currentEpochMs();
  const uint64_t epochSeconds = epochMs / 1000ULL;
  const uint16_t milliseconds = static_cast<uint16_t>(epochMs % 1000ULL);

  Serial.print("seq=");
  Serial.print(sequence);
  Serial.print(" t=");
  Serial.print(static_cast<unsigned long>(epochSeconds));
  Serial.print('.');
  if (milliseconds < 100) Serial.print('0');
  if (milliseconds < 10) Serial.print('0');
  Serial.println(milliseconds);
}

void setup() {
  Serial.begin(115200);

  // Same start-up step as the working diagnostics example: release the
  // I/O expander reset line before using the other peripherals.
  pinMode(PORT_RST, OUTPUT);
  digitalWrite(PORT_RST, HIGH);
  delay(100);

  // Deselect the other devices on the shared SPI bus before using Ethernet.
  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);
  pinMode(DISPLAY_CS, OUTPUT);
  digitalWrite(DISPLAY_CS, HIGH);
  pinMode(SD_CARD_CS, OUTPUT);
  digitalWrite(SD_CARD_CS, HIGH);

  if (!syncNtpClock()) {
    Serial.println("ntp_failed");
    while (true) delay(1000);
  }

  uint32_t nextLineAt = millis();
  for (uint8_t sequence = 1; sequence <= LINE_COUNT; ++sequence) {
    while (static_cast<int32_t>(millis() - nextLineAt) < 0) delay(1);
    printTimestampedLine(sequence);
    nextLineAt += LINE_INTERVAL_MS;
  }
}

void loop() {
  // Emit exactly 100 test lines, then remain quiet.
  delay(1000);
}
