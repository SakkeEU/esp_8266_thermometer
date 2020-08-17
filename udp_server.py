#!/usr/bin/env python3
import sys
import socket
import tkinter as tk
import threading

#temp client, each client creates its own row
class client:
	def __init__(self, addr, data):
		self.addr = tk.StringVar()
		self.addr.set(addr)
		self.data = tk.StringVar()
		self.data.set(data)
		self.to = 5 #timeout
	def addr_get(self):
		return self.addr.get()
	def data_get(self):
		return self.data.get()
	def to_get(self):
		return self.to
	def row_get(self):
		return self.label_addr.grid_info()["row"]
	def data_set(self, data):
		self.data.set(data)
	def to_decrease(self):
		self.to -= 1
	def to_reset(self):
		self.to = 5
	def print_entry(self, r):
		self.label_addr = tk.Label(root, textvariable = self.addr, borderwidth = 2, padx = 20, relief = "sunken", fg="black", bg = "white")
		self.label_addr.grid(row = r, column = 0, pady = 10)
		self.label_data = tk.Label(root, textvariable = self.data, borderwidth = 2, padx = 20, relief = "sunken", fg="black", bg = "white")
		self.label_data.grid(row = r, column = 1, pady = 10)
		
clients = []

#window configs
root = tk.Tk()
root.title("Temp Measurement")
root.resizable(width = False, height = False)
root.geometry("+350+250") #declaring only the position makes the window size dynamic
root.grid_columnconfigure((0, 1), weight = 1)
def close_bind(event):
	server_t_kill.set()
	root.destroy()
def close_x():
	server_t_kill.set()
	root.destroy()
root.protocol("WM_DELETE_WINDOW", close_x)
root.bind("<Escape>", close_bind)
root.update()

#server configs
sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
#get local ip
host_name = socket.gethostname()
UDP_IP = socket.gethostbyname(host_name + ".local")
UDP_PORT = 5005

sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(1)

def server_loop(server_kill):
	while not server_kill.is_set():
		try:
			data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
			data = data[:-3]
			#check if the client is known and add it to the list
			if len(clients) == 0:
				clients.append(client(addr[0], data))
			else:
				known_client = False
				for c in clients:
					if c.addr_get() == addr[0]:
						c.data_set(data)
						c.to_reset()
						known_client = True
						break
				if known_client == False:
					clients.append(client(addr[0], data))
			#create the grid
			c_row = 0
			for c in clients:
				c.print_entry(c_row)
				c_row += 1
			root.update()
		#each server timeout the to of the clients is decreased by 1
		except socket.timeout:
			#decrease every to
			for c in clients:
				c.to_decrease()
			#check if a client needs to be removed
			c_index = 0
			removed = False #remove only one client for each timeout call
			for c in clients:
				if c.to_get() <= 0:
					r = c.row_get()
					clients.pop(c_index)
					l = list(root.grid_slaves(row = r))
					removed = True
					for w in l:
						w.destroy()
					break
				c_index += 1
			#if a client has been removed clear grid and redraw
			if removed == True:
				for c in clients:
					r = c.row_get()
					l = list(root.grid_slaves(row = r))
					for w in l:
						w.destroy()
			root.update()
			#repopulate the grid
			#not really necessary, removes half a second of blank window
			if removed == True:
				c_row = 0
				for c in clients:
					c.print_entry(c_row)
					c_row += 1
				root.update()
			continue

server_t_kill = threading.Event()
server_t = threading.Thread(target = server_loop, args = [server_t_kill])
server_t.start()

root.mainloop()
