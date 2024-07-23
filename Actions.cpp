// ********************
// Gestion des Actions
// ********************
#include <Arduino.h>
#include "Actions.h"
#include "EEPROM.h"
#include <WiFiClient.h>



//Class Action
Action::Action() {
  valide = false;
}
Action::Action(int aIdx) {
  valide = true;  // si le n° de pin n'est pas valid, on ne fait rien
  Idx = aIdx;
  T_LastAction = int(millis() / 1000);
  On = false;
  Actif=0;
}


void Action::Activer(float Pw, int Heure, float Temperature, int Ltarfbin) {
  bool TemperatureOk = false;
  bool TarifOk = false;
  int Tseconde = int(millis() / 1000);
  if (Host == "localhost" && (GpioOff < 0 || GpioOff > 33 || GpioOn < 0 || GpioOn > 33)) valide = false;
  if (valide && Idx > 0 && (Tseconde - T_LastAction) >= Tempo) {
    if (Actif == 1) {
      if (Host == "localhost") {
        for (int i = 0; i < NbPeriode; i++) {
          if (Heure >= Hdeb[i] && Heure < Hfin[i]) {
            TemperatureOk = true;
            if (Temperature > -100) {
              if (Tinf[i] <= 100 && Temperature > Tinf[i]) { TemperatureOk = false; }
              if (Tsup[i] <= 100 && Temperature < Tsup[i]) { TemperatureOk = false; }
            }
            TarifOk = true;
            if (Ltarfbin>0 && (Ltarfbin&Tarif[i])==0 ) TarifOk=false;
            switch (Type[i]) {  //NO,OFF,ON,PW,Triac
              case 1:           //OFF
                digitalWrite(GpioOff, OutOff);
                On = false;
                T_LastAction = Tseconde;
                break;
              case 2:  //ON
                if (TemperatureOk && TarifOk) {
                  digitalWrite(GpioOn, OutOn);
                  On = true;
                  T_LastAction = Tseconde;
                } else {
                  digitalWrite(GpioOff, OutOff);
                  On = false;
                  T_LastAction = Tseconde;
                }
                break;
              case 3:
                if (Pw < Vmin[i] && TemperatureOk && TarifOk) {
                  digitalWrite(GpioOn, OutOn);
                  On = true;
                  T_LastAction = Tseconde;
                }
                if (Pw > Vmax[i] || !TemperatureOk || !TarifOk) {
                  digitalWrite(GpioOff, OutOff);
                  On = false;
                  T_LastAction = Tseconde;
                }
                break;
            }
          }
        }
      } else {  //Ordre distant

        for (int i = 0; i < NbPeriode; i++) {
          if (Heure >= Hdeb[i] && Heure < Hfin[i]) {
            
            TemperatureOk = true;
            if (Temperature > -100) {
              if (Tinf[i] <= 100 && Temperature > Tinf[i]) { TemperatureOk = false; }
              if (Tsup[i] <= 100 && Temperature < Tsup[i]) { TemperatureOk = false; }
            }
            TarifOk = true;
            if (Ltarfbin>0 && (Ltarfbin&Tarif[i])==0 ) TarifOk=false;
            switch (Type[i]) {  //NO,OFF,ON,PW,Triac
              case 1:           //OFF
                if (On) {
                  CallExterne(Host, OrdreOff, Port);
                  On = false;
                  T_LastAction = Tseconde;
                }
                break;
              case 2:  //ON
                if (!On && TemperatureOk && TarifOk) {
                  CallExterne(Host, OrdreOn, Port);
                  On = true;
                  T_LastAction = Tseconde;
                }
                if (On && (!TemperatureOk || !TarifOk)) {
                  CallExterne(Host, OrdreOff, Port);
                  On = false;
                  T_LastAction = Tseconde;
                }
                break;
              case 3:
                if (Pw < Vmin[i] && TemperatureOk && TarifOk && !On) {
                  CallExterne(Host, OrdreOn, Port);
                  On = true;
                  T_LastAction = Tseconde;
                }
                if (Pw > Vmax[i] || !TemperatureOk  || !TarifOk) {
                  if (On) {
                    CallExterne(Host, OrdreOff, Port);
                    On = false;
                    T_LastAction = Tseconde;
                  }
                }
                if ((Tseconde - T_LastAction) > Repet && Repet != 0) {  //Repetion ancien ordre
                  if (On) {
                    CallExterne(Host, OrdreOn, Port);
                  } else {
                    CallExterne(Host, OrdreOff, Port);
                  }
                  T_LastAction = Tseconde;
                }
                break;
            }
          }
        }
      }
    } else {
      if (Host == "localhost") {
        digitalWrite(GpioOff, OutOff);
      } else {
        if (On) {
          CallExterne(Host, OrdreOff, Port);
        }
      }
      On = false;
    }
  }
}


void Action::Definir(String ligne) {
  valide = true;
  Serial.println(ligne);
  String RS = String((char)30);  //Record Separator
  Actif = byte(ligne.substring(0, ligne.indexOf(RS)).toInt());
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  Titre = ligne.substring(0, ligne.indexOf(RS));
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  Host = ligne.substring(0, ligne.indexOf(RS));
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  Port = ligne.substring(0, ligne.indexOf(RS)).toInt();
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  OrdreOn = ligne.substring(0, ligne.indexOf(RS));
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  OrdreOff = ligne.substring(0, ligne.indexOf(RS));
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  Repet = ligne.substring(0, ligne.indexOf(RS)).toInt();
  Repet = min(Repet, 32000);
  Repet = max(0, Repet);
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  Tempo = ligne.substring(0, ligne.indexOf(RS)).toInt();
  Tempo = min(Tempo, 32000);
  Tempo = max(0, Tempo);
  if (Repet > 0) {
    Repet = max(Tempo + 4, Repet);  //Pour eviter conflit
  }
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  NbPeriode = byte(ligne.substring(0, ligne.indexOf(RS)).toInt());
  ligne = ligne.substring(ligne.indexOf(RS) + 1);
  int Hdeb_ = 0;
  for (byte i = 0; i < NbPeriode; i++) {
    Type[i] = byte(ligne.substring(0, ligne.indexOf(RS)).toInt());  //NO,OFF,ON,PW,Triac
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
    Hfin[i] = ligne.substring(0, ligne.indexOf(RS)).toInt();
    Hdeb[i] = Hdeb_;
    Hdeb_ = Hfin[i];
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
    Vmin[i] = ligne.substring(0, ligne.indexOf(RS)).toInt();
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
    Vmax[i] = ligne.substring(0, ligne.indexOf(RS)).toInt();
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
    Tinf[i] = ligne.substring(0, ligne.indexOf(RS)).toInt();
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
    Tsup[i] = ligne.substring(0, ligne.indexOf(RS)).toInt();
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
    Tarif[i] = ligne.substring(0, ligne.indexOf(RS)).toInt();
    ligne = ligne.substring(ligne.indexOf(RS) + 1);
  }
}
String Action::Lire() {
  String GS = String((char)29);  //Group Separator
  String RS = String((char)30);  //Record Separator
  String S;
  S += String(Actif) + RS;
  S += Titre + RS;
  S += Host + RS;
  S += String(Port) + RS;
  S += OrdreOn + RS;
  S += OrdreOff + RS;
  S += String(Repet) + RS;
  S += String(Tempo) + RS;
  S += String(NbPeriode) + RS;
  for (byte i = 0; i < NbPeriode; i++) {
    S += String(Type[i]) + RS;
    S += String(Hfin[i]) + RS;
    S += String(Vmin[i]) + RS;
    S += String(Vmax[i]) + RS;
    S += String(Tinf[i]) + RS;
    S += String(Tsup[i]) + RS;
    S += String(Tarif[i]) + RS;
  }
  return S + GS;
}



byte Action::TypeEnCours(int Heure) {  //Retourne type d'action  active à cette heure
  byte S = 0;
  for (int i = 0; i < NbPeriode; i++) {
    if (Heure >= Hdeb[i] && Heure <= Hfin[i]) S = Type[i];
  }
  return S;
}
byte Action::TypeEnCoursTriac(int Heure, float Temperature, int Ltarfbin) {  //Retourne type d'action  active à cette heure et test temperature OK
  byte S = 0;
  bool TemperatureOk;
  bool TarifOk;
  for (int i = 0; i < NbPeriode; i++) {    
    TemperatureOk = true;
    if (Temperature > -100) {
      if (Tinf[i] <= 100 && Temperature > Tinf[i]) { TemperatureOk = false; }
      if (Tsup[i] <= 100 && Temperature < Tsup[i]) { TemperatureOk = false; }
    }
    TarifOk = true;
    if (Ltarfbin>0 && (Ltarfbin&Tarif[i])==0 ) TarifOk=false;
    if (Heure >= Hdeb[i] && Heure <= Hfin[i] && TemperatureOk && TarifOk) S = Type[i];
  }
  return S;
}
int Action::Valmin(int Heure) {  //Retourne la valeur Vmin (ex seuil Triac) à cette heure
  int S = 0;
  for (int i = 0; i < NbPeriode; i++) {
    if (Heure >= Hdeb[i] && Heure <= Hfin[i]) {
      S = Vmin[i];
      if (Type[i] == 2) S = 32000;  //Explicitement ON. Force Routage
    }
  }
  return S;
}
int Action::Valmax(int Heure) {  //Retourne la valeur Vmax (ex ouverture du Triac) à cette heure
  int S = 0;
  for (int i = 0; i < NbPeriode; i++) {
    if (Heure >= Hdeb[i] && Heure <= Hfin[i]) {
      S = Vmax[i];
      if (Type[i] == 2) S = 100;  //Explicitement ON. Ouverture 100%
    }
  }
  return S;
}

void Action::InitGpio() {  //Initialise les sorties GPIO pour des relais
  int p;
  int q;
  String S;

  if (Host == "localhost" && Idx > 0) {

    p = OrdreOn.indexOf("gpio=");
    if (p >= 0) {
      S = OrdreOn.substring(p + 5);
      q = S.indexOf("&");
      if (q == -1) q = 2;
      GpioOn = S.substring(0, q).toInt();
      pinMode(GpioOn, OUTPUT);
      OutOn = 1 + OrdreOn.indexOf("out=1");
      OutOn = min(OutOn, 1);
      if (OrdreOff.indexOf("init") >= 0) On = true;
      if (OrdreOn.indexOf("init=0") >= 0) digitalWrite(GpioOn, 0);
      if (OrdreOn.indexOf("init=1") >= 0) digitalWrite(GpioOn, 1);
    }
    p = OrdreOff.indexOf("gpio=");
    if (p >= 0) {
      S = OrdreOff.substring(p + 5);
      q = S.indexOf("&");
      if (q == -1) q = 2;
      GpioOff = S.substring(0, q).toInt();
      pinMode(GpioOff, OUTPUT);
      OutOff = 1 + OrdreOff.indexOf("out=1");
      OutOff = min(OutOff, 1);
      if (OrdreOff.indexOf("init") >= 0) On = false;
      if (OrdreOff.indexOf("init=0") >= 0) digitalWrite(GpioOff, 0);
      if (OrdreOff.indexOf("init=1") >= 0) digitalWrite(GpioOff, 1);
    }
    valide = true;
    if (GpioOff < 0 || GpioOff > 33 || GpioOn < 0 || GpioOn > 33) valide = false;
  }
}
void Action::CallExterne(String host, String url, int port) {
  // Use WiFiClient class to create TCP connections
  WiFiClient clientExt;
  char hostbuf[host.length() + 1];
  host.toCharArray(hostbuf, host.length() + 1);

  if (!clientExt.connect(hostbuf, port)) {
    Serial.println("connection to clientExt failed :" + host);
    return;
  }
  clientExt.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (clientExt.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> clientESP_Ext Timeout ! : " + host);
      clientExt.stop();
      return;
    }
  }

  // Read all the lines of the reply from server
  while (clientExt.available()) {
    String line = clientExt.readStringUntil('\r');
  }
}