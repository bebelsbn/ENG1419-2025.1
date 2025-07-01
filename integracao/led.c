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
# define BRIGHTNESS 20
# define COLUNA 8
# define LINHA 8
typedef struct celula Lab;
typedef struct inimigo Fantasma;
struct celula{
 int ind; /* indice correto do led na fita */
 int cor; /* 0 preto | 1 ponto | 2 bonus | 3 fantasma 1 | 4 fantasma 2
*/
 bool ocupado; /* bola está ou não na posição */
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
/* variavel de leds -> fita de leds */
CRGB leds[NUM_LEDS];
/* variavel do labirinto -> matriz de leds */
Lab labirinto[LINHA][COLUNA];
int labirintods[MATRIZ_TAM][MATRIZ_TAM];
/* variaveis globais os inimigos -> fantasmas */
Fantasma f1;
Fantasma f2;
/* CÓDIGO CORES
 CRGB(255, 0, 0); VERDE
 CRGB(0, 255, 0); VERMELHO
 CRGB(0, 0, 255); AZUL -> USAR COMO O BONUS
 CRGB(255, 255, 0); AMARELO -> USAR PARA O JOGO
 CRGB(255, 0, 255); CIANO -> USAR COMO O FANTASMA
 CRGB(0, 255, 255); ROSA -> USAR COMO O FANTASMA
 CRGB(255, 255, 255); BRANCO
*/
/* variavel global inicia jogo */
bool jogo = false;
int nivel = 1;
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
 FastLED.addLeds<TIPO, DATA_PIN, RGB>(leds, NUM_LEDS);
 FastLED.setBrightness(BRIGHTNESS);
 int indice = 1;
 for (int i = 0; i < LINHA; i++) {
 if (i % 2 == 0) {
 for (int j = COLUNA-1; j >=0; j--) {
 labirinto[i][j] = {indice, 0, false};
 colore(&labirinto[i][j]);
 indice++;
 }
 } else {
 for (int j = 0; j < COLUNA; j++) {
 labirinto[i][j] = {indice, 0, false};
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
 } else if (estadoAtual == JOGO) {
 botaoReset.process();
 botaoMenu.process();
 static unsigned long ultimaAtualizacao = 0;
 if (millis() - ultimaAtualizacao >= 1000) {
 ultimaAtualizacao = millis();
 if (estadoAtual == JOGO && jogo && tempo >= 0) {
 tempo++;
 exibirStatus();
 }
 }
 static unsigned long ultimaMovimentacao = 0;
 if (millis() - ultimaMovimentacao >= 2000) {
 ultimaMovimentacao = millis();
 // Garante que só mova e redesenhe se ainda estiver no jogo
 if (estadoAtual == JOGO && jogo) {
 moverFantasma1(f1);
 moverFantasma2(f2);
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
 ocupado(&labirinto[i][j]);
 if (i >= 0 && i < MATRIZ_TAM && j >= 0 && j < MATRIZ_TAM) {
 int valorAtual = labirintods[i][j];
 if (i == 0 || i == MATRIZ_TAM - 1 || j == 0 || j ==
MATRIZ_TAM - 1) {
 Serial.print("Posição bloqueada (");
 Serial.print(i); Serial.print(", "); Serial.print(j);
 Serial.println(") - está na borda da matriz.");
 return;
 }
 for (int x = 0; x < MATRIZ_TAM; x++) {
 for (int y = 0; y < MATRIZ_TAM; y++) {
 if (labirintods[x][y] == 3) {
 labirintods[x][y] = 10;
 atualizarLED(x, y, 10);
 }
 }
 }
 switch (valorAtual) {
 case 0:
 pontos += 1;
 labirintods[i][j] = 10;
 atualizarLED(i, j, 10);
 break;
 case 5:
 poderAtivo = true;
 tempoPoder = millis();
 pontos += 2;
 labirintods[i][j] = 10;
 atualizarLED(i, j, 10);
 //brilharTudoAmareloTemporariamente();
 break;
 case 2:
 case 4:
 if (poderAtivo) {
 labirintods[i][j] = 10;
 atualizarLED(i, j, 10);
 tela.setCursor(60, 220); // Ajuste posição se quiser
 tela.setTextColor(TFT_RED);
 tela.setTextSize(2);
 tela.print("TEMPO PAUSADO!");
 } else {
 pontos = max(0, pontos - 1);
 tempo += 10;
 labirintods[i][j] = 10;
 atualizarLED(i, j, 10);
 Serial.println("Encostou no fantasma! +10 segundos");
 }
 break;
 }
 labirintods[i][j] = 3;
 atualizarLED(i, j, 3);
 desenharLabirinto();
 exibirStatus();
 Serial.print("Posição atualizada para: ");
 Serial.print(i); Serial.print(", "); Serial.println(j);
 }
 }
 } else if (todosPontosComidos() || texto == "FIM") {
 jogo = false;
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
 botaoMenu.init(&tela, &touch, 150, 130, 100, 40, TFT_WHITE,
TFT_BLUE, TFT_BLACK, "MENU", 2);
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
void exibirMenuInicial() {
 jogo = false;
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
 // esta 60 120 e 180 120
 botaoStart.init(&tela, &touch, 180, 120, 100, 40, TFT_WHITE,
TFT_GREEN, TFT_BLACK, "START", 2);
 botaoNivel.init(&tela, &touch, 180, 200, 100, 40, TFT_WHITE,
TFT_ORANGE, TFT_BLACK, "NIVEL", 2);
 botaoStart.setPressHandler(iniciarJogo);
 botaoNivel.setPressHandler(mudarNivel);
 exibirNivel();
}
void voltarMenu(JKSButton &b) {
 jogo = false;
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
 }
 desenharLabirinto();
 botaoReset.init(&tela, &touch, 100, 60, 100, 40, TFT_WHITE, TFT_RED,
TFT_BLACK, "RESET", 2);
 botaoMenu.init(&tela, &touch, 220, 60, 100, 40, TFT_WHITE, TFT_BLUE,
TFT_BLACK, "MENU", 2);
 botaoReset.setPressHandler(resetarJogo);
 botaoMenu.setPressHandler(voltarMenu);
 //enviarEstadoInicial();
}
void mudarNivel(JKSButton &b) {
 nivel = (nivel % 3) + 1;
 exibirNivel();
 //Serial.print("Nivel alterado para: ");
 //Serial.println(nivel);
}
void resetarJogo(JKSButton &b) {
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
 int valor = labirintods[i][j];
 int x = j * TAM_BLOCO + xInicial;
 int y = i * TAM_BLOCO + yInicial;
 uint16_t cor;
 // Define a cor de cada célula
 switch (valor) {
 case 0: cor = TFT_YELLOW; break; // caminho
 case 2: cor = TFT_GREEN; break; // jogador (pacman)
 case 3: cor = TFT_RED; break; // fantasma vermelho
 case 4: cor = TFT_PINK; break; // fantasma rosa
 case 5: cor = TFT_BLUE; break; // fruta
 case 9: cor = TFT_WHITE; break; // borda
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
 labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 :
0;
 }
 }
 labirintods[1][1] = 2; // Pac-Man
 f1 = {2, 1, 3};
 f2 = {5, 2, 4};
 labirintods[3][1] = 5;
 pintaFruta(3, 1);
 labirintods[2][2] = 5;
 pintaFruta(2, 2);
}
void tabela2() {
 for (int i = 0; i < MATRIZ_TAM; i++) {
 for (int j = 0; j < MATRIZ_TAM; j++) {
 labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 :
0;
 }
 }
 labirintods[8][1] = 2;
 labirintods[4][4] = 3; f1 = {4, 4, 3};
 labirintods[3][6] = 4; f2 = {3, 6, 4};
 labirintods[2][2] = 5;
 labirintods[6][3] = 5;
 labirintods[7][7] = 5;
}
void tabela3() {
 for (int i = 0; i < MATRIZ_TAM; i++) {
 for (int j = 0; j < MATRIZ_TAM; j++) {
 labirintods[i][j] = (i == 0 || i == 9 || j == 0 || j == 9) ? 9 :
0;
 }
 }
 labirintods[8][1] = 2;
 labirintods[2][2] = 3; f1 = {2, 2, 3};
 labirintods[3][3] = 4; f2 = {3, 3, 4};
 labirintods[6][6] = 5;
 labirintods[4][7] = 5;
 labirintods[2][5] = 5;
}
bool todosPontosComidos() {
 for (int i = 1; i < MATRIZ_TAM - 1; i++) {
 for (int j = 1; j < MATRIZ_TAM - 1; j++) {
 if (labirintods[i][j] == 0) return false;
 }
 }
 return true;
}
void moverFantasma1(Fantasma &f) {
 // Zera a posição anterior (se não for borda)
 if (f.linha > 0 && f.linha < MATRIZ_TAM - 1 && f.coluna > 0 &&
f.coluna < MATRIZ_TAM - 1) {
 // Só apaga se não havia fruta ali
 if (labirintods[f.linha][f.coluna] == f.cor) {
 labirintods[f.linha][f.coluna] = 0;
 atualizarLED(f.linha, f.coluna, 0);
 }
 }
 // Tentar até encontrar uma direção válida (no máximo 4 tentativas)
 for (int tentativa = 0; tentativa < 4; tentativa++) {
 int direcao = random(4);
 int novaLinha = f.linha;
 int novaColuna = f.coluna;
 int novaCor = f.cor;
 if (direcao == 0 && f.linha > 1) novaLinha--;
// cima
 else if (direcao == 1 && f.linha < MATRIZ_TAM - 2) novaLinha++;
// baixo
 else if (direcao == 2 && f.coluna > 1) novaColuna--;
// esquerda
 else if (direcao == 3 && f.coluna < MATRIZ_TAM - 2) novaColuna++;
// direita
 // Verifica se pode andar: não pode pisar em parede (9) ou fruta
(5)
 int destino = labirintods[novaLinha][novaColuna];
 if (destino != 9 && destino != 5 && destino != 2 && destino != 4) {
 f.linha = novaLinha;
 f.coluna = novaColuna;
 break;
 }
 }
 desenhaLabirinto();
 pintarFantasma(f);
 // Atualiza a nova posição
 labirintods[f.linha][f.coluna] = f.cor;
 atualizarLED(f.linha, f.coluna, f.cor);
 enviaPosicaoFantasma(f);
}
void moverFantasma2(Fantasma &f) {
  // Zera a posição anterior (se não for borda)
  if (f.linha > 0 && f.linha < MATRIZ_TAM - 1 && f.coluna > 0 && f.coluna < MATRIZ_TAM - 1) {
    if (labirintods[f.linha][f.coluna] == f.cor) {
      labirintods[f.linha][f.coluna] = 0;
      atualizarLED(f.linha, f.coluna, 0);
      // Também zera a cor na matriz de LEDs físicos
      labirinto[f.linha - 1][f.coluna - 1].cor = 1;  // volta a ser ponto
    }
  }

  // Tentar até encontrar uma direção válida
  for (int tentativa = 0; tentativa < 4; tentativa++) {
    int direcao = random(4);
    int novaLinha = f.linha;
    int novaColuna = f.coluna;

    if (direcao == 0 && f.linha > 1) novaLinha--;          // cima
    else if (direcao == 1 && f.linha < MATRIZ_TAM - 2) novaLinha++;  // baixo
    else if (direcao == 2 && f.coluna > 1) novaColuna--;   // esquerda
    else if (direcao == 3 && f.coluna < MATRIZ_TAM - 2) novaColuna++; // direita

    int destino = labirintods[novaLinha][novaColuna];
    if (destino != 9 && destino != 5 && destino != 2 && destino != 4) {
      f.linha = novaLinha;
      f.coluna = novaColuna;
      break;
    }
  }

  // Atualiza a lógica do jogo
  labirintods[f.linha][f.coluna] = f.cor;

  // ✅ Atualiza também a matriz física antes de desenhar
  labirinto[f.linha - 1][f.coluna - 1].cor = f.cor;

  // Agora sim pode desenhar o labirinto e pintar os LEDs
  desenhaLabirinto();             // Respeita a nova cor do fantasma
  atualizarLED(f.linha, f.coluna, f.cor); // Atualiza o LED físico
  enviaPosicaoFantasma(f);       // Mensagem via Serial
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
 Serial.print("Fantasma: ");
 Serial.print(f.linha);
 Serial.print(",");
 Serial.print(f.coluna);
 Serial.print(",");
 Serial.println(f.cor);
}
// Envia estado inicial ao começar o jogo
void enviarEstadoInicial() {
 for (int i = 1; i < MATRIZ_TAM - 1; i++) {
  for (int j = 1; j < MATRIZ_TAM - 1; j++) {
    atualizarLED(i, j, labirintods[i][j]);
  }
}

 enviaPosicaoFantasma(f1);
 enviaPosicaoFantasma(f2);
}
// int enderecoRecorde(int nivel) {
 //return (nivel - 1) * sizeof(int);
//}
void colore(Lab* celula){
 if(celula->ocupado){
 leds[celula->ind] = CRGB(0, 0, 0);
 return;
 }
 switch (celula->cor){
 case 1: /* celula ponto */
 leds[celula->ind] = CRGB(255, 255, 0);
 break;
 case 5: /* celula bonus */
 leds[celula->ind] = CRGB(0, 0, 255);
 break;
 case 3: /* celula vermelho*/
 leds[celula->ind] = CRGB(0, 255, 0);
 break;
 case 4: /* celula F rosa */
 leds[celula->ind] = CRGB(0, 255, 255);
 break;
 default: /* celula vazia */
 leds[celula->ind] = CRGB(0, 0, 0);
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
 leds[labirinto[i][j].ind] = CRGB(255, 255, 255);
 FastLED.show();
 delay(50);
 }
 if(j > 0){
 leds[labirinto[i][(j-1)].ind] = CRGB(0, 0, 0);
 FastLED.show();
 delay(50);
 }
 }
 }
}
int posFantasma(Fantasma f){
 return labirinto[f.linha][f.coluna].ind;
}
void pintarFantasma(Fantasma f){
 int idx = labirinto[f.linha-1][f.coluna-1].ind;
 Serial.print("F cor: ");
 Serial.println(f.cor);
 if(f.cor == 3){
 leds[idx] = CRGB(0, 255, 0);
 } else if(f.cor == 4){
 leds[idx] = CRGB(0, 255, 255);
 }
 FastLED.show();
}