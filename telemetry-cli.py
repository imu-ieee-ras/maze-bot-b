import subprocess
import time
import argparse
import sys
import os
import csv
from datetime import datetime
import serial

class TelemetryCLI:
    def __init__(self, mac_address=None, baudrate=9600, log_file='surus_verisi.csv'):
        self.mac_address = mac_address
        self.baudrate = baudrate
        self.log_file = log_file
        self.port = '/dev/rfcomm0'
        self.ser = None
        self.running = False
        
    def setup_bluetooth(self):
        print("[*] Setting up Bluetooth connection...")
        
        # 1. Check if device is paired and get MAC if not provided
        if not self.mac_address:
            print("[*] Scanning for paired HC-05 devices...")
            try:
                result = subprocess.run(['bluetoothctl', 'paired-devices'], capture_output=True, text=True)
                for line in result.stdout.split('\n'):
                    if 'HC-05' in line:
                        self.mac_address = line.split()[1]
                        print(f"[+] Found HC-05 at {self.mac_address}")
                        break
            except Exception as e:
                print(f"[-] Error scanning devices: {e}")
                
        if not self.mac_address:
            print("[-] Could not find HC-05 MAC address. Please provide it manually or pair the device.")
            return False

        # 2. Release all ports and kill lingering rfcomm processes
        try:
            subprocess.run(['sudo', 'rfcomm', 'release', 'all'], stderr=subprocess.DEVNULL)
            time.sleep(1.5)
        except:
            pass

        # 3. Bind port (retry up to 3 times if busy)
        print(f"[*] Binding {self.mac_address} to {self.port}...")
        for attempt in range(3):
            res = subprocess.run(['sudo', 'rfcomm', 'bind', '0', self.mac_address], capture_output=True, text=True)
            if os.path.exists(self.port):
                break
            time.sleep(1.5)
        
        if not os.path.exists(self.port):
            print("[-] Port olusturulamadi (Cihaz mesgul). 3 saniye bekleyip tekrar deneyin.")
            return False

        # 4. Grant permissions
        print(f"[*] Granting read/write permissions to {self.port}...")
        try:
            subprocess.run(['sudo', 'chmod', 'a+rw', self.port])
            time.sleep(1)
        except Exception as e:
            print(f"[-] Error granting permissions: {e}")
            return False
            
        print("[+] Bluetooth setup complete.")
        return True

    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"[+] Connected to {self.port} at {self.baudrate} baud.")
            return True
        except serial.SerialException as e:
            print(f"[-] Connection failed: {e}")
            return False
            
    def run(self):
        self.running = True
        
        while self.running:
            if self.ser is None or not self.ser.is_open:
                print("\n[*] Baglanti bekleniyor (Araci acin)...")
                if not self.connect():
                    time.sleep(2)
                    continue
                    
            try:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        self.process_telemetry(line)
            except (serial.SerialException, OSError) as e:
                print("\n[-] Arac kapandi veya Bluetooth koptu. Port kapatiliyor...")
                try:
                    self.ser.close()
                except:
                    pass
                self.ser = None
                time.sleep(2)
            except Exception as e:
                print(f"\n[-] Beklenmeyen hata: {e}")
                time.sleep(1)
                
            time.sleep(0.01)

    def process_telemetry(self, line):
        if "Initialized" in line:
            print(f"\n[*] {line}\n")
            return
            
        try:
            parts = line.split()
            data = {}
            for part in parts:
                if ':' in part:
                    key, value = part.split(':', 1)
                    data[key] = value
            
            if all(k in data for k in ['F', 'L', 'R', 'Yaw', 'Ticks']):
                sys.stdout.write('\r')
                display_str = f"F: {data['F']:>6} cm | L: {data['L']:>6} cm | R: {data['R']:>6} cm | Yaw: {data['Yaw']:>6}° | Ticks (L,R): {data['Ticks']:>10}"
                sys.stdout.write(display_str)
                sys.stdout.flush()
                
                if self.log_file:
                    self.log_data(data)
            else:
                 print(f"\n[RAW] {line}")
                 
        except Exception as e:
            pass

    def log_data(self, data):
        file_exists = os.path.isfile(self.log_file)
        
        try:
            with open(self.log_file, 'a', newline='') as csvfile:
                fieldnames = ['Timestamp', 'Front_cm', 'Left_cm', 'Right_cm', 'Yaw_deg', 'Left_Ticks', 'Right_Ticks']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

                if not file_exists:
                    writer.writeheader()
                
                l_tick, r_tick = 0, 0
                if ',' in data.get('Ticks', ''):
                    l_str, r_str = data['Ticks'].split(',')
                    l_tick, r_tick = int(l_str), int(r_str)

                writer.writerow({
                    'Timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3],
                    'Front_cm': data.get('F', ''),
                    'Left_cm': data.get('L', ''),
                    'Right_cm': data.get('R', ''),
                    'Yaw_deg': data.get('Yaw', ''),
                    'Left_Ticks': l_tick,
                    'Right_Ticks': r_tick
                })
        except Exception as e:
             pass

    def stop(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("\n[+] Connection closed.")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Maze Robot Auto-Bluetooth Telemetry Monitor")
    parser.add_argument('-m', '--mac', help='Specific MAC address of HC-05 (optional, will auto-detect if paired)')
    parser.add_argument('-b', '--baud', type=int, default=9600, help='Baudrate (default: 9600)')
    parser.add_argument('-l', '--log', default='surus_verisi.csv', help='Log file (default: surus_verisi.csv)')
    
    args = parser.parse_args()
    
    print("==================================================")
    print("      Auto-Bluetooth Maze Robot Telemetry         ")
    print("==================================================")
    
    cli = TelemetryCLI(mac_address=args.mac, baudrate=args.baud, log_file=args.log)
    
    try:
        if cli.setup_bluetooth():
            if cli.connect():
                cli.run()
            else:
                print("[-] Could not open serial port after Bluetooth setup.")
    except KeyboardInterrupt:
        cli.stop()
        
        # Cleanup
        print("[*] Cleaning up Bluetooth binding...")
        try:
            subprocess.run(['sudo', 'rfcomm', 'release', '0'], stderr=subprocess.DEVNULL)
        except:
            pass
        sys.exit(0)
