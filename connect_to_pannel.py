# -*- coding: utf-8 -*-
"""
Created on Mon Oct 24 10:16:24 2022

@author: louis
"""

import socket
hote = "192.168.1.60"
port = 10001
socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
socket.connect((hote, port))
socket.send(b'0001 550000 Bonjour\r\n')
socket.close()
