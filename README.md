# Otonom Labirent Çözen Robot (Maze Robot) - Donanım ve Yazılım Rehberi

Bu doküman, otonom labirent çözen robot projemizin donanım montajını yapacak ekip üyeleri için **kablolama rehberi**, yazılım ekibi için ise **güncel pin haritası ve algoritma referansı** olarak hazırlanmıştır.

## 📑 İçindekiler
- [🎯 Projenin Amacı ve Çalışma Senaryosu](#-projenin-amacı-ve-çalışma-senaryosu)
- [📦 Kullanılan Parçalar (Malzeme Listesi)](#-kullanılan-parçalar-malzeme-listesi)
- [1. Sistem Mimarisi ve Donanım Görev Dağılımı](#1-sistem-mimarisi-ve-donanım-görev-dağılımı)
- [2. Güç ve Voltaj Dağıtım Mimarisi (Kritik Uyarılar)](#2-güç-ve-voltaj-dağıtım-mimarisi-kritik-uyarılar)
- [3. Ana Pin Bağlantı Tablosu (Donanım & Yazılım Referansı)](#3-ana-pin-bağlantı-tablosu-donanım--yazılım-referansı)
- [4. Modül Bazlı Fiziksel Montaj ve Kablolama Rehberi](#4-modül-bazlı-fiziksel-montaj-ve-kablolama-rehberi)
- [5. Güncel Yazılım Mimarisi ve Algoritma Özeti](#5-güncel-yazılım-mimarisi-ve-algoritma-özeti)

## 🎯 Projenin Amacı ve Çalışma Senaryosu

Robotumuz, 4-pin PWM motor kontrolü ve 5 durumlu **Durum Makinesi (State Machine)** ile labirent koridorlarında otonom olarak ilerler. Güncel çalışma döngüsü şu şekildedir:
1. **Düz Sürüş (`FORWARD`):** MPU6050 jiroskobunun Z ekseni açısal verisi (Yaw) ile kapalı çevrim oransal kontrol (P-Kontrol) uygulayarak $0^\circ$ yönünde düz ilerler. *(H2010 hız sensörleri şasede bağlıdır ancak mevcut yazılım aşamasında devre dışıdır.)*
2. **Engel Algılama ve Aktif Fren (`BRAKE_WAIT`):** Ön HC-SR04 ultrasonik sensör $20\text{ cm}$ altında bir engel algıladığında motorlara elektronik aktif fren (`HIGH/HIGH`) uygular ve $250\text{ ms}$ boyunca aracın tam durmasını bekler.
3. **Yön Kararı ve $90^\circ$ Dönüş (`DECIDE` → `TURNING`):** Sağ ve sol HC-SR04 sensörlerini karşılaştırarak daha açık olan yöne karar verir. MPU6050 verisiyle oransal kontrollü $90^\circ$ tank dönüşü yapar (MPU bağlantısı koparsa otomatik olarak zaman bazlı yedek dönüşe geçer).
4. **Dönüş Sonrası Doğrulama (`POST_TURN_WAIT`):** Dönüş bitiminde $500\text{ ms}$ aktif frende beklerken yeni yönün mesafelerini taze olarak ölçer; yeni ön yön de kapalıysa (çıkmaz sokak) ileri gitmeden doğrudan tekrar dönüş kararı verir, yol açıksa yönü sıfırlayıp düz devam eder.
5. **Kablosuz Telemetri:** Tüm süreç boyunca ön/sol/sağ mesafe, Yaw açısı, aktif durum (`State`) ve dönüş bitiş özeti HC-05 Bluetooth modülü üzerinden **`9600 baud`** hızında bilgisayara aktarılır (`telemetry-cli.py`).

## 📦 Kullanılan Parçalar (Malzeme Listesi)

* **Arduino Uno R3 (SMD)** $\times 1$
* **L298N Motor Sürücü Modülü** $\times 1$
* **Sarı Redüktörlü TT DC Motor (6V, 250 RPM)** $\times 2$
* **H2010 Optokuplör Hız Sensörü (Enkoder)** $\times 2$
* **HC-SR04 Ultrasonik Mesafe Sensörü** $\times 3$ *(Ön, Sağ, Sol)*
* **MPU6050 Jiroskop ve İvmeölçer Modülü** $\times 1$
* **HC-05 Bluetooth Modülü** $\times 1$
* **1.5V 4 Slot AA Pil Yuvası** $\times 2$ *(Toplam 8 LR6 Alkalin Pil)*
* **Gerilim Bölücü Dirençleri** *($1\text{ k}\Omega$ / $2\text{ k}\Omega$ veya $3 \times$ eş direnç ile 1:2 oran — HC-05 RXD için)*
* **KCD11 Açma/Kapama Anahtarı** $\times 1$

## 1. Sistem Mimarisi ve Donanım Görev Dağılımı

Robot üzerindeki her bir parçanın sistem içindeki görevi ve diğer parçalarla olan ilişkisi aşağıda detaylandırılmıştır:

### 🧠 Arduino Uno R3 (SMD) – Ana Kontrolcü
Tüm sensörlerden gelen verileri okur, 5 durumlu State Machine ile sürüş ve dönüş kararlarını verir, motor sürücüye 4-pin PWM sinyallerini üretir ve telemetri paketlerini donanımsal UART (`D0/D1`) üzerinden bilgisayara aktarır.

![Arduino Uno R3 Pinout](<images/Arduino Uno R3 SMD.png>)

### ⚡ L298N Motor Sürücü – Güç ve Yön Aktüatörü
Arduino'nun dijital pinleri motorları çalıştırmak için gereken yüksek akımı doğrudan veremez. L298N, pil grubundan aldığı gücü kullanarak Arduino'dan gelen **PWM** sinyalleriyle motorları kontrol eder. Güncel mimaride `ENA` ve `ENB` jumper'ları takılı bırakılır; hız, yön ve aktif fren (`HIGH/HIGH`) doğrudan 4 PWM yön pininden (`IN1`–`IN4`) yönetilir.

![L298N Motor Sürücü](<images/L298N - Motor  Driver.png>)

### ⚙️ Sarı Redüktörlü TT Motorlar (2 Adet) – Tahrik Sistemi
$1:48$ dişli redüksiyon oranıyla tekerleklere tork ve hareket sağlar.

![Sarı TT Motor Özellikleri](<images/Sarı Motor.png>)

### ⏱️ H2010 Optokuplör Hız Sensörleri (2 Adet) – Odometri (Gelecek Faz)
Motor miline bağlı delikli enkoder diskinin dönüşünü dijital darbelere (`D0`) çevirerek tekerlek hızını/mesafesini ölçer. Donanımsal olarak `D2` ve `D3` kesme pinlerine bağlıdır; mevcut sürümde düz gidiş ve dönüş yalnızca MPU6050 ile yapıldığı için yazılımda şimdilik pasif bırakılmıştır.

![H2010 Hız Sensörü Şeması](<images/H2010 - Hız Sensoru.png>)

### 🧭 MPU6050 Jiroskop ve İvmeölçer – Yönelim (Yaw) ve $90^\circ$ Dönüş Kontrolü
I2C (`A4/A5`, $100\text{ kHz}$) üzerinden Z ekseni açısal hızını okur. Dahili $44\text{ Hz}$ alçak geçiren filtre (DLPF) ve açılışta yapılan 200 örneklik sabit sapma (bias) kalibrasyonu sayesinde hem düz koridorda yönü $0^\circ$'da tutar hem de $90^\circ$ dönüşlerde açıyı ölçer.

![MPU6050 Pinout](<images/MPU6050 - Jiroskop & İvmeölçer.png>)

### 📏 HC-SR04 Ultrasonik Mesafe Sensörleri (3 Adet) – Çevre Algılama
Ses dalgaları gönderip yankı süresini ölçerek mesafeyi hesaplar. Ön sensör engel algılama ve durma kararını verir; sağ ve sol sensörler ise dönüş anında daha açık olan yönü seçmek için kullanılır.

![HC-SR04 Ultrasonik Mesafe Sensörü](<images/HC-SR04 - Ultrasonik Mesafe Sensörü.png>)

### 📡 HC-05 Bluetooth Modülü – Kablosuz Telemetri Hattı
Araç üzerindeki mesafe, yönelim açısı, aktif durum (`State`) ve dönüş bitiş verilerini donanımsal UART (`9600 baud`) üzerinden gerçek zamanlı olarak bilgisayara aktarır.

![HC-05 Bluetooth Pinout](<images/HC-05 - Bluetooth.jpg>)

---

## 2. Güç ve Voltaj Dağıtım Mimarisi (Kritik Uyarılar)

Sistemde toplam 8 adet $1.5\text{ V}$ **LR6 Alkalin** AA pil (iki adet 4'lü pil yuvası seri bağlı) kullanılmaktadır. Motor kalkışlarında voltaj çökmesini (brownout reset) önlemek için aşağıdaki güç dağılımı uygulanır:

### 🔋 Pil Bağlantısı ve Motor Voltajı Sınırı
* İki adet 4'lü AA pil yuvası seri bağlandığında nominal **$12\text{ V}$** elde edilir (anahtar `KCD11` üzerinden ana hatta bağlanır). Çinko-karbon (R6) piller kalkış akımında voltaj çökmesine neden olduğundan **LR6 Alkalin** pil kullanılmalıdır.
* L298N motor sürücü iç yapısında yaklaşık **$1.5\text{ V}-2\text{ V}$** gerilim düşümü yaratır; sarı TT motorların nominal voltajı $6\text{ V}$ olduğundan yazılımda `MAX_PWM = 120`, `CRUISE_PWM = 55` ve yumuşak kalkış (soft-start) ile akım sınırlandırılır.

### 🔌 Arduino ve 5V Sensör Besleme Hattı (Yük Paylaşımı)
* Seri bağlı $12\text{ V}$ pil hattından iki paralel hat çıkar: biri L298N **+12V Power** klemensine, diğeri doğrudan Arduino Uno **VIN** pinine gider.
* L298N üzerindeki **5V Enable** jumper'ı takılı kalır; **L298N +5V çıkışı ile Arduino 5V pini birbirine KESİNLİKLE bağlanmaz** (regülatör çakışması olmaması için).
* **Gürültüye hassas modüller (`MPU6050` ve `HC-05`)** doğrudan **Arduino Uno'nun 5V pinlerinden** beslenir.
* **Ultrasonik sensörler (`HC-SR04` $\times 3$) ve `H2010` ($\times 2$)** L298N'in **+5V** çıkışından dağıtılan güç hattından beslenir.

### ⚠️ HC-05 RXD Pini İçin 3.3V Gerilim Bölücü (Zorunlu)
* HC-05'in **VCC** pini $5\text{ V}$ ile çalışır ancak **RXD** veri girişi **$3.3\text{ V}$** lojik seviyesindedir.
* Arduino'nun $5\text{ V}$ çıkış veren **D1 (TX)** pini doğrudan HC-05 `RXD`'ye bağlanmamalı, araya $1:2$ oranlı gerilim bölücü ($1\text{ k}\Omega$ / $2\text{ k}\Omega$ veya üç eş dirençle $R$ / $2R$) konmalıdır:
  * Arduino `D1 (TX)` $\rightarrow$ $1\text{ k}\Omega$ $\rightarrow$ HC-05 `RXD` $\rightarrow$ $2\text{ k}\Omega$ $\rightarrow$ `GND`

### ⏚ Ortak Toprak (Common GND) Kuralı
* Pil grubunun eksi ($-$) kutbu, L298N **Power GND** klemensi, Arduino Uno **GND** pinleri ve tüm sensörlerin **GND** bacakları tek bir ortak toprak hattında birleştirilmelidir.

---

## 3. Ana Pin Bağlantı Tablosu (Donanım & Yazılım Referansı)

Aşağıdaki tablo, [Config.h](maze-robot-control/src/Config.h) dosyasındaki güncel pin tanımlarıyla birebir uyumludur *(Analog `A0`–`A3` pinleri Arduino Uno'da dijital `D14`–`D17` olarak çalışır)*:

| Bileşen | Modül Pini | Arduino Pini | Sinyal Tipi | Yazılım / Donanım Notu |
| :--- | :--- | :--- | :--- | :--- |
| **Sol H2010 Hız Sensörü** | `D0` | **D2** | Dijital Giriş (`INT0`) | Fiziksel bağlı, mevcut yazılımda pasif |
| **Sağ H2010 Hız Sensörü** | `D0` | **D3** | Dijital Giriş (`INT1`) | Fiziksel bağlı, mevcut yazılımda pasif |
| **L298N Motor Sürücü** | `ENA` | **Jumper Takılı** | Sabit `5V` | 4-Pin PWM için kısa devre jumper takılı |
| **L298N Motor Sürücü** | `IN1` | **D5 (~)** | PWM Çıkışı (`Timer0`) | Sol Motor Yön/Hız 1 (`LEFT_IN1`) |
| **L298N Motor Sürücü** | `IN2` | **D6 (~)** | PWM Çıkışı (`Timer0`) | Sol Motor Yön/Hız 2 (`LEFT_IN2`) |
| **L298N Motor Sürücü** | `IN3` | **D9 (~)** | PWM Çıkışı (`Timer1`) | Sağ Motor Yön/Hız 1 (`RIGHT_IN3`) |
| **L298N Motor Sürücü** | `IN4` | **D10 (~)** | PWM Çıkışı (`Timer1`) | Sağ Motor Yön/Hız 2 (`RIGHT_IN4`) |
| **L298N Motor Sürücü** | `ENB` | **Jumper Takılı** | Sabit `5V` | 4-Pin PWM için kısa devre jumper takılı |
| **Ön HC-SR04 Mesafe** | `Trig` | **A0 (D14)** | Dijital Çıkış | $10\text{ }\mu\text{s}$ tetikleme darbesi |
| **Ön HC-SR04 Mesafe** | `Echo` | **A1 (D15)** | Dijital Giriş | Yankı süresi okuma |
| **Sol HC-SR04 Mesafe** | `Trig` | **A2 (D16)** | Dijital Çıkış | $10\text{ }\mu\text{s}$ tetikleme darbesi |
| **Sol HC-SR04 Mesafe** | `Echo` | **A3 (D17)** | Dijital Giriş | Yankı süresi okuma |
| **Sağ HC-SR04 Mesafe** | `Trig` | **D13** | Dijital Çıkış | $10\text{ }\mu\text{s}$ tetikleme darbesi |
| **Sağ HC-SR04 Mesafe** | `Echo` | **D8** | Dijital Giriş | Yankı süresi okuma |
| **MPU6050 Jiroskop** | `SDA` | **A4 (D18/SDA)** | I2C Veri Hattı | Donanımsal I2C (`100 kHz`) |
| **MPU6050 Jiroskop** | `SCL` | **A5 (D19/SCL)** | I2C Saat Hattı | Donanımsal I2C (`100 kHz`) |
| **HC-05 Bluetooth** | `TXD` | **D0 (RX)** *(opsiyonel)* | Donanımsal UART RX | Kod yüklerken geçici olarak çıkarılır |
| **HC-05 Bluetooth** | `RXD` | **D1 (TX)** | Donanımsal UART TX | $1\text{ k}\Omega$ / $2\text{ k}\Omega$ gerilim bölücü ile (`9600 baud`) |

> **Önemli Not (Donanımsal UART ve Kod Yükleme):** `SoftwareSerial` kütüphanesi Arduino Uno'da `Timer1` kesmelerini bozarak `D9/D10` PWM çıkışlarını kilitlediği için Bluetooth doğrudan donanımsal seri porta (`D0/D1`, **`9600 baud`**) bağlanmıştır. Bilgisayardan USB ile yeni kod yüklerken `D0` (ve garanti için `D1`) kablosunu geçici olarak söküp yükleme bitince geri takın.

---

## 4. Modül Bazlı Fiziksel Montaj ve Kablolama Rehberi

### 1. L298N ve Sarı TT Motor Bağlantıları
* Sol sarı motorun kablolarını L298N **Output A** klemensine, sağ sarı motorun kablolarını **Output B** klemensine bağlayın (tekerleklerden biri ileri komutunda geri dönüyorsa o motorun klemensindeki iki kablosunu yer değiştirin).
* L298N üzerindeki **ENA**, **ENB** ve **5V Enable** siyah jumper'larının **hepsini takılı bırakın**.
* Logic Input pinlerini Arduino'nun PWM destekli pinlerine bağlayın: `IN1 -> D5`, `IN2 -> D6`, `IN3 -> D9`, `IN4 -> D10`.

### 2. H2010 Hız Sensörleri (Enkoderler)
* Delikli enkoder diskleri H2010'un U yarığının tam ortasından sürtünmeden dönecek şekilde sensörleri sabitleyin.
* `VCC` -> L298N 5V hattı, `GND` -> ortak toprak, `D0` -> sol için `D2`, sağ için `D3`.

### 3. MPU6050 Jiroskop ve İvmeölçer
* Modülü şasi tabanına tam yatay (zemine paralel), titreşimden uzak ve eksen okları aracın ileri/yan yönüne hizalı olacak şekilde sabitleyin (pin header'lar mutlaka lehimli olmalıdır).
* `VCC` -> **Arduino 5V**, `GND` -> ortak toprak, `SDA` -> `A4`, `SCL` -> `A5`. `AD0`, `XDA`, `XCL`, `INT` boş bırakılır (varsayılan I2C adresi `0x68`, kod otomatik `0x68`/`0x69` tarar).

### 4. HC-SR04 Ultrasonik Mesafe Sensörleri (Ön, Sol, Sağ)
* Üç sensörün `VCC` pinlerini L298N +5V hattına, `GND` pinlerini ortak toprağa bağlayın.
* **Ön:** `Trig -> A0`, `Echo -> A1` | **Sol:** `Trig -> A2`, `Echo -> A3` | **Sağ:** `Trig -> D13`, `Echo -> D8`.

### 5. HC-05 Bluetooth Modülü (Datasheet Görselindeki Hataya Dikkat!)
* **🚨 Görsel Uyarısı:** `HC-05 - Bluetooth.jpg` görselinin alt kısmındaki kırmızı oklar hatalı çizilmiştir. Bağlantıda mavi PCB üzerindeki beyaz yazıları (`STATE`, `RXD`, `TXD`, `GND`, `VCC`, `EN`) baz alın.
* `VCC` -> **Arduino 5V**, `GND` -> ortak toprak.
* `RXD` -> Arduino `D1 (TX)` pininden gelen gerilim bölücü çıkışı; `TXD` -> yalnızca komut alınacaksa Arduino `D0 (RX)` (telemetri için boş kalabilir). `STATE` ve `EN` boş bırakılır.

---

## 5. Güncel Yazılım Mimarisi ve Algoritma Özeti

Firmware kodu [maze-robot-control/src/](maze-robot-control/src/) klasöründe 4 sade modülden oluşur:

* **[Config.h](maze-robot-control/src/Config.h):** Tüm pin tanımları, PWM limitleri, dönüş açısı/toleransı ve zamanlama sabitleri.
* **[Sensors.cpp](maze-robot-control/src/Sensors.cpp):** MPU6050 başlatma (`100 kHz` I2C, `3 ms` timeout, `44 Hz` DLPF, 200 örneklik bias kalibrasyonu), açı entegrasyonu (`updateHeading`) ve 3 ardışık timeout filtreli HC-SR04 okuma (`getDistance`).
* **[Motors.cpp](maze-robot-control/src/Motors.cpp):** 4-pin PWM motor sürme (`setMotor`), düz gidiş için `millis()` tabanlı yumuşak kalkış (`drive`), dönüşler için doğrudan hız komutu (`setDirect`) ve `HIGH/HIGH` aktif fren (`activeBrake`).
* **[main.cpp](maze-robot-control/src/main.cpp):** 5 durumlu bloklamayan (non-blocking) **State Machine** ve `9600 baud` telemetri:
  1. `FORWARD`: MPU6050 ile $0^\circ$ yönünde düz sürüş (`CRUISE_PWM = 55`, `HEADING_KP = 1.0`, `MAX_CORR = 10`). Ön mesafe $2\text{ cm}-20\text{ cm}$ arasındaysa aktif fren yapıp `BRAKE_WAIT`'e geçer.
  2. `BRAKE_WAIT`: $250\text{ ms}$ aktif frende bekler (araç durur, taze sağ/sol mesafe okunur) → `DECIDE`.
  3. `DECIDE`: `rightDist > leftDist` ise sağa ($-90^\circ$), değilse sola ($+90^\circ$) dönüş hedefini seçer ve `heading` açısını sıfırlar → `TURNING`.
  4. `TURNING`: Ultrasonik okumayı geçici olarak durdurup yüksek frekansta MPU6050 okuyarak oransal kontrollü tank dönüşü yapar (`TURN_TOLERANCE = 3.0°`, `TURN_TIMEOUT_MS = 2000 ms`, MPU koparsa `TURN_90_MS = 500 ms` yedek mod). Bitişte `TURN_END` özeti basıp `POST_TURN_WAIT`'e geçer.
  5. `POST_TURN_WAIT`: $500\text{ ms}$ aktif frende beklerken eski mesafe hafızasını sıfırlayıp yeni yönün taze mesafelerini okur; yeni ön yön de kapalıysa ileri hareket etmeden tekrar `DECIDE`'a, açıksa yönü sıfırlayıp `FORWARD`'a geçer.

### 📡 Telemetri Aracını Çalıştırma (Ubuntu)
Proje kök dizinindeki [telemetry-cli.py](telemetry-cli.py) aracı eşleştirilmiş `HC-05` modülünü otomatik bulur, `/dev/rfcomm0` portunu `9600 baud` ile açar, canlı veriyi ekrana basar ve `surus_verisi.csv` dosyasına kaydeder:

```bash
python3 telemetry-cli.py
```

PlatformIO üzerinden USB ile derleme/yükleme ve seri monitör (`9600 baud`) için:
```bash
cd maze-robot-control
pio run
pio device monitor
```
