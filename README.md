# 608-Final-Project
Gesture Controlled Robot

# Server Credentials
Username: team10
Password: 9086526

# Create Server Connect Script
Add this to your `~/.bashrc` or `~/.zshrc` (or whatever other bash profile gets run at inialization on your laptop):
```
alias 608team="expect -c 'spawn ssh team10@608dev-2.net; expect \"password:\"; send \"9086526\r\"; interact'"
```

# Mac Addresses for ESP Now
Sender (adriano) `60:55:F9:D9:D7:32`

Reciever (natasha using dani's board) `7C:DF:A1:15:3A:14`

(Following this tutorial)
`https://randomnerdtutorials.com/esp-now-one-to-many-esp32-esp8266/`

# Server Interaction
Server: http://608dev-2.net/sandbox/sc/team10/server.py

POST request takes x, y as multipart/form-data
GET request just returns plots of data

# MAC Address

Natasha
7C:DF:A1:15:3A:14
