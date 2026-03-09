/*
  Arduino Uno R4 WiFi: Web Server per Matrice LED 8x12

  Questo sketch trasforma l'Arduino Uno R4 WiFi in un punto di accesso (AP)
  Wi-Fi e avvia un server web sulla porta 80. La pagina web fornita permette
  di accendere e spegnere individualmente ogni singolo LED di una matrice 8x12.

  Ogni click su un "LED" nella pagina web invierà una richiesta HTTP al server
  Arduino, che aggiornerà lo stato del LED corrispondente nella memoria e lo
  renderizzerà sulla matrice (se connessa).

  Istruzioni:
  1.  Modifica SSID e PASSWORD qui sotto.
  2.  Carica lo sketch sull'Arduino Uno R4 WiFi.
  3.  Apri il Monitor Seriale per vedere l'indirizzo IP.
  4.  Connetti un dispositivo (PC/smartphone) alla rete Wi-Fi creata dall'Arduino.
  5.  Apri un browser web e vai all'indirizzo IP mostrato nel Monitor Seriale
      (es. http://192.168.4.1/).
*/

#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h" // Assicurati di avere questa libreria installata

// Configurazione del punto di accesso (AP) WiFi
char ssid[] = "arduino_wifi";         // nome della tua rete SSID (nome), deve essere di almeno 8 caratteri
char pass[] = "matteobello";         // password della tua rete (usa per WPA, o usa come chiave per WEP)
                                  // deve essere di almeno 8 caratteri 
int keyIndex = 0;                 // numero indice della tua chiave di rete (necessario solo per WEP)

// Oggetto per la matrice LED
ArduinoLEDMatrix matrix;

// Array per memorizzare lo stato di ogni LED (0 = spento, 1 = acceso)
// Dimensione della matrice: 8 righe x 12 colonne
byte frame[8][12];

// Variabili per il server web
int status = WL_IDLE_STATUS;
WiFiServer server(80); // Il server ascolta sulla porta HTTP standard (80)

// Funzione per stampare lo stato del WiFi sul Monitor Seriale
void printWiFiStatus();

void setup() {
  // Inizializza la comunicazione seriale
  Serial.begin(9600);
  while (!Serial) {
    ; // Attende che la porta seriale si connetta (necessario per schede con USB nativa)
  }
  Serial.println("Avvio Web Server per Matrice LED");

  // Inizializza tutti i LED della matrice a spento
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 12; c++) {
      frame[r][c] = 0; // Tutti i LED inizialmente spenti
    }
  }

  // Inizializza la matrice LED
  matrix.begin();
  // Renderizza lo stato iniziale (tutti spenti)
  matrix.renderBitmap(frame, 8, 12);

  // Controllo del modulo WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Comunicazione con il modulo WiFi fallita!");
    while (true); // Non continuare se il modulo non risponde
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Si prega di aggiornare il firmware del modulo WiFi.");
  }

  // Configura l'indirizzo IP locale dell'AP
  // Di default l'indirizzo sarà 192.168.4.1, ma lo impostiamo esplicitamente
  WiFi.config(IPAddress(192, 168, 4, 1));

  // Stampa il nome della rete (SSID) che verrà creata
  Serial.print("Creazione punto di accesso chiamato: ");
  Serial.println(ssid);

  // Crea la rete AP. La gestione delle nuove connessioni è automatica.
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creazione dell'Access Point fallita.");
    while (true); // Non continuare se l'AP non può essere creato
  }

  // Avvia il server web sulla porta 80
  server.begin();

  // Stampa lo stato del WiFi e l'indirizzo IP
  printWiFiStatus();
}

// Funzione per inviare la pagina HTML al client web
void sendHTMLPage(WiFiClient client) {
  // Intestazioni HTTP
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println(); // Linea vuota che separa le intestazioni dal contenuto

  // Inizio del contenuto HTML
  client.println("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<title>Controllo Matrice LED</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; text-align: center; margin: 20px; background-color: #f4f4f4; }");
  client.println("h1 { color: #333; }");
  client.println(".matrix-grid {");
  client.println("  display: grid;");
  client.println("  grid-template-columns: repeat(12, 35px); /* 12 colonne */");
  client.println("  gap: 5px;");
  client.println("  margin: 20px auto;");
  client.println("  width: fit-content;");
  client.println("  border: 1px solid #ccc;");
  client.println("  padding: 10px;");
  client.println("  background-color: #fff;");
  client.println("  border-radius: 8px;");
  client.println("  box-shadow: 0 4px 8px rgba(0,0,0,0.1);");
  client.println("}");
  client.println(".led-button {");
  client.println("  width: 30px;");
  client.println("  height: 30px;");
  client.println("  border: 1px solid #999;");
  client.println("  background-color: #ddd; /* Stato OFF */");
  client.println("  cursor: pointer;");
  client.println("  font-size: 10px;");
  client.println("  color: #555;");
  client.println("  display: flex;");
  client.println("  justify-content: center;");
  client.println("  align-items: center;");
  client.println("  text-decoration: none;");
  client.println("  border-radius: 50%;");
  client.println("  box-shadow: 1px 1px 2px rgba(0,0,0,0.2);");
  client.println("  transition: background-color 0.2s, box-shadow 0.2s;");
  client.println("}");
  client.println(".led-button.on {");
  client.println("  background-color: #4CAF50; /* Stato ON */");
  client.println("  border-color: #3e8e41;");
  client.println("  color: white;");
  client.println("  box-shadow: 0 0 8px #4CAF50;");
  client.println("}");
  client.println(".led-button:active {");
  client.println("  box-shadow: inset 1px 1px 2px rgba(0,0,0,0.2);");
  client.println("}");
  client.println(".reset-button {");
  client.println("  background-color: #f44336; color: white; padding: 10px 20px;");
  client.println("  border: none; border-radius: 5px; cursor: pointer; font-size: 16px;");
  client.println("  margin-top: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.2);");
  client.println("  transition: background-color 0.2s, box-shadow 0.2s;");
  client.println("}");
  client.println(".reset-button:hover { background-color: #da190b; }");
  client.println("</style></head>");
  client.println("<body><h1>Controllo Matrice LED 8x12</h1>");
  client.println("<div class=\"matrix-grid\">");

  // Genera dinamicamente i pulsanti per ogni LED della matrice
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 12; c++) {
      client.print("<a href=\"/?r="); // Link per il toggle del LED
      client.print(r);
      client.print("&c=");
      client.print(c);
      client.print("\" class=\"led-button");
      if (frame[r][c] == 1) { // Aggiunge la classe 'on' se il LED è acceso
        client.print(" on");
      }
      client.print("\">");
      // Puoi visualizzare le coordinate o un simbolo
      // client.print(r); client.print(","); client.print(c); // Per debug
      client.print("&#x25CF;"); // Simbolo di un cerchio
      client.println("</a>");
    }
  }

  client.println("</div>"); // Chiude matrix-grid
  client.println("<p><a href=\"/reset\"><button class=\"reset-button\">Spegni Tutti i LED</button></a></p>");
  client.println("</body></html>");
}

void loop() {
  // Confronta lo stato precedente del WiFi con lo stato corrente
  if (status != WiFi.status()) {
    status = WiFi.status(); // Aggiorna la variabile di stato
    if (status == WL_AP_CONNECTED) {
      Serial.println("Dispositivo connesso all'AP.");
    } else {
      Serial.println("Dispositivo disconnesso dall'AP. In attesa di nuove connessioni.");
    }
  }

  WiFiClient client = server.available(); // Ascolta i client in entrata

  if (client) { // Se un client si connette
    Serial.println("\nNuovo client TCP connesso.");
    String currentLine = ""; // Stringa per contenere i dati in arrivo dal client
    while (client.connected()) { // Esegui il loop finché il client è connesso
      // Questo delay è importante per l'Arduino Nano RP2040 Connect (e schede simili)
      // per evitare problemi con la gestione SPI del modulo WiFi.
      delayMicroseconds(10);
      if (client.available()) { // Se ci sono byte da leggere dal client
        char c = client.read(); // Leggi un byte
        Serial.write(c); // Stampalo sul monitor seriale (per debugging)

        if (c == '\n') { // Fine della linea HTTP
          if (currentLine.length() == 0) { // Se la linea è vuota (doppio '\n'), è la fine della richiesta HTTP
            // La richiesta è stata completamente ricevuta, ora invia la risposta HTML aggiornata
            sendHTMLPage(client);
            break; // Esci dal ciclo, la risposta è stata inviata
          } else { // Se la linea non è vuota, processala e poi pulisci
            // Controlla se la richiesta del client era per il toggle di un LED:
            if (currentLine.startsWith("GET /?r=")) {
              // Estrai i valori di riga (r) e colonna (c) dalla stringa della richiesta
              int r_start_idx = currentLine.indexOf("r=") + 2;
              int r_end_idx = currentLine.indexOf("&c=");
              int c_start_idx = r_end_idx + 3;
              int c_end_idx = currentLine.indexOf(" ", c_start_idx); // Trova lo spazio dopo il valore della colonna

              if (r_start_idx != -1 && r_end_idx != -1 && c_start_idx != -1 && c_end_idx != -1) {
                String r_str = currentLine.substring(r_start_idx, r_end_idx);
                String c_str = currentLine.substring(c_start_idx, c_end_idx);

                int row = r_str.toInt();
                int col = c_str.toInt();

                // Verifica che le coordinate siano valide per la matrice 8x12
                if (row >= 0 && row < 8 && col >= 0 && col < 12) {
                  frame[row][col] = !frame[row][col]; // Inverti lo stato del LED (toggle)
                  matrix.renderBitmap(frame, 8, 12); // Aggiorna la matrice fisica
                  Serial.print("LED toggled: [");
                  Serial.print(row);
                  Serial.print("][");
                  Serial.print(col);
                  Serial.println("]");
                }
              }
            } else if (currentLine.startsWith("GET /reset")) {
              // Se la richiesta è per resettare tutti i LED
              for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 12; j++) {
                  frame[i][j] = 0; // Spegni tutti i LED
                }
              }
              matrix.renderBitmap(frame, 8, 12); // Aggiorna la matrice fisica
              Serial.println("Tutti i LED sono stati spenti.");
            }
            currentLine = ""; // Pulisci la linea corrente per la prossima riga della richiesta
          }
        } else if (c != '\r') { // Se non è un ritorno a capo (che va scartato), aggiungilo alla stringa
          currentLine += c;
        }
      }
    }
    // Chiudi la connessione TCP
    client.stop();
    Serial.println("Client TCP disconnesso.");
  }
}

// Implementazione della funzione printWiFiStatus
void printWiFiStatus() {
  Serial.print("SSID AP: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("Indirizzo IP AP: ");
  Serial.println(ip);

  Serial.print("Per controllare la matrice, apri un browser e vai su: http://");
  Serial.println(ip);
}
