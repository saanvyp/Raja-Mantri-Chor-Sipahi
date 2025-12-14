const express = require('express');
const app = express();
const PORT = 1908;

// Middleware to parse JSON bodies from incoming requests
app.use(express.json());

// store for all rooms and players
const rooms = {};
// defaults
const POINTS = {
    Raja: 1000,
    Mantri: 800,
    Sipahi: 500,
    Chor: 0
};

// Start the server
app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});

// function to generate a unique ID
const generateUniqueId = () => Math.random().toString(36).substring(2, 9);

// POST /room/create - create room & become player 1
app.post('/room/create', (req, res) => {
    // player name in the req body
    const { playerName } = req.body;
    if (!playerName) {
        return res.status(400).send({ message: "Player name is required." });
    }

    const roomId = generateUniqueId();
    const playerId = generateUniqueId();

    const player = {
        id: playerId,
        name: playerName,
        role: null, // Role assigned later
        cumulativeScore: 0,
    };

    rooms[roomId] = {
        id: roomId,
        status: 'WAITING', // WAITING, ASSIGNED, GUESSED, RESULT
        players: [player],
        roundRoles: {},
        mantriGuess: null,
    };

    res.status(201).send({
        message: 'Room created successfully. Waiting for 3 more players.',
        roomId: roomId,
        playerId: playerId,
        playerList: [playerName]
    });
});

// POST /room/join - join existing room
app.post('/room/join', (req, res) => {
    const { roomId, playerName } = req.body;
    const room = rooms[roomId];

    if (!room) {
        return res.status(404).send({ message: "Room not found." });
    }
    if (room.players.length >= 4) {
        return res.status(403).send({ message: "Room is full. Max 4 players." });
    }

    const playerId = generateUniqueId();
    const player = { id: playerId, name: playerName, role: null, cumulativeScore: 0 };
    room.players.push(player);

    const playerNames = room.players.map(p => p.name);

    res.status(200).send({
        message: `${playerName} joined successfully. Players in room: ${room.players.length}`,
        roomId: roomId,
        playerId: playerId,
        playerList: playerNames
    });
});

const shuffleArr = (arr) => {
    for(let i = arr.length - 1; i > 0; i--){
        const j = Math.floor(Math.random()*(i+1));
        [arr[i], arr[j] = arr[j], arr[i]];
    }
    return arr;
};
