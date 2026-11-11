# Otonom Labirent Çözen Robot (Maze Robot) - Donanım ve Yazılım Rehberi

Bu doküman, otonom labirent çözen robot projemizin donanım montajını yapacak ekip üyeleri için **adım adım kablolama rehberi**, yazılım ekibi için ise **pin haritası ve algoritma referansı** olarak hazırlanmıştır. 

> **GitHub Görsel Notu:** Bu dokümandaki şemaların GitHub üzerinde otomatik görünmesi için ekteki görsel dosyalarını (`Arduino Uno R3 SMD.png`, `L298N - Motor  Driver.jpg` vb.) bu `README.md` dosyası ile aynı dizine (ana klasöre) yüklemeniz yeterlidir.

---

## 🎯 Projenin Amacı ve Çalışma Senaryosu

Robotumuz, motor sürücü aracılığıyla tekerleklere güç vererek labirent koridorlarında otonom olarak ilerleyecektir. Temel çalışma döngüsü şu şekildedir:
1. **Düz Sürüş:** Hız sensörlerinden (enkoder) ve jiroskoptan aldığı verilerle iki tekerleğin hızını eşitleyerek koridorda sapmadan, düz bir çizgide ilerler.
2. **Engel Algılama ve Durma:** Ön, sağ ve soldaki ultrasonik mesafe sensörleri ile çevresini sürekli tarar. Önüne bir engel (duvar) çıktığında mesafeye bağlı olarak önce kademeli şekilde yavaşlar, ardından tam duruş yapar.
3. **Yön Kararı ve Dönüş:** Sağ ve sol mesafe sensörlerinden gelen verilere göre açık olan yolu seçer. Jiroskop (MPU6050) verisini referans alarak tam açılı ($90^\circ$ veya $180^\circ$) dönüş yapar ve yeni yönde ilerlemeye devam eder.
4. **Kablosuz Telemetri:** Tüm bu süreç boyunca sensörlerden okunan veriler ve aracın anlık karar durumu Bluetooth modülü (HC-05) üzerinden gerçek zamanlı olarak ana bilgisayara aktarılır.

---

## 📦 Kullanılan Parçalar (Malzeme Listesi)

* **Arduino Uno R3 (SMD)** $\times 1$
* **L298N Motor Sürücü Modülü** $\times 1$
* **Sarı Redüktörlü TT DC Motor (6V, 250 RPM)** $\times 2$
* **H2010 Optokuplör Hız Sensörü (Enkoder)** $\times 2$
* **HC-SR04 Ultrasonik Mesafe Sensörü** $\times 3$ *(Ön, Sağ, Sol)*
* **MPU6050 Jiroskop ve İvmeölçer Modülü** $\times 1$
* **HC-05 Bluetooth Modülü** $\times 1$
* **1.5V 4 Slot AA Pil Yuvası** $\times 2$ *(Toplam 8 Pil Yuvası)*
* **1.5V AA Pil** $\times 8$
* **1k$\Omega$ ve 2k$\Omega$ Direnç** *(HC-05 RXD gerilim bölücü devresi için)*

---

## 1. Sistem Mimarisi ve Donanım Görev Dağılımı

Robot üzerindeki her bir parçanın sistem içindeki görevi ve diğer parçalarla olan ilişkisi aşağıda detaylandırılmıştır:

### 🧠 Arduino Uno R3 (SMD) – Ana Kontrolcü
Tüm sensörlerden gelen verileri okur, duvar takip ve dönüş kararlarını verir, motor sürücüye hız ve yön sinyallerini üretir ve telemetri paketlerini Bluetooth üzerinden ana bilgisayara aktarır.

![Arduino Uno R3 Pinout](<images/Arduino Uno R3 SMD.png>)

### ⚡ L298N Motor Sürücü – Güç ve Yön Aktüatörü
Arduino'nun dijital pinleri motorları çalıştırmak için gereken yüksek akımı doğrudan veremez (Arduino pinleri maksimum $20\text{ mA}-40\text{ mA}$ verirken motorlar yüzlerce miliamper çeker). L298N, pil grubundan aldığı yüksek gücü kullanarak Arduino'dan gelen düşük güçlü **PWM** sinyalleriyle motorları kademeli olarak hızlandırır veya yavaşlatır. Lojik yön pinleriyle (`IN1`–`IN4`) aracın ileri, geri, olduğu yerde eksen dönüşü (tank dönüşü) ve ani elektronik frenleme hareketlerini yaptırır.

![L298N Motor Sürücü](<images/L298N - Motor  Driver.jpg>)

### ⚙️ Sarı Redüktörlü TT Motorlar (2 Adet) – Tahrik Sistemi
$1:48$ dişli redüksiyon oranıyla tekerleklere tork ve hareket sağlar.

![Sarı TT Motor Özellikleri](<images/Sarı Motor.png>)

### ⏱️ H2010 Optokuplör Hız Sensörleri (2 Adet) – Odometri ve Hız Eşitleme
Üretim toleransları sebebiyle iki DC motora aynı voltaj verilse bile tekerlekler birebir aynı hızda dönmez ve araç bir tarafa çeker. H2010 üzerindeki kızılötesi yarık ve LM393 karşılaştırıcı entegre, motor miline bağlı delikli enkoder diskinin dönüşünü dijital sinyallere (`D0`) çevirir. Bu sayede sağ ve sol tekerleklerin anlık dönüş hızı (RPM) ve katettiği mesafe ölçülerek iki tekerleğin hızı yazılımsal olarak eşitlenir.

![H2010 Hız Sensörü Şeması](<images/H2010 - Hız Sensoru.jpg>)

### 🧭 MPU6050 Jiroskop ve İvmeölçer – Yönelim (Yaw) ve Hassas Dönüş Kontrolü
Sadece önüne engel çıktığında $90^\circ$ veya $180^\circ$ köşe dönüşlerini hassas ölçmekle kalmaz; araç düz koridorda ilerlerken zemin kayması (patinaj) nedeniyle tekerlek enkoderlerinin fark edemediği açısal sapmaları (Z ekseni dönüş hızı) anlık yakalayarak aracın düz bir çizgide kalmasını sağlar.

![MPU6050 Pinout](<images/MPU6050 - Jiroskop & İvmeölçer.png>)

### 📏 HC-SR04 Ultrasonik Mesafe Sensörleri (3 Adet) – Çevre Algılama ve Koridor Ortalama
Ses dalgaları gönderip yankı süresini ölçerek mesafeyi hesaplar. Ön sensör engele yaklaşırken kademeli yavaşlama ve durma mesafesini belirlerken, sağ ve sol sensörler hem açık koridor (dönüş yönü) kararını verir hem de düz sürüş esnasında aracın sağ ve sol duvarlara eşit uzaklıkta (koridor ortasında) kalmasını sağlar.

![HC-SR04 Ultrasonik Mesafe Sensörü](<images/HC-SR04 - Ultrasonik Mesafe Sensörü.jpg>)

### 📡 HC-05 Bluetooth Modülü – Kablosuz Telemetri ve Komut Hattı
Araç üzerindeki sensör verilerini, anlık hızı, yönelim açısını ve robotun karar durumunu UART seri haberleşme protokolü üzerinden gerçek zamanlı olarak ana makineye (PC) aktarır.

![HC-05 Bluetooth Pinout](<images/HC-05 - Bluetooth.jpg>)

---

## 2. Güç ve Voltaj Dağıtım Mimarisi (Kritik Uyarılar)

Sistemde toplam 8 adet $1.5\text{ V}$ AA pil (iki adet 4'lü pil yuvası) kullanılacaktır. Elektronik bileşenlerin yanmaması ve Arduino'nun motor parazitlerinden dolayı kapanıp açılmaması (reset atmaması) için aşağıdaki kurallara dikkat edilmelidir:

### 🔋 Pil Bağlantısı ve Motor Voltajı Sınırı
* İki adet 4'lü AA pil yuvası seri bağlandığında nominal **$12\text{ V}$** (tam dolu alkalin pillerle yaklaşık $12.8\text{ V}$) elde edilir.
* L298N motor sürücü kendi iç yapısında yaklaşık **$1.5\text{ V}$ ile $2\text{ V}$** arasında gerilim düşümü yaratır; yani $12\text{ V}$ giriş verildiğinde motorlara maksimum **$10\text{ V}$** ulaşır.
* Sarı TT motorlar $3\text{ V}-12\text{ V}$ aralığında çalışabilse de nominal tasarım voltajları **$6\text{ V}$ ($250\text{ RPM}$)** seviyesindedir ve maksimum $160\text{ mA}$ (boşta $95\text{ mA}$) akım çekerler. Motorlara sürekli $10\text{ V}$ vermek labirent içinde aşırı hıza ve plastik dişlilerin aşınmasına yol açacağından, yazılım ekibi PWM üst sınırını `255` yerine **`150-170`** bandında sınırlandırmalıdır.
* *Alternatif Güç Önerisi:* İki 4'lü pil yuvasını seri bağlamak yerine; birinci $6\text{ V}$ pil yuvası sadece L298N motor girişine, ikinci $6\text{ V}$ pil yuvası ise sadece Arduino `VIN` girişine ayrılarak tam $6\text{ V}$ motor sürüşü ve sıfır elektriksel gürültü elde edilebilir.

### 🔌 Arduino ve 5V Sensör Besleme Hattı
* Seri bağlı $12\text{ V}$ pil grubunun artı ($+$) kutbundan iki paralel kablo çıkarılmalıdır: Birinci kablo L298N üzerindeki **+12V Power** klemensine, ikinci kablo ise Arduino Uno üzerindeki **VIN** pinine (veya DC adaptör girişine) bağlanmalıdır.
* L298N üzerindeki **5V Enable** jumper'ı takılı bırakılmalı, ancak L298N'in **+5V Power** klemensi Arduino'nun **5V** pinine **kesinlikle bağlanmamalıdır** (iki voltaj regülatörünün çakışmasını ve motor kalkışlarındaki voltaj çökmesini önlemek için).
* HC-SR04 ($\times 3$), H2010 ($\times 2$), MPU6050 ve HC-05 modüllerinin **VCC** beslemeleri, doğrudan Arduino Uno'nun regüleli **+5V** pininden çoğaltılarak (breadboard üzerinden) dağıtılmalıdır.

### ⚠️ HC-05 RXD Pini İçin 3.3V Gerilim Bölücü (Zorunlu)
* HC-05 modülünün **VCC** pini $5\text{ V}$ ile çalışır ancak veri alma pini olan **RXD** lojik olarak **$3.3\text{ V}$** seviyesindedir.
* Arduino'nun $5\text{ V}$ çıkış veren TX pinini (`D12`) doğrudan HC-05 `RXD` pinine bağlamak modülü yakabilir. Araya **$1\text{ k}\Omega$ ve $2\text{ k}\Omega$** dirençlerle bir gerilim bölücü kurulmalıdır:
  * `Arduino D12 (TX)` $\rightarrow$ `$1\text{ k}\Omega$ Direnç` $\rightarrow$ `HC-05 RXD Pini` $\rightarrow$ `$2\text{ k}\Omega$ Direnç` $\rightarrow$ `GND`

### ⏚ Ortak Toprak (Common GND) Kuralı
* Pil yuvasının eksi ($-$) kutbu, L298N üzerindeki **Power GND** klemensi, Arduino Uno'nun **GND** pinleri ve tüm sensörlerin **GND** bacakları birbirine bağlanarak ortaklanmalıdır. Ortak toprak yapılmazsa sensör sinyalleri okunamaz.

---

## 3. Ana Pin Bağlantı Tablosu (Donanım & Yazılım Referansı)

Arduino Uno R3 üzerindeki donanımsal kesme (Interrupt), PWM ve I2C pinleri hiçbir çakışma olmayacak şekilde aşağıdaki gibi atanmıştır. 

*(Not: Arduino Uno üzerindeki Analog `A0`-`A3` pinleri aynı zamanda `D14`-`D17` dijital giriş/çıkış pinleri olarak tam performansla çalışmaktadır).*

| Bileşen | Modül Pini | Arduino Pini | Sinyal Tipi | Yazılım / Donanım Notu |
| :--- | :--- | :--- | :--- | :--- |
| **Sol H2010 Hız Sensörü** | `D0` | **D2** | Dijital Giriş (`INT0`) | Donanımsal kesme (Interrupt) pini |
| **Sağ H2010 Hız Sensörü** | `D0` | **D3** | Dijital Giriş (`INT1`) | Donanımsal kesme (Interrupt) pini |
| **L298N Motor Sürücü** | `ENA` (A Enable) | **D9 (~)** | PWM Çıkışı (`Timer1`) | Jumper çıkarılacak, Sol Motor Hız |
| **L298N Motor Sürücü** | `IN1` | **D4** | Dijital Çıkış | Sol Motor Yön 1 |
| **L298N Motor Sürücü** | `IN2` | **D5 (~)** | Dijital Çıkış | Sol Motor Yön 2 |
| **L298N Motor Sürücü** | `IN3` | **D6 (~)** | Dijital Çıkış | Sağ Motor Yön 1 |
| **L298N Motor Sürücü** | `IN4` | **D7** | Dijital Çıkış | Sağ Motor Yön 2 |
| **L298N Motor Sürücü** | `ENB` (B Enable) | **D10 (~)** | PWM Çıkışı (`Timer1`) | Jumper çıkarılacak, Sağ Motor Hız |
| **Ön HC-SR04 Mesafe** | `Trig` | **A0 (D14)** | Dijital Çıkış | $10\text{ }\mu\text{s}$ tetikleme darbesi |
| **Ön HC-SR04 Mesafe** | `Echo` | **A1 (D15)** | Dijital Giriş | Yankı süresi okuma |
| **Sol HC-SR04 Mesafe** | `Trig` | **A2 (D16)** | Dijital Çıkış | $10\text{ }\mu\text{s}$ tetikleme darbesi |
| **Sol HC-SR04 Mesafe** | `Echo` | **A3 (D17)** | Dijital Giriş | Yankı süresi okuma |
| **Sağ HC-SR04 Mesafe** | `Trig` | **D13** | Dijital Çıkış | $10\text{ }\mu\text{s}$ tetikleme darbesi |
| **Sağ HC-SR04 Mesafe** | `Echo` | **D8** | Dijital Giriş | Yankı süresi okuma |
| **MPU6050 Jiroskop** | `SDA` | **A4 (D18/SDA)** | I2C Veri Hattı | Donanımsal I2C SDA |
| **MPU6050 Jiroskop** | `SCL` | **A5 (D19/SCL)** | I2C Saat Hattı | Donanımsal I2C SCL |
| **HC-05 Bluetooth** | `TXD` | **D11** | Yazılımsal UART RX | Doğrudan `D11` pinine bağlanır |
| **HC-05 Bluetooth** | `RXD` | **D12** | Yazılımsal UART TX | $1\text{ k}\Omega$ / $2\text{ k}\Omega$ gerilim bölücü ile bağlanır |

> **Tasarım Notu:** Arduino'nun `D0 (RX)` ve `D1 (TX)` pinleri, bilgisayardan USB ile kod yüklerken kabloları sök-tak yapmamak ve Seri Port Ekranı (Serial Monitor) üzerinden hata ayıklayabilmek için özellikle boş bırakılmıştır. Ayrıca L298N üzerinde `ENA` ve `ENB` pinlerinin ikisi de `Timer1` (`D9` ve `D10`) üzerine atanarak iki motorun birebir aynı PWM frekansıyla ($490\text{ Hz}$) sürülmesi sağlanmıştır.

---

## 4. Modül Bazlı Fiziksel Montaj ve Kablolama Rehberi

Aracı şasi üzerine toplarken her modül için izlemeniz gereken bağlantı adımları aşağıdadır:

### 1. L298N ve Sarı TT Motor Bağlantıları
* Sol sarı motorun iki kablosunu L298N üzerindeki **Output A** klemenslerine, sağ sarı motorun iki kablosunu ise **Output B** klemenslerine vidalayın. *(İlk testte ileri komutu verildiğinde tekerleklerden biri geriye dönerse, o motorun Output klemensindeki iki kablosunu kendi arasında yer değiştirin).*
* L298N üzerindeki **A Enable (ENA)** ve **B Enable (ENB)** pinlerinin üzerindeki siyah kısa devre jumper'larını yerinden çıkarın. Bu pinleri dişi-erkek kablolarla sırasıyla Arduino **D9** ve **D10** pinlerine bağlayın.
* **Logic Input** kısmındaki **IN1, IN2, IN3, IN4** pinlerini sırasıyla Arduino **D4, D5, D6, D7** pinlerine takın.

### 2. H2010 Hız Sensörleri (Enkoderler)
* Şasede motorların iç tarafındaki delikli enkoder diskleri H2010'un U şeklindeki yarığının tam ortasından, kenarlara sürtünmeden dönecek şekilde sensörleri sabitleyin.
* 3 pinli konnektörde **1 numaralı pin (VCC)** Arduino $5\text{ V}$ hattına, **3 numaralı pin (GND)** ortak toprağa, ortadaki **2 numaralı pin (D0)** ise sol tekerlek için **D2**, sağ tekerlek için **D3** pinine bağlanır.

### 3. MPU6050 Jiroskop ve İvmeölçer
* Modülü şasinin ağırlık merkezine mümkün olduğunca yakın, titreşimden izole ve üzerindeki X/Y eksen okları aracın ileri/yan eksenleriyle paralel olacak şekilde düz sabitleyin.
* **VCC** pinini $5\text{ V}$ hattına (modül üzerinde dahili $3.3\text{ V}$ regülatörü bulunur), **GND** pinini toprağa, **SCL** pinini Arduino **A5 (D19)** pinine, **SDA** pinini ise Arduino **A4 (D18)** pinine bağlayın.
* **XDA, XCL, AD0** ve **INT** pinlerini boş bırakın (`AD0` boş bırakıldığında cihazın varsayılan I2C adresi `0x68` olarak sabitlenir).

### 4. HC-SR04 Ultrasonik Mesafe Sensörleri (Ön, Sol, Sağ)
* Üç sensörün de **VCC** pinlerini $5\text{ V}$ hattına, **GND** pinlerini ortak toprağa bağlayın.
* **Ön Sensör:** `Trig` $\rightarrow$ **A0**, `Echo` $\rightarrow$ **A1**
* **Sol Sensör:** `Trig` $\rightarrow$ **A2**, `Echo` $\rightarrow$ **A3**
* **Sağ Sensör:** `Trig` $\rightarrow$ **D13**, `Echo` $\rightarrow$ **D8**

### 5. HC-05 Bluetooth Modülü (Datasheet Görselindeki Hataya Dikkat!)
* **🚨 Görsel Uyarısı:** `HC-05 - Bluetooth.jpg` görselinin alt kısmındaki kırmızı oklar hatalı çizilmiştir (`Vcc` oku `RXD`'ye, `GND` oku `TXD`'ye gösterilmiştir). Bağlantıyı yaparken kırmızı okları değil, **mavi kartın (PCB) üzerindeki beyaz yazıları (`State`, `RXD`, `TXD`, `GND`, `VCC`, `Key`)** baz alın!
* Kart üzerindeki **VCC** pinini $5\text{ V}$ hattına, **GND** pinini ortak toprağa bağlayın.
* Kart üzerindeki **TXD** pinini doğrudan Arduino **D11** pinine bağlayın.
* Arduino **D12** pininden çıkan sinyali $1\text{ k}\Omega$–$2\text{ k}\Omega$ gerilim bölücüden geçirerek kart üzerindeki **RXD** pinine bağlayın; **STATE** ve **KEY** pinlerini boş bırakın.

---

## 5. Yazılım Ekibi İçin Kritik Uygulama ve Algoritma Notları

Yazılım ekibinin kodu geliştirirken donanımla tam uyumlu çalışması için dikkat etmesi gereken teknik detaylar şunlardır:

### 1. Enkoder Kesmeleri (Interrupts)
H2010 sensörleri `attachInterrupt(digitalPinToInterrupt(2), solSayacISR, RISING)` ve `attachInterrupt(digitalPinToInterrupt(3), sagSayacISR, RISING)` fonksiyonlarıyla okunmalıdır. Kesme (ISR) fonksiyonları içindeki sayaç değişkenleri derleyici optimizasyonu nedeniyle veri kaybı yaşanmaması için mutlaka `volatile unsigned long` olarak tanımlanmalıdır.

### 2. Ultrasonik Sensör Zamanlaması (60ms Kuralı)
HC-SR04 datasheet'inde belirtildiği üzere, bir sensörden gönderilen $40\text{ kHz}$ ses dalgasının yankısı sönümlenmeden diğer sensör tetiklenirse sensörler birbirinin yankısını (cross-talk) okur. Ön, sağ ve sol sensörler aynı anda değil, aralarında en az **$60\text{ ms}$** zaman bırakılarak sırayla okunmalıdır. Mesafe hesabı şu formülle yapılır:

$$\text{Mesafe (cm)} = \frac{\text{Süre (}\mu\text{s)}}{58}$$

*Not:* `pulseIn()` fonksiyonuna mutlaka `30000` mikrosaniye ($\mu\text{s}$) timeout parametresi eklenmelidir ki sensör uzak boşluğa baktığında işlemci kilitlenmesin.

### 3. Motor Yön ve Frenleme Mantığı (L298N Doğruluk Tablosu)
* **İleri Sürüş:** Sol motor için `IN1 = HIGH`, `IN2 = LOW`; Sağ motor için `IN3 = HIGH`, `IN4 = LOW` ve hız kontrolü için `analogWrite(9, solPWM)`, `analogWrite(10, sagPWM)`.
* **Kademeli Yavaşlama ve Aktif Fren:** Ön mesafe $25\text{ cm}$ altına indiğinde PWM değeri mesafeyle orantılı olarak düşürülmeli; durma noktasında (örneğin $10\text{ cm}$) motorların ataletle kaymasını önlemek için `IN1 = HIGH`, `IN2 = HIGH` (veya ikisi de `LOW`, `ENA`/`ENB = HIGH`) yapılarak elektronik aktif fren uygulanmalıdır.

### 4. Düz Gidiş ve Dönüş Kontrolü (Sensör Füzyonu)
* **Düz Gidiş:** Araç ilerlerken `hata = (solEnkoderSayisi - sagEnkoderSayisi) + (k_gyro * sapmaAcisi)` denklemiyle sağ ve sol motor PWM değerleri anlık dengelenmeli, ayrıca sağ ve sol HC-SR04 sensörleri koridor duvarlarını görüyorsa `(solMesafe - sagMesafe)` farkı da koridor merkezine hizalama için kullanılmalıdır.
* **$90^\circ$ Dönüş:** Araç durduktan sonra MPU6050 Z ekseni jiroskop verisi ($\omega_z$) zamana göre entegre edilerek ($\theta \mathrel{+}= \omega_z \cdot \Delta t$) hedef açıya (motor durma ataleti hesaba katılarak örneğin $\pm 88^\circ$) ulaşılana kadar bir motor ileri, diğer motor geri (tank dönüşü) sürülmelidir.
