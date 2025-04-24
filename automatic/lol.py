import pyautogui
import pytesseract
from PIL import ImageGrab
import keyboard
import time
import re

# Setează calea către executabilul Tesseract
pytesseract.pytesseract.tesseract_cmd = r'C:\Program Files\Tesseract-OCR\tesseract.exe'

def gaseste_text_pe_ecran(text_cautat):
    # Capturează ecranul
    screenshot = ImageGrab.grab()
    # Extrage textul cu poziții
    data = pytesseract.image_to_data(screenshot, output_type=pytesseract.Output.DICT)

    # Creează un pattern regex (ex: "a)")
    pattern = re.compile(rf"^{re.escape(text_cautat)}", re.IGNORECASE)

    for i, word in enumerate(data["text"]):
        if pattern.match(word.strip()):
            x = data["left"][i]
            y = data["top"][i]
            w = data["width"][i]
            h = data["height"][i]
            return x + w // 2, y + h // 2  # Coordonatele centrului cuvântului
    return None

print("Scriptul rulează. Apasă tasta 'a', 'b' sau 'c' ca să alegi răspunsul.")

while True:
    for tasta in ["a", "b", "c"]:
        if keyboard.is_pressed(tasta):
            varianta = f"{tasta})"
            print(f"Ai ales varianta {varianta}")

            coord = gaseste_text_pe_ecran(varianta)
            if coord:
                pyautogui.click(coord)
                time.sleep(1)
            else:
                print("Nu am găsit pe ecran varianta:", varianta)

            # Găsește și apasă "Check"
            check_btn = gaseste_text_pe_ecran("Check")
            if check_btn:
                pyautogui.click(check_btn)
                time.sleep(1.5)
            else:
                print("Butonul 'Check' nu a fost găsit.")

            # Apasă "Next" de 2 ori
            for i in range(2):
                next_btn = gaseste_text_pe_ecran("Next")
                if next_btn:
                    pyautogui.click(next_btn)
                    time.sleep(1.2)
                else:
                    print(f"Butonul 'Next' nu a fost găsit la pasul {i+1}.")

            # Așteaptă până eliberezi tasta
            while keyboard.is_pressed(tasta):
                time.sleep(0.1)
