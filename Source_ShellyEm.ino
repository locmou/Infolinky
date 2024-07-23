// ****************************************************
// * Client d'un Shelly Em sur voie 0 ou 1 ou triphasé*
// ****************************************************
void LectureShellyEm() {
  String S = "";
  String Shelly_Data = "";
  float Pw = 0;
  float voltage = 0;
  float pf = 0;


  // Use WiFiClient class to create TCP connections
  WiFiClient clientESP_RMS;
  byte arr[4];
  arr[0] = RMSextIP & 0xFF;          // 0x78
  arr[1] = (RMSextIP >> 8) & 0xFF;   // 0x56
  arr[2] = (RMSextIP >> 16) & 0xFF;  // 0x34
  arr[3] = (RMSextIP >> 24) & 0xFF;  // 0x12

  String host = String(arr[3]) + "." + String(arr[2]) + "." + String(arr[1]) + "." + String(arr[0]);
  if (!clientESP_RMS.connect(host.c_str(),80)) {
    Serial.println("connection to client Shelly Em failed (call from LectureShellyEm)");
    delay(200);
    WIFIbug++;
    return;
  }
  int voie = EnphaseSerial.toInt();
  int Voie = voie % 2;

  if (ShEm_comptage_appels == 1) {
    Voie = (Voie + 1) % 2;
  }
  String url = "/emeter/" + String(Voie);
  if (voie == 3) url = "/status";                         //Triphasé
  ShEm_comptage_appels = (ShEm_comptage_appels + 1) % 5;  // 1 appel sur 6 vers la deuxième voie qui ne sert pas au routeur
  clientESP_RMS.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (clientESP_RMS.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> client Shelly Em Timeout !");
      clientESP_RMS.stop();
      return;
    }
  }
  timeout = millis();
  // Lecture des données brutes distantes
  while (clientESP_RMS.available() && (millis() - timeout < 5000)) {
    Shelly_Data += clientESP_RMS.readStringUntil('\r');
  }
  int p = Shelly_Data.indexOf("{");
  Shelly_Data = Shelly_Data.substring(p);
  if (voie == 3) {  //Triphasé
    ShEm_dataBrute = "<strong>Triphasé</strong><br>" + Shelly_Data;
    p = Shelly_Data.indexOf("emeters");
    Shelly_Data = Shelly_Data.substring(p + 10);
    Pw = PfloatMax(ValJson("power", Shelly_Data));  //Phase 1
    pf = ValJson("pf", Shelly_Data);
    pf = abs(pf);
    float total_Pw = Pw;
    float total_Pva = 0;
    if (pf > 0) {
      total_Pva = abs(Pw) / pf;
    }
    float total_E_soutire = ValJson("total\"", Shelly_Data);
    float total_E_injecte = ValJson("total_returned", Shelly_Data);
    p = Shelly_Data.indexOf("}");
    Shelly_Data = Shelly_Data.substring(p + 1);
    Pw = PfloatMax(ValJson("power", Shelly_Data));  //Phase 2
    pf = ValJson("pf", Shelly_Data);
    pf = abs(pf);
    total_Pw += Pw;
    if (pf > 0) {
      total_Pva += abs(Pw) / pf;
    }
    total_E_soutire += ValJson("total\"", Shelly_Data);
    total_E_injecte += ValJson("total_returned", Shelly_Data);
    p = Shelly_Data.indexOf("}");
    Shelly_Data = Shelly_Data.substring(p + 1);
    Pw = PfloatMax(ValJson("power", Shelly_Data));  //Phase 3
    pf = ValJson("pf", Shelly_Data);
    pf = abs(pf);
    total_Pw += Pw;
    if (pf > 0) {
      total_Pva += abs(Pw) / pf;
    }
    total_E_soutire += ValJson("total\"", Shelly_Data);
    total_E_injecte += ValJson("total_returned", Shelly_Data);
    Energie_M_Soutiree = int(total_E_soutire);
    Energie_M_Injectee = int(total_E_injecte);
    if (total_Pw == 0){
      total_Pva=0;
    }
    if (total_Pw > 0) {
      PuissanceS_M = int(total_Pw);
      PuissanceI_M = 0;
      PVAS_M = int(total_Pva);
      PVAI_M = 0;
    } else {
      PuissanceS_M = 0;
      PuissanceI_M = -int(total_Pw);
      PVAI_M = int(total_Pva);
      PVAS_M = 0;
    }
  } else {  //Monophasé
    ShEm_dataBrute = "<strong>Voie : " + String(voie) + "</strong><br>" + Shelly_Data;
    Shelly_Data = Shelly_Data + ",";
    if (Shelly_Data.indexOf("true") > 0) {  // Donnée valide
      Pw = PfloatMax(ValJson("power", Shelly_Data));
      voltage = ValJson("voltage", Shelly_Data);
      pf = ValJson("pf", Shelly_Data);
      pf = abs(pf);
      if (Voie == voie) {  //voie du routeur
        if (Pw >= 0) {
          PuissanceS_M = int(Pw);
          PuissanceI_M = 0;
          if (pf > 0) {
            PVAS_M = int(Pw / pf);
          } else {
            PVAS_M=0;
          }
          PVAI_M = 0;
        } else {
          PuissanceS_M = 0;
          PuissanceI_M = -int(Pw);
          if (pf > 0) {
            PVAI_M = int(-Pw / pf);
          } else {
            PVAI_M=0;
          }
          PVAS_M = 0;
        }
        Energie_M_Soutiree = int(ValJson("total\"", Shelly_Data));
        Energie_M_Injectee = int(ValJson("total_returned", Shelly_Data));
        PowerFactor_M = pf;
        Tension_M = voltage;
      } else {  // voie secondaire
        if (Pw >= 0) {
          PuissanceS_T = int(Pw);
          PuissanceI_T = 0;
          if (pf > 0) {
            PVAS_T = int(Pw / pf);
          } else {
            PVAS_T=0;
          }
          PVAI_T = 0;
        } else {
          PuissanceS_T = 0;
          PuissanceI_T = -int(Pw);
          if (pf > 0) {
            PVAI_T = int(-Pw / pf);
          } else {
            PVAI_T=0;
          }
          PVAS_T = 0;
        }
        Energie_T_Soutiree = int(ValJson("total\"", Shelly_Data));
        Energie_T_Injectee = int(ValJson("total_returned", Shelly_Data));
        PowerFactor_T = pf;
        Tension_T = voltage;
      }
    }
  }

  esp_task_wdt_reset();  //Reset du Watchdog à chaque trame du Shelly reçue
  if (ShEm_comptage_appels > 1) EnergieActiveValide = true;
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}
