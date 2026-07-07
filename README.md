<div align="center">

# Universal AC Smart Remote (Blynk IoT)

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3_%7C_C3-blue.svg)]()
[![IoT](https://img.shields.io/badge/IoT-Blynk-green.svg)]()
[![Library](https://img.shields.io/badge/Library-IRremoteESP8266-orange.svg)]()

<img src="assets/tampilan app mobile blynk.jpeg" width="300" alt="Blynk Dashboard">
</div>

---

## 📖 Latar Belakang & Motivasi
Proyek *Independent/Personal* ini murni didasari oleh keresahan pribadi saya sebagai mahasiswa di Surabaya. Cuaca Surabaya yang sangat panas membuat kondisi kamar kos terasa seperti oven saat baru pulang bepergian atau pulang kuliah. 

Kondisi tersebut memotivasi saya untuk mendesain dan merancang alat pintar berbasis IoT yang memungkinkan saya untuk **menyalakan dan mengontrol AC di kamar kos dari mana saja** sebelum saya tiba. Dengan begitu, kamar sudah berada dalam kondisi sejuk tepat ketika saya membuka pintu.

## ⚙️ Fitur Utama
- **Kontrol Jarak Jauh (Anywhere Access):** Terintegrasi penuh dengan platform **Blynk IoT** untuk kontrol melalui aplikasi *mobile* dari jaringan mana pun.
- **Multibrand Support:** Menggunakan pustaka (library) sakti `IRremoteESP8266`, sistem ini secara dinamis bisa mengganti jenis sinyal protokol AC untuk berbagai merek populer di Indonesia, antara lain:
  - Midea (Coolix)
  - LG
  - Sharp
  - Daikin
  - Panasonic
  - Aqua (Haier / Sanyo)
- **Auto-Provisioning WiFi:** Menggunakan `WiFiManager`, alat tidak memerlukan *hardcode* kata sandi WiFi, sehingga sangat *plug-and-play* jika dibawa pindah ke jaringan baru.
- **Visualisasi Real-time:** Menampilkan Mode (Cool/Auto), Suhu, dan Merek AC langsung di panel LCD virtual aplikasi Blynk.

---

## 🛠 Spesifikasi Perangkat Keras
Berikut adalah perangkat keras yang menyusun sistem ini:
1. **Microcontroller:** ESP32-S3 (Sangat kompatibel dengan varian murah dan kecil seperti ESP32-C3 Supermini).
2. **IR Transmitter Dual Channel:** Bertugas memancarkan kode infra-merah yang dikonstruksi oleh algoritma ke arah AC.
3. **IR Receiver:** Digunakan untuk *reverse-engineering* atau melakukan *sniffing* terhadap kode remote AC bawaan (opsional, untuk *debugging*).

<div align="center">
  <img src="assets/esp32-s3.jpg" width="250" alt="ESP32-S3"> 
  <img src="assets/ir transmitter dual channel.jpg" width="250" alt="IR Transmitter"> 
  <img src="assets/IR receiver.jpg" width="250" alt="IR Receiver">
</div>

---

## 💻 Dependensi & Perangkat Lunak
Pastikan pustaka berikut terinstal di dalam Arduino IDE Anda sebelum melakukan kompilasi (*compile*):
- `BlynkSimpleEsp32.h` (Komunikasi IoT Blynk)
- `IRremoteESP8266.h` & `IRac.h` (Protokol dan konstruksi sinyal IR AC)
- `WiFiManager.h` (Portal penangkap koneksi WiFi)

> [!IMPORTANT]
> **Pengamanan Kredensial:** Variabel otentikasi `BLYNK_TEMPLATE_ID` dan `BLYNK_AUTH_TOKEN` pada berkas `Remote_AC_multi_brand.ino` di repositori ini telah di-*redact* demi alasan keamanan. Anda wajib menggantinya dengan Token Blynk milik Anda sendiri.

---

## 👨‍💻 Kontributor & Pengembangan
**Ali Akbar Alhabsyi (ali1140)**  
Proyek ini membuktikan kemampuan saya dalam memecahkan masalah (*problem solving*) dunia nyata menggunakan pendekatan teknik tertanam (*Embedded Engineering*). Kompleksitas utama dalam proyek ini terletak pada pemahaman lapisan protokol inframerah (*Infrared Protocol Layer*) yang rumit untuk setiap merek AC yang berbeda, yang kemudian berhasil diabstraksikan menjadi antarmuka UI aplikasi seluler yang sangat mudah digunakan.
