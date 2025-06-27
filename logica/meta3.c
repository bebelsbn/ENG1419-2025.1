#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <JKSButton.h>
#include <EEPROM.h>




#define TAM_BLOCO 12
#define MATRIZ_TAM 10


// Tamanho útil da matriz de LED física (8x8)
#define LINHA 8
#define COLUNA 8




MCUFRIEND_kbv tela;
TouchScreen touch(6, A1, A2, 7, 300);


JKSButton botaoStart, botaoNivel;
JKSButton botaoReset, botaoMenu;


// struct fantasma
typedef struct {
  int linha;
  int coluna;
  int cor; // 2 = vermelho, 4 = rosa
} Fantasma;


// Instâncias dos dois fantasmas
Fantasma f1 = {2, 1, 2};  // vermelho
Fantasma f2 = {5, 2, 4};  // rosa




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
int backupCores[8][8];  // para guardar as cores temporárias durante o brilho




void atualizarLED(int i, int j, int valor) {
  if (i <= 0 || i >= MATRIZ_TAM - 1 || j <= 0 || j >= MATRIZ_TAM - 1) {
    return;
  }


  String cor;
  switch (valor) {
    case 0: cor = "amarelo"; break;
    case 2: cor = "vermelho"; break;
    case 3: cor = "verde"; break;
    case 4: cor = "rosa"; break;
    case 5: cor = "azul"; break;
    case 10: cor = "preto"; break;
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


// Envia a posição do fantasma via Serial
void enviaPosicaoFantasma(Fantasma f) {
  Serial.print("Fantasma:");
  Serial.print(f.linha);
  Serial.print(",");
  Serial.println(f.coluna);
}


// Envia estado inicial ao começar o jogo
void enviarEstadoInicial() {
  for (int i = 1; i <= LINHA; i++) {
    for (int j = 1; j <= COLUNA; j++) {
      int valor = labirinto[i][j];  // aqui é só int, não struct
      atualizarLED(i, j, valor);    // envia para matriz física
    }
  }
  enviaPosicaoFantasma(f1);
  enviaPosicaoFantasma(f2);
}




// int enderecoRecorde(int nivel) {
  //return (nivel - 1) * sizeof(int);
//}




void setup() {
  tela.begin(tela.readID());
  tela.setRotation(1);
  Serial.begin(9600);
  exibirMenuInicial();
}


// Aqui começa a função que desenha o menu principal
void exibirMenuInicial() {
  jogoAtivo = false;
  estadoAtual = MENU;
  tela.fillScreen(TFT_BLACK);


  tela.setCursor(10, 10);
  tela.setTextColor(TFT_WHITE);
  tela.setTextSize(2);
  tela.print("Menu Inicial");


  //int endereco = enderecoRecorde(nivel);
  //int recordeSalvo;
  //EEPROM.get(endereco, recordeSalvo);


  //tela.setCursor(10,70);
  //tela.setTextColor(TFT_YELLOW);
  //tela.setTextSize(2);
  //tela.print("Recorde: ");
  //tela.println(recordeSalvo);




  botaoStart.init(&tela, &touch, 60, 120, 100, 40, TFT_WHITE, TFT_GREEN, TFT_BLACK, "START", 2);
  botaoNivel.init(&tela, &touch, 180, 120, 100, 40, TFT_WHITE, TFT_ORANGE, TFT_BLACK, "NIVEL", 2);
  botaoStart.setPressHandler(iniciarJogo);
  botaoNivel.setPressHandler(mudarNivel);


  exibirNivel();
}




// Volta ao menu após reset ou fim de jogo
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


// Mostra o nível atual no menu
void exibirNivel() {
  tela.fillRect(10, 40, 300, 30, TFT_BLACK);
  tela.setCursor(10, 40);
  tela.setTextColor(TFT_ORANGE);
  tela.setTextSize(2);
  tela.print("Nivel atual: ");
  tela.println(nivel);
}


// Incio do jogo após pressionar "START"
void iniciarJogo(JKSButton &b) {
  Serial.println("inicia");
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


  enviarEstadoInicial();
}


void mudarNivel(JKSButton &b) {
  nivel = (nivel % 3) + 1;
  exibirNivel();
  Serial.print("Nivel alterado para: ");
  Serial.println(nivel);
}


void resetarJogo(JKSButton &b) {
  Serial.println("reset");
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


// Desenha o labirinto visualmente na tela TFT
void desenharLabirinto() {
  int xInicial = (320 - TAM_BLOCO * MATRIZ_TAM) / 2;
  int yInicial = 100;


  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      int valor = labirinto[i][j];
      int x = j * TAM_BLOCO + xInicial;
      int y = i * TAM_BLOCO + yInicial;
      uint16_t cor;


      // Define a cor de cada célula
      switch (valor) {
        case 0: cor = TFT_YELLOW; break; // caminho
        case 2: cor = TFT_RED; break;    // fantasma vermelho
        case 3: cor = TFT_GREEN; break;  // jogador (pacman)
        case 4: cor = TFT_PINK; break;   // fantasma rosa
        case 5: cor = TFT_BLUE; break;   // fruta
        case 9: cor = TFT_WHITE; break;  // borda
        case 10: cor = TFT_BLACK; break; // já caminhado
        default: cor = TFT_BLACK; break;
      }


      // Pinta e desenha a célula na tela
      tela.fillRect(x, y, TAM_BLOCO, TAM_BLOCO, cor);
      tela.drawRect(x, y, TAM_BLOCO, TAM_BLOCO, TFT_BLACK);
    }
  }
}


// Define o labirinto do nível 1 e posiciona fantasmas/frutas
void tabela1() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirinto[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }


  labirinto[8][1] = 3;      // Pac-Man
  labirinto[2][1] = 2; f1 = {2, 1, 2}; // Fantasma 1
  labirinto[5][2] = 4; f2 = {5, 2, 4}; // Fantasma 2


  labirinto[6][6] = 5;
  labirinto[5][6] = 5;
  labirinto[2][2] = 5;
}


// Define o nível 2
void tabela2() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirinto[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }


  labirinto[8][1] = 3;
  labirinto[4][4] = 2; f1 = {4, 4, 2};
  labirinto[3][6] = 4; f2 = {3, 6, 4};


  labirinto[2][2] = 5;
  labirinto[6][3] = 5;
  labirinto[7][7] = 5;
}


// Define o nível 3
void tabela3() {
  for (int i = 0; i < MATRIZ_TAM; i++) {
    for (int j = 0; j < MATRIZ_TAM; j++) {
      labirinto[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 : 0;
    }
  }


  labirinto[8][1] = 3;
  labirinto[2][2] = 2; f1 = {2, 2, 2};
  labirinto[3][3] = 4; f2 = {3, 3, 4};


  labirinto[6][6] = 5;
  labirinto[4][7] = 5;
  labirinto[2][5] = 5;
}


// Verifica se todas as bolinhas amarelas foram comidas
bool todosPontosComidos() {
  for (int i = 1; i < MATRIZ_TAM - 1; i++) {
    for (int j = 1; j < MATRIZ_TAM - 1; j++) {
      if (labirinto[i][j] == 0) return false;
    }
  }
  return true;
}


// Brilho amarelo temporário nos LEDs e tela por 5 segundos (efeito de poder)
void brilharTudoAmareloTemporariamente() {
  int xInicial = (320 - TAM_BLOCO * MATRIZ_TAM) / 2;
  int yInicial = 100;


  // Guarda estado original e acende tudo em amarelo
  for (int i = 1; i <= 8; i++) {
    for (int j = 1; j <= 8; j++) {
      backupCores[i - 1][j - 1] = labirinto[i][j];
      Serial.print("LED:");
      Serial.print(i - 1);
      Serial.print(",");
      Serial.print(j - 1);
      Serial.println(",amarelo");


      int x = j * TAM_BLOCO + xInicial;
      int y = i * TAM_BLOCO + yInicial;
      tela.fillRect(x, y, TAM_BLOCO, TAM_BLOCO, TFT_YELLOW);
      tela.drawRect(x, y, TAM_BLOCO, TAM_BLOCO, TFT_BLACK);
    }
  }


  delay(5000); // espera os 5 segundos


  // Restaura estado anterior dos blocos
  for (int i = 1; i <= 8; i++) {
    for (int j = 1; j <= 8; j++) {
      int corOriginal = backupCores[i - 1][j - 1];
      labirinto[i][j] = corOriginal;
      atualizarLED(i, j, corOriginal);
    }
  }


  desenharLabirinto(); // atualiza a tela
}


// Movimento aleatório simples para o fantasma (linha ou coluna)
void moverFantasma(Fantasma &f) {
  int anterior = backupCores[f.linha - 1][f.coluna - 1];
  if (anterior == 0 || anterior == 5) {
    labirinto[f.linha][f.coluna] = anterior;
    atualizarLED(f.linha, f.coluna, anterior);
  } else {
    labirinto[f.linha][f.coluna] = 10;
    atualizarLED(f.linha, f.coluna, 10);
  }


  int direcao = random(4);
  int novaLinha = f.linha;
  int novaColuna = f.coluna;
  if (direcao == 0 && f.linha > 1) novaLinha--;           // cima
  else if (direcao == 1 && f.linha < MATRIZ_TAM - 2) novaLinha++;  // baixo
  else if (direcao == 2 && f.coluna > 1) novaColuna--;    // esquerda
  else if (direcao == 3 && f.coluna < MATRIZ_TAM - 2) novaColuna++; // direita


  if (labirinto[novaLinha][novaColuna] != 9) {
    f.linha = novaLinha;
    f.coluna = novaColuna;
  }


  labirinto[f.linha][f.coluna] = f.cor;
  atualizarLED(f.linha, f.coluna, f.cor);
  enviaPosicaoFantasma(f);
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

    static unsigned long ultimaMovimentacao = 0;
    if (millis() - ultimaMovimentacao >= 2000) {
    ultimaMovimentacao = millis();

    // Garante que só mova e redesenhe se ainda estiver no jogo
    if (estadoAtual == JOGO && jogoAtivo) {
        moverFantasma(f1);
        moverFantasma(f2);
        desenharLabirinto();
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


          if (i == 0 || i == MATRIZ_TAM - 1 || j == 0 || j == MATRIZ_TAM - 1) {
            Serial.print("Posição bloqueada (");
            Serial.print(i); Serial.print(", "); Serial.print(j);
            Serial.println(") - está na borda da matriz.");
            return;
          }


          for (int x = 0; x < MATRIZ_TAM; x++) {
            for (int y = 0; y < MATRIZ_TAM; y++) {
              if (labirinto[x][y] == 3) {
                labirinto[x][y] = 10;
                atualizarLED(x, y, 10);
              }
            }
          }


          switch (valorAtual) {
            case 0:
              pontos += 1;
              labirinto[i][j] = 10;
              atualizarLED(i, j, 10);
              break;


            case 5:
              poderAtivo = true;
              tempoPoder = millis();
              pontos += 2;
              labirinto[i][j] = 10;
              atualizarLED(i, j, 10);
              brilharTudoAmareloTemporariamente();
              break;


            case 2:
            case 4:
              if (poderAtivo) {
                labirinto[i][j] = 10;
                atualizarLED(i, j, 10);


                tela.setCursor(60, 220); // Ajuste posição se quiser
                tela.setTextColor(TFT_RED);
                tela.setTextSize(2);
                tela.print("TEMPO PAUSADO!");
              } else {
                pontos = max(0, pontos - 1);
                tempo += 10;
                labirinto[i][j] = 10;
                atualizarLED(i, j, 10);
                Serial.println("Encostou no fantasma! +10 segundos");
              }
              break;
          }


          labirinto[i][j] = 3;
          atualizarLED(i, j, 3);
          desenharLabirinto();
          exibirStatus();


          Serial.print("Posição atualizada para: ");
          Serial.print(i); Serial.print(", "); Serial.println(j);
        }
      }
    } else if (todosPontosComidos() || texto == "FIM") {
      jogoAtivo = false;
      estadoAtual = FIM;


      //int endereco = enderecoRecorde(nivel);
      //int recordeAnterior;
      //EEPROM.get(endereco, recordeAnterior);


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
        tela.print(minutos);
        tela.print("m ");
        if (segundos < 10) tela.print("0");
      }


      tela.print(segundos);
      tela.print("s");


      // if (pontos > recordeAnterior) {
        //EEPROM.put(endereco, pontos);
        //tela.setCursor(80, 110);
        //tela.setTextColor(TFT_GREEN);
        //tela.setTextSize(2);
        //tela.print("NOVO RECORDE!");
      //}


      botaoMenu.init(&tela, &touch, 150, 130, 100, 40, TFT_WHITE, TFT_BLUE, TFT_BLACK, "MENU", 2);
      botaoMenu.setPressHandler(voltarMenu);


      Serial.println("Recebido FIM. Jogo encerrado.");
      Serial.print("Tempo final do jogador: ");
      //Serial.print(tempo);
      //Serial.println(" segundos");
    }
  }


  if (poderAtivo && (millis() - tempoPoder > 3000)) {
    poderAtivo = false;
    Serial.println("Poder especial acabou.");
  }
}


