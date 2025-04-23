import serial
import csv
import threading
import os
from datetime import datetime
import tkinter as tk
from tkinter import ttk, messagebox

# --- Settings ---
BAUD = 9600
DEFAULT_PORT = "COM3"
LOG_DIR = "log"  # New directory for log files

class SerialLoggerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Arduino Logger to CSV")

        self.running = False
        self.ser = None
        self.filename = None
        self.file = None
        self.writer = None

        # GUI layout
        self.port_label = ttk.Label(root, text="COM Port:")
        self.port_label.grid(row=0, column=0, padx=5, pady=5, sticky="e")

        self.port_entry = ttk.Entry(root)
        self.port_entry.insert(0, DEFAULT_PORT)
        self.port_entry.grid(row=0, column=1, padx=5, pady=5)

        self.start_button = ttk.Button(root, text="Start Logging", command=self.start_logging)
        self.start_button.grid(row=1, column=0, padx=5, pady=5)

        self.stop_button = ttk.Button(root, text="Stop", command=self.stop_logging, state=tk.DISABLED)
        self.stop_button.grid(row=1, column=1, padx=5, pady=5)

        self.output_box = tk.Text(root, height=15, width=70)
        self.output_box.grid(row=2, column=0, columnspan=2, padx=5, pady=5)

    def start_logging(self):
        port = self.port_entry.get()
        try:
            self.ser = serial.Serial(port, BAUD, timeout=1)
        except serial.SerialException as e:
            messagebox.showerror("Connection Error", str(e))
            return

        # Ensure log directory exists
        os.makedirs(LOG_DIR, exist_ok=True)

        # Generate full path for the CSV file
        self.filename = os.path.join(LOG_DIR, f"log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")

        # Open the log file
        self.file = open(self.filename, 'w', newline='', encoding='utf-8')
        self.writer = csv.writer(self.file)
        self.writer.writerow(["Timestamp", "Program", "Data1", "Data2"])
        self.file.flush()

        self.running = True
        self.start_button.config(state=tk.DISABLED)
        self.stop_button.config(state=tk.NORMAL)
        self.thread = threading.Thread(target=self.read_serial)
        self.thread.daemon = True
        self.thread.start()

    def stop_logging(self):
        self.running = False
        if self.ser:
            self.ser.close()
        if self.file:
            self.file.close()
        self.start_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)
        messagebox.showinfo("Logging Stopped", f"Data saved to {self.filename}")

    def read_serial(self):
        while self.running:
            try:
                line = self.ser.readline().decode(errors='ignore').strip()
                if line:
                    self.output_box.insert(tk.END, line + '\n')
                    self.output_box.see(tk.END)

                    parts = line.split(',')
                    if len(parts) >= 4 and parts[0] in ("BTN", "REACT"):
                        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        program = parts[0]
                        data1 = parts[2].strip()
                        data2 = parts[3].strip()
                        self.writer.writerow([timestamp, program, data1, data2])
                        self.file.flush()
            except Exception as e:
                print(f"Error: {e}")
                break

# --- Run App ---
if __name__ == "__main__":
    root = tk.Tk()
    app = SerialLoggerApp(root)
    root.mainloop()
