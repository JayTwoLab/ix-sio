const http = require('http');
const socketIo = require('socket.io');

// 1. Create HTTP server
const server = http.createServer((req, res) => {
    res.writeHead(200);
    res.end('Socket.io 2.x Server is running');
});

// 2. Bind Socket.io to the server (v2.x syntax)
const io = socketIo(server);

// Print the version of socket.io being used
console.log('socket.io version:', require('socket.io/package.json').version);

// 3. Handle connection event
io.on('connection', (socket) => {
    console.log('A new user has connected: ', socket.id);

    // Receive message from client (echo back to sender and broadcast to all)
    socket.on('chat message', (msg) => {
        console.log('Message received:', msg);
        const serverTime = new Date().toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        const socketIoVersion = require('socket.io/package.json').version;
        const messageWithMeta = {
            text: msg,
            serverTime,
            socketIoVersion
        };
        io.emit('chat message', messageWithMeta);
    });

    // On disconnect
    socket.on('disconnect', () => {
        console.log('User disconnected');
    });
	
 
	
});

// 4. Set port and start server
const PORT = 3002;
server.listen(PORT, () => {
    console.log(`Server is running on port ${PORT}.`);
});



