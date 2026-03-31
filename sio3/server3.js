const express = require('express');
const { createServer } = require('http'); // Node.js built-in http module
const { Server } = require('socket.io');

const app = express();
const httpServer = createServer(app);

// Socket.io 3.x Configuration (CORS settings are mandatory)
const io = new Server(httpServer, {
  cors: {
    origin: "http://localhost:8080", // Client app URL (use "*" to allow all)
    methods: ["GET", "POST"],
    credentials: true
  }
});

io.on("connection", (socket) => {
  console.log(`User connected: ${socket.id}`);

  // Receive a specific event
  socket.on("message", (data) => {
      console.log("Message received from client:", data);

      const serverTime = new Date().toISOString();

      const path = require('path');
      const socketIoPkg = require(path.join(__dirname, 'node_modules', 'socket.io', 'package.json'));
      const socketIoVersion = socketIoPkg.version;    

      socket.emit("server-info", {
        serverTime,
        socketIoVersion
      });

      socket.disconnect(); // Disconnect after sending the response
  });

  socket.on("disconnect", () => {
    // clearInterval(intervalId);
    console.log("User disconnected");
  });
  
});

const PORT = 3004;
const { exec } = require('child_process');
httpServer.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}.`);
  // Print socket.io version safely
  exec('npm list socket.io --depth=0', (err, stdout, stderr) => {
    if (err) {
      console.log('Could not determine socket.io version.');
    } else {
      const match = stdout.match(/socket\.io@(\S+)/);
      if (match) {
        console.log(`socket.io version: ${match[1]}`);
      } else {
        console.log('socket.io version not found.');
      }
    }
  });
});
