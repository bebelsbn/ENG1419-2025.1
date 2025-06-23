#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <JKSButton.h>

#define TAM_BLOCO 12
#define MATRIZ_TAM 10

MCUFRIEND_kbv tela;
TouchScreen touch(6, A1, A2, 7, 300);

JKSButton botaoStart, botaoNivel;
JKSButton botaoReset, botaoMenu;

enum EstadoTela { MENU, JOGO, FIM };
EstadoTela estadoAtual = MENU;

int nivel = 1;
int pontos = 0;
int tempo = 0;
bool jogoAtivo = false;
bool poderAtivo = false;
unsigned long tempoPoder = 0;
unsigned long inicio;


int labirinto[MATRIZ_TAM][MATRIZ_TAM];

void setup() {
  tela.begin(tela.readID());
  tela.setRotation(1);
  Serial.begin(9600);
  exibirMenuInicial();
}

void loop() {

  if (estadoAtual == MENU) {
    botaoStart.process();
    botaoNivel.process();
  } else if (estadoAtual == JOGO && jogoAtivo == true) {
    botaoReset.process();
    botaoMenu.process();

    static unsigned long ultimaAtualizacao = 0;
    if (millis() - ultimaAtualizacao >= 1000) {
      ultimaAtualizacao = millis();
      if (estadoAtual == JOGO && jogoAtivo && tempo >= 0) {
        tempo++; 
        exibirStatus();
      }
    }

  } else if (estadoAtual == FIM) {
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
        int i = iStr.toInt();
        int j = jStr.toInt();

        if (i >= 0 && i < MATRIZ_TAM && j >= 0 && j < MATRIZ_TAM) {
          int valorAtual = labirinto[i][j];

          // Bloqueia borda 
          if (i == 0 || i == MATRIZ_TAM - 1 || j == 0 || j == MATRIZ_TAM - 1) {
            Serial.print("Posição bloqueada (");
            Serial.print(i); Serial.print(", "); Serial.print(j);
            Serial.println(") - está na borda da matriz.");
            return;
          }


          for (int x = 0; x < MATRIZ_TAM; x++) {
            for (int y = 0; y < MATRIZ_TAM; y++) {
              if (labirinto[x][y] == 3) {
                labirinto[x][y] = 10;  // transforma em caminho preto
              }
            }
          }


          switch (valorAtual) {
            case 0: // Caminho +1
              pontos += 1;
              labirinto[i][j] = 10;
              break;

            case 5: 
              if (tempo >= 10) {
                tempo = tempo - 10;
              } else {
                tempo = 0;
              }

              poderAtivo = true;
              tempoPoder = millis();
              pontos+=2;
              labirinto[i][j] = 10; // fruta desaparece
              Serial.println("Pegou fruta! -10 segundos");
              Serial.print("Tempo após fruta: ");
              Serial.println(tempo);
              break;

            case 2: 
            case 4: 
              if (poderAtivo) {
                labirinto[i][j] = 10;
              } else {
                pontos = max(0, pontos - 1);
                tempo += 10;
                Serial.println("Encostou no fantasma! +10 segundos");
                Serial.print("Tempo após fantasma: ");
                Serial.println(tempo);
              }
              break;
          }

          labirinto[i][j] = 3; // Nova posição do Pac-Man
          desenharLabirinto();
          exibirStatus();

          Serial.print("Posição atualizada para: ");
          Serial.print(i); Serial.print(", "); Serial.println(j);
        }
      }
    } else if (todosPontosComidos()) {
      jogoAtivo = false;
      estadoAtual = FIM;
      tela.fillScreen(TFT_BLACK);

      tela.setCursor(70, 50);
      tela.setTextColor(TFT_RED);
      tela.setTextSize(2);
      tela.print("Jogo terminado!");

      tela.setCursor(100, 80);
      tela.setTextColor(TFT_YELLOW);
      tela.print("Tempo: ");
      tela.print(tempo);
      tela.print("s");

      botaoMenu.init(&tela, &touch, 150, 130, 100, 40,TFT_WHITE, TFT_BLUE, TFT_BLACK, "MENU", 2);
      botaoMenu.setPressHandler(voltarMenu);

      Serial.println("Recebido FIM. Jogo encerrado.");
      Serial.print("Tempo final do jogador: ");
      Serial.print(tempo);
      Serial.println(" segundos");
    }
  }
  if (poderAtivo && (millis() - tempoPoder > 3000)) {
    poderAtivo = false;
    Serial.println("Poder especial acabou.");
  }
}

void exibirMenuInicial() {
  jogoAtivo = false;
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
  jogoAtivo = false;
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
  Serial.println("incia");
  tela.fillScreen(TFT_BLACK);
  pontos = 0;
  tempo = 0;
  inicio = millis();
  jogoAtivo = true;
  estadoAtual = JOGO;

  Serial.println("Jogo iniciado.");
  Serial.print("Nivel atual: ");
  Serial.println(nivel);

  exibirStatus();

  if (nivel == 1) {
    tabela1();
  } else if (nivel == 2) {
    tabela2();
  } else if (nivel == 3) {
    tabela3();
  }

  desenharLabirinto();

  botaoReset.init(&tela, &touch, 100, 60, 100, 40, TFT_WHITE, TFT_RED, TFT_BLACK, "RESET", 2);
  botaoMenu.init(&tela, &touch, 220, 60, 100, 40, TFT_WHITE, TFT_BLUE, TFT_BLACK, "MENU", 2);
  botaoReset.setPressHandler(resetarJogo);
  botaoMenu.setPressHandler(voltarMenu);
}

void mudarNivel(JKSButton &b) {
  nivel = (nivel % 3) + 1;
  exibirNivel();
  Serial.print("Nivel alterado para: ");
  Serial.println(nivel);
}

void resetarJogo(JKSButton &b) {
  pontos = 0;
  tempo = 0;
  inicio = millis();
  exibirStatus();
  tela.setCursor(10, 130);
  tela.setTextColor(TFT_RED);
  tela.setTextSize(2);
  Serial.println("Jogo resetado.");
}

void exibirStatus() {
  tela.fillRect(10, 10, 300, 30, TFT_BLACK);
  tela.setCursor(10, 10);
  tela.setTextColor(TFT_CYAN);
  tela.setTextSize(2);
  tela.print("Pontos: ");
  tela.print(pontos);
  tela.print(" | Tempo: ");

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

void desenharLabirinto() {
  int xInicial = (320 - TAM_BLOCO * MATRIZ_TAM) / 2;
  int yInicial = 100;

  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      int valor = labirinto[i][j];
      int x = j * TAM_BLOCO + xInicial;
      int y = i * TAM_BLOCO + yInicial;
      uint16_t cor;

      switch (valor) {
        case 0: cor = TFT_YELLOW; break;    // trilha/caminho
        case 2: cor = TFT_RED; break;      // fantasma 1
        case 3: cor = TFT_GREEN; break;      // Pac-Man 
        case 4 : cor = TFT_PINK; break; // fantasma 2 
        case 5: cor = TFT_BLUE; break; // fruta
        case 9: cor = TFT_WHITE; break;     // borda
        case 10: cor = TFT_BLACK; break;  // ja caminhou
        default: cor = TFT_BLACK; break;
      }

      tela.fillRect(x, y, TAM_BLOCO, TAM_BLOCO, cor);
      tela.drawRect(x, y, TAM_BLOCO, TAM_BLOCO, TFT_BLACK);
    }
  }
}


void tabela1() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      if (i == 0 || i == 9 || j == 0 || j == 9) {
        labirinto[i][j] = 9;  // Borda
      } else {
        labirinto[i][j] = 0;  // Caminho 
      }
    }
  }

  // Pac-Man 
  labirinto[8][1] = 3;

  // Fantasma
  labirinto[2][1] = 2;
  labirinto[5][2] = 4;

  //fruta 
   labirinto[6][6] = 5;
   labirinto[5][6] = 5;
   labirinto[2][2] = 5;

}

void tabela2() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      if (i == 0 || i == 9 || j == 0 || j == 9) {
        labirinto[i][j] = 9;
      } else {
        labirinto[i][j] = 0;
      }
    }
  }

  // Pac-Man
  labirinto[8][1] = 3;

  // Fantasmas
  labirinto[4][4] = 2;
  labirinto[3][6] = 4;

  // Frutas
  labirinto[2][2] = 5;
  labirinto[6][3] = 5;
  labirinto[7][7] = 5;
}

void tabela3() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      if (i == 0 || i == 9 || j == 0 || j == 9) {
        labirinto[i][j] = 9;
      } else {
        labirinto[i][j] = 0;
      }
    }
  }

  // Pac-Man
  labirinto[1][1] = 3;

  // Fantasmas em diagonal
  labirinto[2][2] = 2;
  labirinto[3][3] = 4;

  // Frutas mais espalhadas
  labirinto[6][6] = 5;
  labirinto[4][7] = 5;
  labirinto[2][5] = 5;
}


bool todosPontosComidos() {
  for (int i = 1; i < MATRIZ_TAM - 1; i++) {
    for (int j = 1; j < MATRIZ_TAM - 1; j++) {
      if (labirinto[i][j] == 0) {
        return false;
      }
    }
  }
  return true; // Nenhum ponto amarelo restante no interior
}
