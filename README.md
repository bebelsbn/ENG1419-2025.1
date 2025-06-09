# ENG1419-2025.1
# Projeto de Microcontroladores da Turma A 2025.1 : Pac-Man Físico Interativo

## Descrição Geral

O projeto consiste em criar uma versão física e interativa do clássico jogo **Pac-Man**, utilizando uma **bola física representando o Pac-Man** que se movimenta sobre uma superfície iluminada por uma **tira de LED**. Essa superfície atua como um “**tabuleiro digital**”, com atualizações dinâmicas conforme a lógica do jogo.

Elementos do jogo, como **fantasmas (inimigos)** e **frutas (pontos)**, serão representados por **cores e posições específicas na tira de LED**. O movimento do Pac-Man será rastreado em **tempo real** por meio de **visão computacional com OpenCV**, promovendo a integração entre o ambiente físico e a lógica digital do jogo.

---

## Divisão do Trabalho

O projeto foi dividido em **quatro grandes partes**, atribuídas a diferentes membros da equipe, com metas semanais de desenvolvimento:

### *Lógica do Jogo*

* **Meta 1:** Menu básico no LCD (start/nível/andamento/fim) + navegação por botões
* **Meta 2:** Mapa do jogo com atualização via serial + regras do jogo
* **Meta 3:** Integração lógica com a posição detectada pelo OpenCV
* **Meta 4:** Comunicação entre a lógica e Arduino para acionar efeitos da tira de LED
* **Meta 5:** Integração final e edição do vídeo

---

### *Movimento*

* **Meta 1:** Prototipagem da base de movimentação da bola
* **Meta 2:** Desenvolvimento do controle de movimento (servo)
* **Meta 3:** Controle dos servos via acelerômetro
* **Meta 4:** Integração com a lógica do jogo (ex: impedir movimento inválido)
* **Meta 5:** Integração final

---

### *Tira de LED*

* **Meta 1:** Mapeamento da matriz de LED e testes de controle com Arduino
* **Meta 2:** Implementação de padrões de cores para pontos, fantasmas e caminhos
* **Meta 3:** Integração com menu do jogo
* **Meta 4:** Sincronização com movimento e detecção
* **Meta 5:** Integração final

---

### *Detecção com OpenCV + Montagem*

* **Meta 1:** Protótipo básico de detecção da bola (Pac-Man) com webcam
* **Meta 2:** Rastreamento da posição em tempo real com coordenadas calibradas
* **Meta 3:** Integração lógica com a posição detectada pelo OpenCV
* **Meta 4:** Modelagem e impressão 3D do labirinto físico
* **Meta 5:** Integração final

