// =====================================================================
//  TORPEDINO – DIN KODE
//
//  Legg all egenkode i userSetup() og userLoop() nedenfor.
//  WiFi, OTA og webserver håndteres automatisk av core.ino.
//
//  Nyttige globale variabler fra core.ino:
//    bool apMode   – true hvis enheten kjører som hotspot
//    WebServer server – legg til egne ruter med server.on(...)
// =====================================================================

void userSetup() {
    // Kjøres én gang etter at WiFi og webserver er klare.
    // Eksempel: pinMode(LED_BUILTIN, OUTPUT);
}

void userLoop() {
    // Kjøres kontinuerlig i hovedløkken.
    // Eksempel: digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
