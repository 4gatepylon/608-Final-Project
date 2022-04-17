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

# Server Interaction
Server: http://608dev-2.net/sandbox/sc/team10/server.py

POST request takes x, y as multipart/form-data
GET request just returns plots of data
