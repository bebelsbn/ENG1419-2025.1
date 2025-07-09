#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <JKSButton.h>
#include <EEPROM.h>
#define TAM_BLOCO 12
#define MATRIZ_TAM 10


#include <FastLED.h>
# define NUM_LEDS 86
# define DATA_PIN 32
# define TIPO WS2811
# define BRIGHTNESS 10
# define COLUNA 8
# define LINHA 8

#define ENDERECO_REC_NIVEL0 0
#define ENDERECO_REC_NIVEL1 (ENDERECO_REC_NIVEL0 + sizeof(int))
#define ENDERECO_REC_NIVEL2 (ENDERECO_REC_NIVEL1 + sizeof(int))
#define ENDERECO_REC_NIVEL3 (ENDERECO_REC_NIVEL2 + sizeof(int))


/*cores como variaveis */
#define CIANO    CRGB(255, 0, 255);
#define LARANJA  CRGB(85, 255, 0);
#define ROSA     CRGB(0, 255, 255);
#define VERMELHO CRGB(0, 255, 0);
#define AMARELO  CRGB(255, 255, 0);
#define AZUL     CRGB(0, 0, 255);
#define PRETO    CRGB(0, 0, 0);
#define BRANCO   CRGB(255, 255, 255);

const int LED_PIN = 49;  

typedef struct celula Lab;
typedef struct inimigo Fantasma;


struct celula{
  int ind; 
  int cor; 
  bool ocupado; 
};
struct inimigo{
  int coluna, linha;
  int cor;
};


MCUFRIEND_kbv tela;
TouchScreen touch(6, A1, A2, 7, 300);


JKSButton botaoStart, botaoNivel;
JKSButton botaoReset, botaoMenu;


enum EstadoTela { MENU, JOGO, FIM };
EstadoTela estadoAtual = MENU;

int ipacman;
int jpacman;

int backupFantasmas[MATRIZ_TAM][MATRIZ_TAM];


CRGB leds[NUM_LEDS];

Lab labirinto[LINHA][COLUNA];
Lab labirintofant[LINHA][COLUNA];
int labirintods[MATRIZ_TAM][MATRIZ_TAM];



Fantasma f1;
Fantasma f2;
Fantasma f3;
Fantasma f4;


/* CÓDIGO CORES
  CRGB(255, 0, 0);     VERDE
  CRGB(0, 255, 0);     VERMELHO
  CRGB(0, 0, 255);     AZUL     -> USAR COMO O BONUS
  CRGB(255, 255, 0);   AMARELO  -> USAR PARA O JOGO
  CRGB(255, 0, 255);   CIANO    -> USAR COMO O FANTASMA
  CRGB(0, 255, 255);   ROSA     -> USAR COMO O FANTASMA
  CRGB(255, 255, 255); BRANCO
*/



bool jogo = false;

int nivel = 0;
int pontos = 0;
int tempo = 0;
bool poderAtivo = false;
unsigned long tempoPoder = 0;
unsigned long inicio;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);


  tela.begin(tela.readID());
  tela.setRotation(1);


  exibirMenuInicial();
  mostrarRecordeNivel();


  FastLED.addLeds<TIPO, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(LED_PIN, OUTPUT);  // configura como saída
  digitalWrite(LED_PIN, LOW);

  //EEPROM.put(ENDERECO_REC_NIVEL0, 0);
  //EEPROM.put(ENDERECO_REC_NIVEL1, 0);
  //EEPROM.put(ENDERECO_REC_NIVEL2, 0);
  //EEPROM.put(ENDERECO_REC_NIVEL3, 0);

  int indice = 1;
  for (int i = 0; i < LINHA; i++) {
    if (i % 2 == 0) {
        for (int j = COLUNA-1; j >=0; j--) {
            labirinto[i][j] = {indice, 0, false};
            labirintofant[i][j] = {indice, 0, false};
            colore(&labirinto[i][j]);
            indice++;
        }
    } else {
        for (int j = 0; j < COLUNA; j++) {
            labirinto[i][j] = {indice, 0, false};
            labirintofant[i][j] = {indice, 0, false};
            colore(&labirinto[i][j]);
            indice++;
        }
    }
    indice += 3;
  }
  FastLED.show();
  delay(2000);
}

void loop() {
  if (estadoAtual == MENU) {
    botaoStart.process();
    botaoNivel.process();
  } 
  else if (estadoAtual == JOGO) {
    botaoReset.process();
    botaoMenu.process();

    static unsigned long ultimaAtualizacao = 0;
    if (millis() - ultimaAtualizacao >= 1000) {
      ultimaAtualizacao = millis();
      if (jogo && tempo >= 0) {
        tempo++;
        exibirStatus();
        desenharTFTdoLED();  
        desenhaPacMan(ipacman,jpacman);
      }
    }

    static unsigned long ultimaMovimentacao = 0;
    if (millis() - ultimaMovimentacao >= 2000) {
      ultimaMovimentacao = millis();

      if (jogo) {

        if (nivel >= 1) {
          moverFantasma(f1);
          moverFantasma(f2);
        }
        if (nivel >= 2) moverFantasma(f3);
        if (nivel >= 3) moverFantasma(f4);


      }
    }
  } 
  else if (estadoAtual == FIM) {
    botaoMenu.process();
  }

  if (Serial.available()) {
    String texto = Serial.readStringUntil('\n');
    texto.trim();

    if (texto.startsWith("Bolinha:")) {
      int iVirgula = texto.indexOf(',');
      if (iVirgula > 0) {
        String iStr = texto.substring(8, iVirgula);
        String jStr = texto.substring(iVirgula + 1);
        int i = iStr.toInt() + 1;
        int j = jStr.toInt() + 1;

        ipacman = i;
        jpacman = j;

        Serial.print("PAC MAN: ");
        Serial.print(ipacman);
        Serial.print(" ");
        Serial.println(jpacman);

        ocupado(&labirinto[i - 1][j - 1]);

        if (i >= 0 && i < MATRIZ_TAM && j >= 0 && j < MATRIZ_TAM) {
          int valorAtual = labirintods[i][j];

          if (i == 0 || i == MATRIZ_TAM - 1 || j == 0 || j == MATRIZ_TAM - 1) {
            Serial.print("Posição bloqueada (");
            Serial.print(i); Serial.print(", "); Serial.print(j);
            Serial.println(") - está na borda da matriz.");
            return;
          }


          switch (valorAtual) {
            case 0:
              pontos += 1;
              break;

            case 3:
            case 4:
              if (poderAtivo) {

                tela.setCursor(60, 220);
                tela.setTextColor(TFT_RED);
                tela.setTextSize(2);
                tela.print("TEMPO PAUSADO!");
              } else {
                pontos = max(0, pontos - 1);
                tempo += 10;

                Serial.println("Encostou no fantasma! +10 segundos");
              }
              break;

            case 5:
              poderAtivo = true;
              tempoPoder = millis();
              pontos += 2;
              break;
          }

          Serial.print("Posição atualizada para: ");
          Serial.print(i); Serial.print(", "); Serial.println(j);
        }
      }
    } 
    else if (todosPontosComidos() || texto == "FIM") {
      jogo = false;
      estadoAtual = FIM;

      int desempenhoAtual = pontos * 100 + tempo * 10;
      int endereco = enderecoRecorde(nivel);
      int melhorDesempenho;
      EEPROM.get(endereco, melhorDesempenho);

      if (desempenhoAtual > melhorDesempenho || melhorDesempenho <= 0 || melhorDesempenho > 1000000) {
        EEPROM.put(endereco, desempenhoAtual);
        tela.setCursor(80, 110);
        tela.setTextColor(TFT_GREEN);
        tela.setTextSize(2);
        tela.print("NOVO RECORDE!");
      }

      tela.fillScreen(TFT_BLACK);
      tela.setCursor(70, 50);
      tela.setTextColor(TFT_RED);
      tela.setTextSize(2);
      tela.print("Jogo terminado!");

      tela.setCursor(100, 80);
      tela.setTextColor(TFT_YELLOW);
      tela.print("Tempo: ");
      int minutos = tempo / 60;
      int segundos = tempo % 60;
      if (minutos > 0) {
        tela.print(minutos); tela.print("m ");
        if (segundos < 10) tela.print("0");
      }
      tela.print(segundos); tela.print("s");

      botaoMenu.init(&tela, &touch, 150, 130, 100, 40, TFT_WHITE, TFT_BLUE, TFT_BLACK, "MENU", 2);
      botaoMenu.setPressHandler(voltarMenu);

      Serial.println("Recebido FIM. Jogo encerrado.");
    }
  }

  if (poderAtivo && (millis() - tempoPoder > 3000)) {
    poderAtivo = false;
    Serial.println("Poder especial acabou.");
  }
}

void desenharTFTdoLED() {
  int xInicial = (320 - TAM_BLOCO * MATRIZ_TAM) / 2;
  int yInicial = 100;

  for (int i = 1; i <= 8; i++) {
    for (int j = 1; j <= 8; j++) {
      int iLED = i - 1;
      int jLED = j - 1;

      int idx = labirinto[iLED][jLED].ind;
      CRGB corLed = leds[idx];

      uint16_t corTFT;


      if (corLed == CRGB(255, 0, 0)) {
        corTFT = TFT_GREEN;
      } else if (corLed == CRGB(0, 255, 0)) {
        corTFT = TFT_RED;
      } else if (corLed == CRGB(0, 0, 255)) {
        corTFT = TFT_BLUE;
      } else if (corLed == CRGB(255, 255, 0)) {
        corTFT = TFT_YELLOW;
      } else if (corLed == CRGB(255, 0, 255)) {
        corTFT = TFT_CYAN;
      } else if (corLed == CRGB(0, 255, 255)) {
        corTFT = TFT_PINK;
      } else if (corLed == CRGB(255, 255, 255)) {
        corTFT = TFT_WHITE;
      } else if (corLed == CRGB(85, 255, 0)) {
        corTFT = TFT_ORANGE;
      } else {
        corTFT = TFT_BLACK;
      }

      int x = j * TAM_BLOCO + xInicial;
      int y = i * TAM_BLOCO + yInicial;
      tela.fillRect(x, y, TAM_BLOCO, TAM_BLOCO, corTFT);
      tela.drawRect(x, y, TAM_BLOCO, TAM_BLOCO, TFT_BLACK);
    }
  }

}

void desenhaPacMan(int i, int j){
  labirintods[i][j] = 2;     
  desenharTFTdoLED();          
}


void exibirMenuInicial() {
  jogo = false;
  estadoAtual = MENU;
  tela.fillScreen(TFT_BLACK);
  tela.setCursor(10, 10);
  tela.setTextColor(TFT_WHITE);
  tela.setTextSize(2);
  tela.print("Menu Inicial");

  botaoStart.init(&tela, &touch, 60, 120, 100, 40, TFT_WHITE, TFT_GREEN, TFT_BLACK, "START", 2);
  botaoNivel.init(&tela, &touch, 180, 120, 100, 40, TFT_WHITE, TFT_ORANGE, TFT_BLACK, "NIVEL", 2);


  botaoStart.setPressHandler(iniciarJogo);
  botaoNivel.setPressHandler(mudarNivel);


  exibirNivel();
}


void voltarMenu(JKSButton &b) {
  Serial1.println("menu");
  resetLab(labirinto);
  Serial1.println("menu");
  digitalWrite(LED_PIN, LOW); 
  jogo = false;
  estadoAtual = MENU;
  tela.fillScreen(TFT_BLACK);
  tela.fillRect(0, 0, 320, 60, TFT_BLACK);
  pontos = 0;
  tempo = 0;
  exibirMenuInicial();
  Serial.println("Voltou ao menu.");
}


void exibirNivel() {
  tela.fillRect(10, 40, 300, 30, TFT_BLACK);
  tela.setCursor(10, 40);
  tela.setTextColor(TFT_ORANGE);
  tela.setTextSize(2);
  tela.print("Nivel atual: ");
  tela.println(nivel);
}


void iniciarJogo(JKSButton &b) {
  Serial1.println("inicia");
  Serial.println("inicia");

  digitalWrite(LED_PIN, HIGH); 

  iniciaLabirinto();
  tela.fillScreen(TFT_BLACK);
  pontos = 0;
  tempo = 0;
  inicio = millis();
  jogo = true;
  estadoAtual = JOGO;

  exibirStatus();

  if (nivel == 1) {
    tabela1();
  } else if (nivel == 2) {
    tabela2();
  } else if (nivel == 3) {
    tabela3();
  }else if(nivel == 0){
    tabela0();
  }

  desenharLabirinto();

  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      backupFantasmas[i][j] = labirintods[i][j];
    }
  }
  

  botaoReset.init(&tela, &touch, 100, 60, 100, 40, TFT_WHITE, TFT_RED, TFT_BLACK, "RESET", 2);
  botaoMenu.init(&tela, &touch, 220, 60, 100, 40, TFT_WHITE, TFT_BLUE, TFT_BLACK, "MENU", 2);

  botaoReset.setPressHandler(resetarJogo);
  botaoMenu.setPressHandler(voltarMenu);
}

void mudarNivel(JKSButton &b) {
  nivel = (nivel + 1) % 4;
  exibirNivel();
  mostrarRecordeNivel();

}

void resetarJogo(JKSButton &b) {
  
  Serial1.println("reset");
  resetLab(labirinto);
  pontos = 0;
  tempo = 0;
  inicio = millis();
  exibirStatus();
  tela.setCursor(10, 130);
  tela.setTextColor(TFT_RED);
  tela.setTextSize(2);

}

void exibirStatus() {
  tela.fillRect(10, 10, 300, 30, TFT_BLACK);
  tela.setCursor(10, 10);
  tela.setTextColor(TFT_CYAN);
  tela.setTextSize(2);
  tela.print("Pontos: ");
  tela.print(pontos);
  tela.print(" Tempo: ");

  int minutos = tempo / 60;
  int segundos = tempo % 60;

  if (minutos > 0) {
    tela.print(minutos);
    tela.print("m ");
    if (segundos < 10) tela.print("0");
  }

  tela.print(segundos);
  tela.print("s");
}

int enderecoRecorde(int nivel) {
  switch (nivel) {
    case 0 : return ENDERECO_REC_NIVEL0;
    case 1: return ENDERECO_REC_NIVEL1;
    case 2: return ENDERECO_REC_NIVEL2;
    case 3: return ENDERECO_REC_NIVEL3;
    default: return 0;
  }
}

void mostrarRecordeNivel() {
  int endereco = enderecoRecorde(nivel);
  int recorde;
  EEPROM.get(endereco, recorde);
  tela.fillRect(10, 70, 300, 30, TFT_BLACK);
  tela.setCursor(10, 70);
  tela.setTextColor(TFT_YELLOW);
  tela.setTextSize(2);
  tela.print("Recorde: ");

  if (recorde > 0 && recorde < 1000000) {
    tela.print(recorde);
  } else {
    tela.print("Nenhum");
  }
}

void desenharLabirinto() {

  int xInicial = (320 - TAM_BLOCO * MATRIZ_TAM) / 2;
  int yInicial = 100;

  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      int valor = labirintods[i][j];
      int x = j * TAM_BLOCO + xInicial;
      int y = i * TAM_BLOCO + yInicial;
      uint16_t cor;

      if ( x == 0|| x==9|| y==0 ||y==9){
        valor = 9;
      }


      switch (valor) {
        case 0: cor = TFT_YELLOW; break;
        case 2: cor = TFT_GREEN; break;  
        case 3: cor = TFT_RED; break;   
        case 4: cor = TFT_PINK; break;  
        case 5: cor = TFT_BLUE; break; 
        case 6: cor = TFT_CYAN; break;  
        case 7: cor = TFT_ORANGE; break;  
        case 9: cor = TFT_WHITE; break; 
        case 10: cor = TFT_BLACK; break; 
        default: cor = TFT_BLACK; break;
      }
      
        tela.fillRect(x, y, TAM_BLOCO, TAM_BLOCO, cor);
        tela.drawRect(x, y, TAM_BLOCO, TAM_BLOCO, TFT_BLACK);

    }
  }
}

void tabela0(){

  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }

  labirintods[1][1] = 2;  	

}


void tabela1() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }

  labirintods[1][1] = 2;  	
  f1 = {2, 1, 3};         	
  f2 = {5, 2, 4};         	
  labirintods[3][1] = 5; 
  pintaFruta(3, 1);
  labirintods[2][2] = 5; 
  pintaFruta(2, 2);
  labirintods[4][6] = 5; 
  pintaFruta(4, 6);
  labirintods[6][3] = 5; 
  pintaFruta(6, 3);

}


void tabela2() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }


  labirintods[1][1] = 2;  	

  f1 = {4, 4, 3}; 
  f2 = {3, 6, 4}; 
  f3 = {6, 6, 6}; 

  labirintods[2][2] = 5; 
  pintaFruta(2, 2);
  labirintods[6][3] = 5; 
  pintaFruta(6, 3);
  labirintods[7][7] = 5; 
  pintaFruta(7, 7);
}


void tabela3() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }

  labirintods[1][1] = 2;  	

  f1 = {2, 2, 3}; 
  f2 = {3, 3, 4}; 
  f3 = {5, 5, 6};
  f4 = {4, 6, 7};


  labirintods[6][6] = 5; 
  pintaFruta(6, 6);
  labirintods[2][5] = 5; 
  pintaFruta(2, 5);

}


bool todosPontosComidos() {
  for (int i = 0; i < LINHA; i++) {
    for (int j = 0; j < COLUNA; j++) {
      if (labirinto[i][j].cor == 1 && !labirinto[i][j].ocupado) {
        return false;
      }
    }
  }
  return true;
}


void moverFantasma(Fantasma &f) {
  int linhaAntiga = f.linha;
  int colunaAntiga = f.coluna;


  if (f.linha > 0 && f.linha < MATRIZ_TAM - 1 && f.coluna > 0 && f.coluna < MATRIZ_TAM - 1) {
    labirintods[f.linha][f.coluna] = backupFantasmas[f.linha][f.coluna];
  }

  atualizarLED(f.linha, f.coluna, labirintods[f.linha][f.coluna]);



  if (f.linha > 0 && f.linha < MATRIZ_TAM - 1 && f.coluna > 0 && f.coluna < MATRIZ_TAM - 1) {

    if (labirintods[f.linha][f.coluna] == f.cor) {
      labirintods[f.linha][f.coluna] = 0;
      atualizarLED(f.linha, f.coluna, 0);
    }
  }


  
  for (int tentativa = 0; tentativa < 4; tentativa++) {
    int direcao = random(4);
    int novaLinha = f.linha;
    int novaColuna = f.coluna;
    int novaCor = f.cor;




    if (direcao == 0 && f.linha > 1) novaLinha--;                     
    else if (direcao == 1 && f.linha < MATRIZ_TAM - 2) novaLinha++;  
    else if (direcao == 2 && f.coluna > 1) novaColuna--;              
    else if (direcao == 3 && f.coluna < MATRIZ_TAM - 2) novaColuna++; 



    int destino = labirintods[novaLinha][novaColuna];
    if (destino != 9 && destino != 5 && destino != 2 && destino != 4 && destino != 6 && destino != 7) {
      f.linha = novaLinha;
      f.coluna = novaColuna;
      break;
    }


  }

  backupFantasmas[f.linha][f.coluna] = labirintods[f.linha][f.coluna];


  labirintods[f.linha][f.coluna] = f.cor;
  atualizarLED(f.linha, f.coluna, f.cor);
  enviaPosicaoFantasma(f);


  if (f.linha != linhaAntiga || f.coluna != colunaAntiga) {
    desenhaLabirinto();
    pintarFantasma(f, linhaAntiga, colunaAntiga);
  }
}


void atualizarLED(int i, int j, int valor) {
  if (i <= 0 || i >= MATRIZ_TAM - 1 || j <= 0 || j >= MATRIZ_TAM - 1) {
    return;
  }

  String cor;
  switch (valor) {
    case 0: cor = "amarelo"; break;
    case 2: cor = "verde"; break;
    case 3: cor = "vermelho"; break;
    case 4: cor = "rosa"; break;
    case 5: cor = "azul"; break;
    case 10: cor = "preto"; break;
    case 6: cor = "roxa"; break;
    case 7: cor = "laranja"; break;
    default: return;
  }
  // Envio correto para a matriz física 8x8
  int iLED = i - 1;
  int jLED = j - 1;
 
  Serial.print("LED:");
  Serial.print(iLED);
  Serial.print(",");
  Serial.print(jLED);
  Serial.print(",");
  Serial.println(cor);
}


void enviaPosicaoFantasma(Fantasma f) {
  Serial.print("Fantasma: ");
  Serial.print(f.linha);
  Serial.print(",");
  Serial.print(f.coluna);
  Serial.print(",");
  Serial.println(f.cor);




}


void enviarEstadoInicial() {
  for (int i = 1; i <= LINHA; i++) {
    for (int j = 1; j <= COLUNA; j++) {
      int valor = labirintods[i][j];  // aqui é só int, não struct
      atualizarLED(i, j, valor);    // envia para matriz física
    }
  }
  enviaPosicaoFantasma(f1);
  enviaPosicaoFantasma(f2);
}

void colore(Lab* celula){
  if(celula->ocupado){
    leds[celula->ind] = CRGB(0, 0, 0);
    return;
  }
  switch (celula->cor){
      case 1: /* celula ponto */
        leds[celula->ind] = AMARELO;
        break;
      case 5: /* celula bonus */
        leds[celula->ind] = AZUL;
        break;
      case 3: /* celula vermelho*/
        leds[celula->ind] = VERMELHO;
        break;
      case 4: /* celula F rosa */
        leds[celula->ind] = ROSA;
        break;
      case 6: 
        leds[celula->ind] = CIANO;
        break; 
      case 7: 
        leds[celula->ind] = LARANJA;
        break; 

      default: /* celula vazia */
        leds[celula->ind] = PRETO;
    }
  FastLED.show();
}

void iniciaLabirinto(){
  for(int linha = 0; linha < LINHA; linha++){
    for(int coluna = 0; coluna < COLUNA; coluna++){
      labirinto[linha][coluna].cor = 1;
      colore(&labirinto[linha][coluna]);
    }
  }
}

void pintaFruta(int i, int j){
  labirinto[i-1][j-1].cor = 5;
  colore(&labirinto[i][j]);
}

void desenhaLabirinto(){
  for(int linha = 0; linha < LINHA; linha++){
    for(int coluna = 0; coluna < COLUNA; coluna++){
      int corFantasma = labirintofant[linha][coluna].cor;


      if (corFantasma == 3 || corFantasma == 4 || corFantasma == 6 || corFantasma == 7) {
        continue; 
      }

      colore(&labirinto[linha][coluna]);
    }
  }
}


void ocupado(Lab* celula){
  celula->cor = 0;
  colore(celula);
  celula->ocupado = true;
  FastLED.show();
}


void resetLab(Lab labirinto[][COLUNA]){
  for(int i = 0; i < LINHA; i++){
        for(int j = 0; j <= COLUNA; j++){
          if(j < COLUNA){
            leds[labirinto[i][j].ind] = BRANCO;
            FastLED.show();
            delay(50);
          }
          if(j > 0){
            leds[labirinto[i][(j-1)].ind] = PRETO;
            FastLED.show();
            delay(50);
          }
        }
      }
}


int posFantasma(Fantasma f){
  return labirintofant[f.linha][f.coluna].ind;
}


void pintarFantasma(Fantasma f, int linhaAnterior, int colunaAnterior){
  int iLED = f.linha - 1;
  int jLED = f.coluna - 1;


  // Apaga a posição anterior
  if (linhaAnterior >= 1 && linhaAnterior <= LINHA &&
      colunaAnterior >= 1 && colunaAnterior <= COLUNA) {
    int iAnt = linhaAnterior - 1;
    int jAnt = colunaAnterior - 1;


    labirintofant[iAnt][jAnt].cor = 0;
    colore(&labirinto[iAnt][jAnt]); 
  }


  if (iLED >= 0 && iLED < LINHA && jLED >= 0 && jLED < COLUNA) {
    labirintofant[iLED][jLED].cor = f.cor;
    int idx = labirintofant[iLED][jLED].ind;

    if(f.cor == 3){
      leds[idx] = VERMELHO; 
    } else if(f.cor == 4){
      leds[idx] = ROSA;  
    }else if (f.cor == 6) {
      leds[idx] = CIANO;
    } else if (f.cor == 7) {
      leds[idx] = LARANJA;
    }

    FastLED.show();
  }
}
