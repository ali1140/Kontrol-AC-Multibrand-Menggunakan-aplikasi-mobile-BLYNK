// --- KONFIGURASI BLYNK (WAJIB PALING ATAS) ---
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Remote AC kosan"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"
#define BLYNK_PRINT Serial

// --- LIBRARY ---
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <WiFiManager.h>

// --- WIFI/BLYNK ---
char auth[] = BLYNK_AUTH_TOKEN;

// --- PIN IR ---
const uint8_t IR_TX_PIN = 16;   

// --- Virtual Pin ---
#define VP_POWER        V0
#define VP_TEMP_UP      V1
#define VP_TEMP_DOWN    V2
#define VP_MODE_TOGGLE  V3
#define VP_LCD          V4
#define VP_TEMPDSP      V5
#define VP_MERK_AC      V6
#define VP_BTN_NEXT     V7
#define VP_BTN_PREV     V8
// VP_PING (V9) sudah dihapus untuk menghemat komunikasi Blynk

// --- Batas suhu ---
#define TEMP_MIN 17
#define TEMP_MAX 30

// --- MODE UJI CEPAT ---
// false = mode normal
// true  = mode stress test / uji cepat
const bool FAST_TEST_MODE = false;

// --- IRac state ---
IRac ac(IR_TX_PIN);
stdAc::state_t s, prev;

// ====== Vendor List pakai decode_type_t ======
static const decode_type_t VENDORS[] = {
  COOLIX,         // MIDEA (Coolix)
  LG,
  SHARP_AC,
  DAIKIN,
  PANASONIC_AC,
  HAIER_AC_YRW02, // AQUA (Menggunakan Protokol Haier) - INDEX 5
  SANYO_AC        // AQUA (Menggunakan Protokol Sanyo)
};

static const char* VENDOR_NAMES[] = {
  "Midea",
  "LG",
  "Sharp",
  "Daikin",
  "Panasonic",
  "Aqua (Haier)",
  "Aqua (Sanyo)"
};

const int VENDOR_COUNT = sizeof(VENDORS) / sizeof(VENDORS[0]);

// DIUBAH: Default awal diatur ke index 5 (HAIER_AC_YRW02 / AQUA)
int  vendor_idx = 5;
decode_type_t current_vendor = HAIER_AC_YRW02;

// ====== Variabel monitor loop ======
unsigned long lastLoop = 0;

// ---------- UTIL IR / BLYNK ----------

const char* modeToString(stdAc::opmode_t m) {
  return (m == stdAc::opmode_t::kAuto) ? "Auto" : "Cool";
}

static inline void resetMomentaryButtons() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VP_TEMP_UP, 0);
  Blynk.virtualWrite(VP_TEMP_DOWN, 0);
  Blynk.virtualWrite(VP_MODE_TOGGLE, 0);
  Blynk.virtualWrite(VP_BTN_NEXT, 0);
  Blynk.virtualWrite(VP_BTN_PREV, 0);
}

static inline void normalizeTwoModes(stdAc::state_t* st) {
  st->quiet  = false;
  st->turbo  = false;
  st->econo  = false;
  st->light  = false;
  st->filter = false;
  st->clean  = false;
  st->beep   = true;

  // Hanya pakai Auto & Cool
  if (st->mode != stdAc::opmode_t::kAuto &&
      st->mode != stdAc::opmode_t::kCool) {
    st->mode = stdAc::opmode_t::kCool;
  }

  // Clamp suhu
  if ((int)st->degrees < TEMP_MIN) st->degrees = TEMP_MIN;
  if ((int)st->degrees > TEMP_MAX) st->degrees = TEMP_MAX;

  if (st->mode == stdAc::opmode_t::kAuto) {
    st->fanspeed = stdAc::fanspeed_t::kAuto;
  } else {
    if (!(st->fanspeed == stdAc::fanspeed_t::kAuto ||
          st->fanspeed == stdAc::fanspeed_t::kMin  ||
          st->fanspeed == stdAc::fanspeed_t::kMedium ||
          st->fanspeed == stdAc::fanspeed_t::kMax)) {
      st->fanspeed = stdAc::fanspeed_t::kAuto;
    }
  }
}

// update LCD Blynk
void updateBlynkDisplay() {
  if (!Blynk.connected()) return;

  char lcdText[64];
  
  // Tampilan LCD hanya mode, tanpa status ping
  snprintf(lcdText, sizeof(lcdText),
           "Mode: %s",
           modeToString(s.mode));

  Blynk.virtualWrite(VP_LCD, lcdText);
  Blynk.virtualWrite(VP_TEMPDSP, (int)s.degrees);
  Blynk.virtualWrite(VP_MERK_AC, VENDOR_NAMES[vendor_idx]);
}

void applyVendor(decode_type_t v) {
  bool              pwr = s.power;
  stdAc::opmode_t   md  = s.mode;
  int               deg = (int)s.degrees;
  stdAc::fanspeed_t fan = s.fanspeed;

  IRac::initState(&s,
                  v,
                  -1,
                  pwr,
                  md,
                  deg,
                  true,
                  fan,
                  s.swingv,
                  s.swingh,
                  false, false, false, false,
                  false, false, true,
                  -1, -1);

  prev           = s;
  current_vendor = v;
  normalizeTwoModes(&s);
  updateBlynkDisplay();

  Serial.printf("[IR] Vendor set: %s\n", VENDOR_NAMES[vendor_idx]);
}

void sendAcOnce(const stdAc::state_t& st) {
  ac.sendAc(st, &prev);
  prev = st;
}

void sendAcCommand() {
  normalizeTwoModes(&s);

  Serial.print("[IR] Send | Vend="); Serial.print(VENDOR_NAMES[vendor_idx]);
  Serial.print(" Power=");  Serial.print(s.power ? "ON" : "OFF");
  Serial.print(" Mode=");   Serial.print(modeToString(s.mode));
  Serial.print(" Temp=");   Serial.print((int)s.degrees);
  Serial.print(" Fan=");    Serial.println((int)s.fanspeed);

  sendAcOnce(s);
  updateBlynkDisplay();
}

void printCurrentStatus(const char* source) {
  Serial.print("[STATUS] ");
  Serial.print(source);
  Serial.print(" -> Power=");
  Serial.print(s.power ? "ON" : "OFF");
  Serial.print(", Mode=");
  Serial.print(modeToString(s.mode));
  Serial.print(", Temp=");
  Serial.print((int)s.degrees);
  Serial.print("C, Merk=");
  Serial.println(VENDOR_NAMES[vendor_idx]);
}

// ---------- BLYNK HANDLERS ----------

BLYNK_WRITE(VP_POWER) {
  s.power = (param.asInt() == 1);
  printCurrentStatus("POWER");
  sendAcCommand();
}

// SUHU BISA DIUBAH DI AUTO & COOL
BLYNK_WRITE(VP_TEMP_UP) {
  int v = param.asInt();
  Serial.printf("[BTN] TEMP_UP: %d\n", v);

  if (v == 1 && s.power) {
    if ((int)s.degrees < TEMP_MAX) {
      s.degrees = (int)s.degrees + 1;
      Serial.printf("  -> New Temp: %d\n", (int)s.degrees);
      sendAcCommand();
    }
    if (Blynk.connected()) Blynk.virtualWrite(VP_TEMP_UP, 0);
  }
}

BLYNK_WRITE(VP_TEMP_DOWN) {
  int v = param.asInt();
  Serial.printf("[BTN] TEMP_DOWN: %d\n", v);

  if (v == 1 && s.power) {
    if ((int)s.degrees > TEMP_MIN) {
      s.degrees = (int)s.degrees - 1;
      Serial.printf("  -> New Temp: %d\n", (int)s.degrees);
      sendAcCommand();
    }
    if (Blynk.connected()) Blynk.virtualWrite(VP_TEMP_DOWN, 0);
  }
}

// Toggle Auto <-> Cool
BLYNK_WRITE(VP_MODE_TOGGLE) {
  int v = param.asInt();
  Serial.printf("[BTN] MODE_TOGGLE: %d\n", v);

  if (v == 1) {
    if (s.mode == stdAc::opmode_t::kCool) {
      s.mode    = stdAc::opmode_t::kAuto;
      s.degrees = 25;
    } else {
      s.mode    = stdAc::opmode_t::kCool;
      s.degrees = 20;
    }

    if (!s.power) s.power = true;
    sendAcCommand();

    if (Blynk.connected()) Blynk.virtualWrite(VP_MODE_TOGGLE, 0);
  }
}

BLYNK_WRITE(VP_BTN_NEXT) {
  int v = param.asInt();
  Serial.printf("[BTN] NEXT_VENDOR: %d\n", v);

  if (v == 1) {
    vendor_idx = (vendor_idx + 1) % VENDOR_COUNT;
    applyVendor(VENDORS[vendor_idx]);

    sendAcCommand();

    if (Blynk.connected()) Blynk.virtualWrite(VP_BTN_NEXT, 0);
  }
}

BLYNK_WRITE(VP_BTN_PREV) {
  int v = param.asInt();
  Serial.printf("[BTN] PREV_VENDOR: %d\n", v);

  if (v == 1) {
    vendor_idx = (vendor_idx - 1 + VENDOR_COUNT) % VENDOR_COUNT;
    applyVendor(VENDORS[vendor_idx]);

    sendAcCommand();

    if (Blynk.connected()) Blynk.virtualWrite(VP_BTN_PREV, 0);
  }
}

BLYNK_CONNECTED() {
  Serial.println("[Blynk] Connected ke server.");
  normalizeTwoModes(&s);
  updateBlynkDisplay();
  resetMomentaryButtons();
}

// (opsional, buat debug WiFi)
void WiFiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("[WiFi] Terhubung ke AP.");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("[WiFi] IP: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.print("[WiFi] Terputus, reason=");
      Serial.println(info.wifi_sta_disconnected.reason);
      break;
    default:
      break;
  }
}

// ---------- TIMER UNTUK CEK KONEKSI ----------

BlynkTimer timer;

const uint32_t WIFI_RECONNECT_MS  = FAST_TEST_MODE ? 3000 : 10000;
const uint32_t BLYNK_RECONNECT_MS = FAST_TEST_MODE ? 5000 : 15000;

unsigned long lastWifiAttempt  = 0;
unsigned long lastBlynkAttempt = 0;

void checkConnections() {
  unsigned long now = millis();

  // 1. WiFi dulu
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWifiAttempt > WIFI_RECONNECT_MS) {
      Serial.println("[WiFi] Tidak terhubung, mencoba reconnect...");
      WiFi.reconnect();
      lastWifiAttempt = now;
    }
    return;
  }

  // 2. Kalau WiFi OK, cek Blynk
  if (!Blynk.connected()) {
    if (now - lastBlynkAttempt > BLYNK_RECONNECT_MS) {
      Serial.println("[Blynk] Tidak terhubung, mencoba connect...");
      Blynk.connect(800);
      lastBlynkAttempt = now;
    }
  }
}

// ---------- SELF TEST / UJI CEPAT ----------
// Aktif hanya saat FAST_TEST_MODE = true
void selfTestTask() {
  static bool up = true;

  if (!s.power) s.power = true;

  if (up) {
    if ((int)s.degrees < TEMP_MAX) {
      s.degrees++;
    } else {
      up = false;
    }
  } else {
    if ((int)s.degrees > TEMP_MIN) {
      s.degrees--;
    } else {
      up = true;
    }
  }

  Serial.println("[SELFTEST] Mengirim perubahan suhu otomatis.");
  sendAcCommand();
}

// ---------- SETUP / LOOP ----------

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Remote AC multi-merk + WiFiManager...");

  // DIUBAH: State awal pakai HAIER_AC_YRW02 (AQUA), mode Auto, suhu awal 25°C
  IRac::initState(&s,
                  HAIER_AC_YRW02, -1,
                  false,
                  stdAc::opmode_t::kAuto,
                  25.0,
                  true,
                  stdAc::fanspeed_t::kAuto,
                  stdAc::swingv_t::kOff,
                  stdAc::swingh_t::kOff,
                  false, false, false, false,
                  false, false, true,
                  -1, -1);
  prev = s;

  WiFi.onEvent(WiFiEventHandler);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.setSleep(false);

  // --- WiFiManager: konfigurasi SSID/PASS ---
  WiFiManager wm;
  const char* apName = "RemoteAC-Setup";
  const char* apPass = "12345678";

  Serial.println("[WiFiManager] Mencoba autoConnect...");
  bool res = wm.autoConnect(apName, apPass);

  if (!res) {
    Serial.println("[WiFiManager] Gagal connect & user tidak konfigurasi. Restart...");
    delay(3000);
    ESP.restart();
  } else {
    Serial.print("[WiFiManager] Terhubung ke WiFi, IP: ");
    Serial.println(WiFi.localIP());
  }

  // --- Blynk: config + connect sekali ---
  Serial.println("[Blynk] Konfigurasi & menghubungkan ke server Blynk...");
  Blynk.config(auth);
  Blynk.connect(1500);

  // DIUBAH: Menggunakan index 5 (HAIER/AQUA) saat setup awal
  vendor_idx     = 5;
  current_vendor = VENDORS[5];
  applyVendor(current_vendor);

  // Timer cek koneksi
  timer.setInterval(FAST_TEST_MODE ? 3000L : 10000L, checkConnections);

  // Self-test untuk uji cepat
  if (FAST_TEST_MODE) {
    timer.setInterval(2000L, selfTestTask);
    Serial.println("[TEST] FAST_TEST_MODE aktif.");
  }

  Serial.println("Siap: Multi-merk (Default: AQUA) + WiFiManager.");
}

void loop() {
  unsigned long now = millis();

  // monitor loop yang tersendat
  if (lastLoop != 0 && now - lastLoop > 200) {
    Serial.printf("[WARN] Loop terlambat: %lu ms\n", now - lastLoop);
  }
  lastLoop = now;

  Blynk.run();
  timer.run();
}
