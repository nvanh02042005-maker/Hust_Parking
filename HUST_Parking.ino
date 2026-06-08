#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ==========================================
// CẤU HÌNH CHÂN I2C CHO ESP32-S3
// ==========================================
#define I2C_SDA 18
#define I2C_SCL 17

// ==========================================
// CẤU HÌNH WIFI & MQTT
// ==========================================
const char* ssid = "vanhtroller";
const char* password = "12345678";

const char* mqtt_server = "broker.emqx.io"; 
const int mqtt_port = 1883;
const char* mqtt_topic = "hust_parking/d8/vehicle_count";

WiFiClient espClient;
PubSubClient client(espClient);
BH1750 lightMeter;

// ==========================================
// CẤU HÌNH NGƯỠNG ÁNH SÁNG & BIẾN ĐẾM
// ==========================================
int choTrong = 150;                  
float LUX_THRESHOLD = 200.0; // Ngưỡng ánh sáng (cần tinh chỉnh thực tế)
bool isBlocked = false;      // Trạng thái tia sáng (bị che hay không)

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Đang kết nối WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi kết nối thành công!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối MQTT Broker...");
    String clientId = "ESP32S3-BH1750-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Thành công!");
    } else {
      Serial.print("Thất bại, mã lỗi: ");
      Serial.print(client.state());
      Serial.println(" Thử lại sau 5 giây...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo I2C với các chân cụ thể của ESP32-S3
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (lightMeter.begin()) {
    Serial.println(F("Khởi tạo BH1750 thành công!"));
  } else {
    Serial.println(F("Lỗi: Không tìm thấy cảm biến BH1750. Vui lòng kiểm tra dây nối."));
  }
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc giá trị cường độ ánh sáng (Lux)
  float lux = lightMeter.readLightLevel();
  
  // Mở comment dòng dưới đây khi cần đo đạc để tìm giá trị LUX_THRESHOLD chuẩn
  // Serial.print("Ánh sáng đo được: "); Serial.print(lux); Serial.println(" lx");

  // Xử lý logic đếm xe
  if (lux < LUX_THRESHOLD) {
    // Nếu ánh sáng tụt dưới ngưỡng và trước đó chưa bị che
    if (!isBlocked) { 
      isBlocked = true; 
      
      choTrong--; 
      if (choTrong < 0) choTrong = 0; 
      
      Serial.println("\n[SỰ KIỆN] Có phương tiện đi qua cổng!");
      
      // Tạo chuỗi JSON
      String payload = "{\"bai_xe\": \"D8\", \"cho_trong\": " + String(choTrong) + ", \"trang_thai\": \"Binh thuong\"}";
      client.publish(mqtt_topic, payload.c_str());
      
      Serial.print("Đã đẩy dữ liệu lên server: ");
      Serial.println(payload);
      
      // Thời gian trễ để chống dội tín hiệu (tránh đếm 1 xe thành nhiều xe)
      delay(2000); 
    }
  } else {
    // Khi xe đã đi qua, ánh sáng trở lại bình thường
    if (isBlocked) {
      isBlocked = false; 
    }
  }
  
  delay(100); 
}