import cv2 as cv
import numpy as np
from serial import Serial
from threading import Timer, Thread
import time

global timer
timer = None

global l
global c
global comecei
global texto

c = 0
l = 0
tempo = 0
pontos = 0
texto = None
comecei = False

meu_serial = Serial("COM10", baudrate=9600, timeout=0.1)

stream = cv.VideoCapture(0)
prevCircle = None
grid_shape = (8, 8)
dist = lambda x1, y1, x2, y2: (x1 - x2) ** 2 + (y1 - y2) ** 2


def draw_grid(img, grid_shape, color=(0, 255, 0), thickness=1):
    h, w, _ = img.shape
    rows, cols = grid_shape
    dy, dx = h / rows, w / cols
    for x in np.linspace(start=dx, stop=w - dx, num=cols - 1):
        x = int(round(x))
        cv.line(img, (x, 0), (x, h), color=color, thickness=thickness)
    for y in np.linspace(start=dy, stop=h - dy, num=rows - 1):
        y = int(round(y))
        cv.line(img, (0, y), (w, y), color=color, thickness=thickness)
    return img


def show_hsv_on_click(event, x, y, flags, param):
    if event == cv.EVENT_LBUTTONDOWN:
        frameS, dh, dw = param
        h, w, _ = frameS.shape

        x_s = x - int(dw)
        y_s = y - int(dh)

        if 0 <= x_s < w and 0 <= y_s < h:
            hsv = cv.cvtColor(frameS, cv.COLOR_BGR2HSV)
            pixel_hsv = hsv[y_s, x_s]
            print(f"Clicked HSV: H={pixel_hsv[0]}, S={pixel_hsv[1]}, V={pixel_hsv[2]}")
        else:
            print("Clique fora da área útil.")


def timer_recorrente():
    global c, l, timer
    texto = "Bolinha: " + str(l) + "," + str(c) + "\n"
    try:
        meu_serial.write(texto.encode("utf-8"))
        #print(texto.strip())
    except:
        print("Erro ao enviar pelo Serial.")

    timer = Timer(0.05, timer_recorrente)
    timer.start()


def inicia():
    global timer
    if timer is None:
        timer_recorrente()


def thread_serial():
    global comecei, tempo, pontos, timer
    while True:
        texto_r = meu_serial.readline().decode().strip()
        
        if texto_r != "":
            print("Recebido: ", texto_r)
            if texto_r == "inicia":
                print("oi")
                comecei = True
                inicia()
            elif texto_r == "reset":
                comecei = False
                tempo = 0
                pontos = 0
                if timer is not None:
                    timer.cancel()
                    timer = None
            elif texto_r == "menu":
                comecei = False
                if timer is not None:
                    timer.cancel()
                    timer = None
        time.sleep(0.05)


def thread_camera():
    global c, l, prevCircle, comecei, texto
    while True:
        ret, frame = stream.read()
        if not ret:
            break

        h, w, _ = frame.shape
        crop_size = h
        start_x = (w - crop_size) // 2
        frame = frame[:, start_x:start_x + crop_size]
        frameS = cv.resize(frame, (0, 0), None, 0.8, 0.8)

        hsvFrame = cv.cvtColor(frameS, cv.COLOR_BGR2HSV)
        amarelo_escuro = (0, 0, 0)
        amarelo_claro = (27, 255, 255)
        yellowFrame = cv.inRange(hsvFrame, amarelo_escuro, amarelo_claro)

        kernel = np.ones((16, 16), np.uint8)
        yellowFrame = cv.morphologyEx(yellowFrame, cv.MORPH_OPEN, kernel)
        yellowFrame = cv.morphologyEx(yellowFrame, cv.MORPH_CLOSE, kernel)

        circles = cv.HoughCircles(yellowFrame, cv.HOUGH_GRADIENT, dp=1.2, minDist=100,
                                  param1=100, param2=2, minRadius=9, maxRadius=12)

        if circles is not None:
            circles = np.uint16(np.around(circles))
            chosen = None
            for i in circles[0, :]:
                if chosen is None:
                    chosen = i
                if prevCircle is not None:
                    if dist(chosen[0], chosen[1], prevCircle[0], prevCircle[1]) <= dist(i[0], i[1], prevCircle[0], prevCircle[1]):
                        chosen = i

            if chosen is not None:
                c_px, l_px = chosen[0], chosen[1]
                hS, wS, _ = frameS.shape
                rows, cols = grid_shape
                dy, dx = hS / rows, wS / cols

                col = int(c_px // dx)
                row = int(l_px // dy)
                row = rows - 1 - row
                col = cols - 1 - col

                c, l = col, row
                cv.circle(frameS, (chosen[0], chosen[1]), 1, (0, 100, 100), 3)
                cv.circle(frameS, (chosen[0], chosen[1]), chosen[2], (255, 0, 255), 3)
                prevCircle = chosen

        draw_grid(frameS, grid_shape, color=(0, 0, 0), thickness=1)

        hF, wF, _ = frame.shape
        hS, wS, _ = frameS.shape
        dh = (hF - hS) / 2
        dw = (wF - wS) / 2

        canvas = np.zeros_like(frame)
        canvas[int(dh):int(dh + hS), int(dw):int(dw + wS)] = frameS
        frame = canvas

        text = f"Posicao do PacMan: {l},{c}"
        cv.putText(frame, text, (20, 30), cv.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        cv.imshow("frame", frame)
        cv.setMouseCallback("frame", show_hsv_on_click, param=(frameS, dh, dw))
        cv.imshow("yellow frame", yellowFrame)

        if cv.waitKey(1) & 0xFF == ord("q"):
            break

    stream.release()
    cv.destroyAllWindows()


# Iniciar as threads
t1 = Thread(target=thread_serial, daemon=True)
t2 = Thread(target=thread_camera, daemon=True)

t1.start()
t2.start()

# Mantém o programa rodando
t1.join()
t2.join()
