//CO2 sensörü kütüphaneleri
// AN-126 Demo of K-30 using Software Serial
#include <SoftwareSerial.h>
/*
 Basic Arduino example for K-Series sensor
 Created by Jason Berger
 Co2meter.com
*/
#include "SoftwareSerial.h"

// touch-screen kütüphaneleri!
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#define Arial_24 &FreeSans9pt7b
#define Arial_48 &FreeSans12pt7b
#define Arial_60 &FreeSans18pt7b
#define ILI9341_BLACK  0x0000
#define ILI9341_RED    0xF800
#define ILI9341_GREEN  0x07E0
#define ILI9341_YELLOW 0xFFE0
#define CS_PIN  4  // Waveshare Touch - Dokunmatik ekran iletişimi için pin
#define TFT_DC  7  // Waveshare - Ekran ile Kod arasındaki iletişim
#define TFT_CS 10  // Waveshare - Ekran ile Kod arasındaki iletişim
#define TFT_BL  9   // Waveshare backlight - Ekran ile Kod arasındaki iletişim
// MOSI=11, MISO=12, SCK=13
#define TIRQ_PIN  2

XPT2046_Touchscreen ts(CS_PIN);
Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_BL);

//Malzeme listesi
const char* items[] = { "Yumurta", "Pekmez", "Sut", "Zeytin", "Tavuk", "Yogurt", "Peynir", "Ispanak", "Cilek", "Tire Sutu" };

//Normal aralık listesi
const char* normalRanges[] = { "40-50 mg/dL", "100-200 mg/dL", "70-100 mg/dL", "40-60 mg/dL", "50-60 mg/dL", "90-150 mg/dL", "30-50 mg/dL", "20-30 mg/dL", "30-40 mg/dL", "60-80 mg/dL" };

//Anlık item indeksi
int currentItemIndex = 0; //ilk indeksi belirttik

//İtem Listesi için bayrak koyalım
bool itemListShown = true;

//koordinat hesaplayacağız.
int startX, startY, endX, endY;

//Buton boyutlarını ayarlayalım
int buttonWidth = 60;
int buttonHeight = 40;

//Buton koordinatlarını belirtelim
int nextButtonX, nextButtonY;
int backButtonX, backButtonY;

//CO2 sensörü için gerekli tanımlamaları yapalım. 
SoftwareSerial K_30_Serial(11,12); //Sets up a virtual serial port - TX ve RX pinleridir. boş kalan herhangi bir dijital pine yerleştirilebilirler.
//SoftwareSerial K_30_Serial(12,13); //Sets up a virtual serial port
 //Using pin 12 for Rx and pin 13 for Tx
byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25}; //Command packet to read Co2 (see app note) - CO2 i okumak için gerekli olan ama anlamadığım değerler
byte response[] = {0,0,0,0,0,0,0}; //create an array to store the response - sensöre bunun ile istek atacağız
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;



void setup() {
  Serial.begin(9600);
  tft.init(240, 320, SPI_MODE0); //TFT ekranı SPI_MODE0 protokolü ile başlattık
  tft.setRotation(1); //ekranı 90 derece döndürdük istediğimiz yönde kullanabiliriz.
  tft.fillScreen(ILI9341_BLACK); //TFT ekranın arka planını full siyah yaptık
  ts.begin(); //TFT Ekrandan ayrıyetten dokunmatiğini de aktif ettik.
  ts.setRotation(1); //TFT Ekrandan ayrıyetten dokunmatiğini de istediğimiz yönde kullanabiliriz
  while (!Serial && (millis() <= 1000)); //Ekranın seri haberleşmesi döngüsünü başlattık.  1 saniyede kendini yenileyecek.
  
  //Buton koordinatlarını belirttik.
  nextButtonX = tft.width() - buttonWidth;
  nextButtonY = tft.height() - buttonHeight;
  backButtonX = 0;
  backButtonY = tft.height() - buttonHeight;

  //CO2 sensörü için gerekli setupları burada kuralım. -Bu kod ile aşağıda Serial Monitor de görüntüleyebileceğiz.
  K_30_Serial.begin(9600); //Opens the virtual serial port with a baud of 9600
  Serial.println(" Demo of AN-126 Software Serial and K-40 Sensor");
}

//Co2 sensörün çalışması için bazı fonksiyon tanımlamaları yapalım. DİKKAT! CO2 SENSÖRÜ KILAVUZU OKUMALISIN DOĞRU SONUÇ İÇİN
void sendRequest(byte packet[])
{
 while(!K_30_Serial.available()) //keep sending request until we start to get a response
 {
  Serial.println("waiting for Software.serial port availability");
 K_30_Serial.write(readCO2,7);
 delay(3000); 
 }
 int timeout=0; //set a timeout counter
 while(K_30_Serial.available() < 7 ) //Wait to get a 7 byte response
 {
 timeout++;
 if(timeout > 10) //if it takes too long there was probably an error
 {
 while(K_30_Serial.available()) //flush whatever we have
 K_30_Serial.read();
 break; //exit and try again
 }
 delay(50);
 }
 for (int i=0; i < 7; i++)
 {
 response[i] = K_30_Serial.read();
 }
}
unsigned long getValue(byte packet[])
{
 int high = packet[3]; //high byte for value is 4th byte in packet in the packet
 int low = packet[4]; //low byte for value is 5th byte in the packet
 unsigned long val = high*256 + low; //Combine high byte and low byte with this formula to get value
 return val* valMultiplier;
}


void loop() {
  // Check if the screen is touched
  if (ts.touched()) { //dokunuş algılarsa if'in içine gir.
    // Get touch coordinates
    TS_Point p = ts.getPoint(); //TSPoint fonksiyonu için p değişkeni atadım. dokunulan noktanın koordinatını aldım.
    startX = p.x; //Dokununca ilk x kooordinatını aldık
    startY = p.y; //Dokununca ilk y koordinatını aldık.
    
    // Wait for the touch to be released before processing
    while (ts.touched()) { //eğer dokunmatik ekrana dokunmaya devam edersem, elimi nereye kaydırırsam orayı algılasın diye döngüye alarak son x ve y koordinatlarını alıyorum.
      p = ts.getPoint(); //yeniden p için değerler aldık
      endX = p.x; //son koordinatların x ve y sini hesapladık
      endY = p.y;
    }

    // Determine if a swipe gesture occurred (horizontal swipe)
    int deltaX = endX - startX; // yatay yöndeki değişimi hesapladık. 
    int deltaY = endY - startY; //dikey yöndeki değişimi hesapladık.

    if (abs(deltaX) > abs(deltaY)) {  //yatay yöndeki değişim > dikey yöndeki değişim ise if içerisine gir.
      
      if (deltaX > 0) { // yatay kaydırma pozitif yönde ise bu koşula gir.
        
        currentItemIndex = (currentItemIndex + 1) % 10; // eğer sağa kaydırırsan kaçıncı itemde olursan ol bir sonraki iteme atacak. Aslında buton varken buna gerek yok ama olsun, iki adet olsun, bizim olsun.
      } else { // yatay kaydırma negatif yönde ise bu koşula gir.
        
        currentItemIndex = (currentItemIndex - 1 + 10) % 10; // Rotate between 0-9 //yukarıda 8'den dokuza gidiyorsak burada 9'dan 8 e gideriz
      }
    }

    itemListShown = false; //liste bayrağını işaretledik, listeyi gösterme dedik.
  }

  if (itemListShown) { //Liste görünürken ekran ayarlarını yapalım. - true yazmıyorum çünkü zaten yukarıda true olarak varsayılan bool atadık.
    // Display the item list
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setFont(Arial_24); //Font ve renkler ihtiyaca göre rahatlıkla değiştirilebilir. Ama arka plan rengin dikkat edilmeli yoksa yazılar görünmez.

    for (int i = 0; i < 10; i++) {
      tft.setCursor(120, 10 + i * 25);
      tft.print(items[i]);
    }

//İleri ve geri butonlarının yerini belirlemek için aşağıdaki kodlar kullanılır. Aslında yukarıdaki fonksiyon sonrası gereksiz ama olsun yine de bulunsun. istenilirse yorum satırı yapılır.
    tft.fillRect(nextButtonX, nextButtonY, buttonWidth, buttonHeight, ILI9341_GREEN);
    tft.setTextColor(ILI9341_BLACK);
    tft.setFont(Arial_24);
    tft.setCursor(nextButtonX + 10, nextButtonY + 10);
    tft.print(">");
    
    // Draw "Geri" butonunu sol alt köşede çiz
    tft.fillRect(backButtonX, backButtonY, buttonWidth, buttonHeight, ILI9341_GREEN); //ön yüzünü ayarladık.
    tft.setTextColor(ILI9341_BLACK);
    tft.setFont(Arial_24);
    tft.setCursor(backButtonX + 10, backButtonY + 10); //düğme içine yazılacak metnin nereye konumlanacağına buradan karar vereyim
    tft.print("<"); //şeklini belirttim. Şekil değiştirilebilir. 
  } else { //Liste ekranı değilde Normal Aralığın yazılı olduğu ekrana gelince ekranın nasıl olacağını belirleyelim.
    
    tft.fillScreen(ILI9341_BLACK);
    
    tft.setTextColor(ILI9341_YELLOW);
    tft.setFont(Arial_60);
    tft.setCursor(60, 80);
    tft.print(items[currentItemIndex]); //hangi index onu yazalım önce. Gereksiz ama zevk işte.

    tft.setTextColor(ILI9341_GREEN);
    tft.setFont(Arial_24);
    tft.setCursor(60, 150);
    tft.print("Normal aralik: ");
    tft.print(normalRanges[currentItemIndex]);


    //Co2 için ayarlama
    tft.setTextColor(ILI9341_RED);
    tft.setFont(Arial_24);
    tft.setCursor(60, 100);
    tft.print("30 saniye bekle");

    sendRequest(readCO2);
    unsigned long valCO2_atmosphere = getValue(response);
    Serial.print("Atmosphere's Co2 ppm = ");
    Serial.println(valCO2_atmosphere);
    delay(5000);
    sendRequest(readCO2);
    unsigned long valCO2_food = getValue(response);
    Serial.print("Food's CO2 ppm =  ");
    Serial.println(valCO2_food);
    delay(15000);
    Serial.print("Bozulma durumu ppm farkı= ");
    Serial.println(valCO2_food - valCO2_atmosphere);

    // Ekstra yazıyı ekleyin
    tft.setCursor(60, 200);
    tft.setTextColor(ILI9341_RED); // Renk seçimi
    tft.setFont(Arial_24);
    tft.print("Olculen ppm farkı:  ");
    tft.print(valCO2_food - valCO2_atmosphere);
    // Burada ölçülen değeri eklemek için uygun bir değeri tft.print ile yazdırralım.
  }
  delay(200);
}







