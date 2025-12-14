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
