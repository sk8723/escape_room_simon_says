import tkinter as tk
import random
import pygame

# -----------------------------
# Game Settings
# -----------------------------

COLORS = ["red", "blue", "yellow", "green"]
COLOR_HEX = {
    "red": "#EA4335",
    "blue": "#4285F4",
    "yellow": "#FBBC05",
    "green": "#34A853"
}
FLASH_COLORS = {
    "red": "#FF9999",
    "blue": "#99BBFF",
    "yellow": "#FFF799",
    "green": "#99FF99"
}
ROUND_TYPES = ["color", "animal", "note"]
ROUND_LENGTHS = {
    "color": 4,
    "animal": 4,
    "note": 4
}
FLASH_TIME = 500
PAUSE_TIME = 800
BUTTON_SIZE = 100
INNER_RATIO = 0.85
SPACING = 30
BROWN = "#8B4513"
COCONUT_EYES_RATIO = 0.03
MAX_IMG_HEIGHT = 60

# -----------------------------
# Main Game Class
# -----------------------------

class SimonGame:

    def __init__(self, root):
        self.root = root
        self.root.title("Simon Says")

        self.sequence = []
        self.user_sequence = []
        self.score = 0
        self.round_index = 0
        self.round_type = ROUND_TYPES[0]
        self.accepting_input = False
        self.try_again = False 

        # UI
        self.label = tk.Label(root, text="Press Start", font=("Arial", 18))
        self.label.pack(pady=10)

        self.start_button = tk.Button(
            root,
            text="Start",
            font=("Arial", 14),
            command=self.start_game
        )
        self.start_button.pack()

        self.canvas_width = len(COLORS) * (BUTTON_SIZE + SPACING)
        self.canvas_height = BUTTON_SIZE + 100
        self.canvas = tk.Canvas(root, width=self.canvas_width, height=self.canvas_height)
        self.canvas.pack(pady=10)

        self.buttons = {}

        # Load Images
        self.images = {}
        for color in COLORS:
            img = tk.PhotoImage(file=f"images/{color}.png")
            if img.height() > MAX_IMG_HEIGHT:
                factor = img.height() // MAX_IMG_HEIGHT
                img = img.subsample(factor, factor)
            self.images[color] = img

        # Sounds
        pygame.mixer.init()
        self.animal_sounds = {color: pygame.mixer.Sound(f"animals/{color}.wav") for color in COLORS}
        self.note_sounds = {color: pygame.mixer.Sound(f"notes/{color}.wav") for color in COLORS}

        self.create_coconut_buttons()

    # -----------------------------
    # Create Buttons
    # -----------------------------

    def create_coconut_buttons(self):
        for index, color in enumerate(COLORS):
            x0 = index * (BUTTON_SIZE + SPACING)
            y0 = 20
            x1 = x0 + BUTTON_SIZE
            y1 = y0 + BUTTON_SIZE

            outer = self.canvas.create_oval(x0, y0, x1, y1, fill=COLOR_HEX[color], outline=COLOR_HEX[color])
            margin = BUTTON_SIZE * (1 - INNER_RATIO) / 2
            inner = self.canvas.create_oval(x0 + margin, y0 + margin, x1 - margin, y1 - margin, fill=BROWN, outline=BROWN)

            eye_radius = BUTTON_SIZE * COCONUT_EYES_RATIO
            cx = (x0 + x1) / 2
            cy = (y0 + y1) / 2
            offset = BUTTON_SIZE * 0.07
            eyes = [
                self.canvas.create_oval(cx - offset - eye_radius, cy - offset - eye_radius, cx - offset + eye_radius, cy - offset + eye_radius, fill="black"),
                self.canvas.create_oval(cx + offset - eye_radius, cy - offset - eye_radius, cx + offset + eye_radius, cy - offset + eye_radius, fill="black"),
                self.canvas.create_oval(cx - eye_radius, cy + offset - eye_radius, cx + eye_radius, cy + offset + eye_radius, fill="black")
            ]

            for item in [outer, inner] + eyes:
                self.canvas.tag_bind(item, "<Button-1>", lambda e, c=color: self.handle_click(c))

            img_x = (x0 + x1) / 2
            img_y = y1 + self.images[color].height() / 2 + 5
            img = self.canvas.create_image(img_x, img_y, image=self.images[color])
            self.canvas.tag_bind(img, "<Button-1>", lambda e, c=color: self.handle_click(c))

            self.buttons[color] = {"outer": outer}

    # -----------------------------
    # Start Game
    # -----------------------------
    
    def start_game(self):
        self.round_index = 0
        self.score = 0
        self.round_type = ROUND_TYPES[0]
        self.label.config(text="Watch the pattern!")
        self.next_round()

    # -----------------------------
    # Next Round
    # -----------------------------

    def next_round(self):
        self.user_sequence = []
        self.round_type = ROUND_TYPES[self.round_index]
        length = ROUND_LENGTHS[self.round_type]

        if self.try_again:
            self.label.config(text=f"Try again! Round {self.round_index + 1}")
        else:
            self.label.config(text=f"Round {self.round_index + 1}: {self.round_type}")
            # Generate new sequence
            self.sequence = []
            for _ in range(length):
                # Choose random color not equal to last
                options = [c for c in COLORS if not self.sequence or c != self.sequence[-1]]
                self.sequence.append(random.choice(options))

        self.root.after(1000, lambda: self.play_sequence(0))

    # -----------------------------
    # Play Sequence
    # -----------------------------

    def play_sequence(self, index):
        if index >= len(self.sequence):
            self.accepting_input = True
            return

        color = self.sequence[index]

        # Flash only for round 1 (color round)
        if self.round_type == "color":
            self.flash_button(color)

        # Play sound for rounds 2 & 3
        if self.round_type == "animal":
            self.animal_sounds[color].play()
        elif self.round_type == "note":
            self.note_sounds[color].play()

        self.root.after(FLASH_TIME + PAUSE_TIME, lambda: self.play_sequence(index + 1))

    # -----------------------------
    # Flash Button
    # -----------------------------

    def flash_button(self, color):
        original = COLOR_HEX[color]
        flash = FLASH_COLORS[color]

        self.canvas.itemconfig(self.buttons[color]["outer"], fill=flash)
        self.root.after(FLASH_TIME, lambda: self.canvas.itemconfig(self.buttons[color]["outer"], fill=original))

    # -----------------------------
    # Handle Click
    # -----------------------------

    def handle_click(self, color):
        if not self.accepting_input:
            return

        # Always flash lights on click
        self.flash_button(color)

        # Play sound if applicable
        if self.round_type == "animal":
            self.animal_sounds[color].play()
        elif self.round_type == "note":
            self.note_sounds[color].play()

        self.user_sequence.append(color)

        # Check user input
        if self.user_sequence[-1] != self.sequence[len(self.user_sequence) - 1]:
            self.accepting_input = False
            self.try_again = True
            self.root.after(500, self.next_round)
            return

        if len(self.user_sequence) == len(self.sequence):
            self.accepting_input = False
            self.try_again = False
            self.round_index += 1
            if self.round_index >= len(ROUND_TYPES):
                self.label.config(text="You won!")
            else:
                self.root.after(1000, self.next_round)

# -----------------------------
# Main Function
# -----------------------------

def main():
    root = tk.Tk()
    game = SimonGame(root)
    root.mainloop()

if __name__ == "__main__":
    main()
