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

app.post('/room/assign/:roomId', (req, res) => {
    const roomId = req.params.roomId;
    const room = rooms[roomId];

    if (!room) return res.status(404).send({ message: "Room not found." });
    if (room.players.length !== 4) return res.status(400).send({ message: "Need exactly 4 players to assign roles." });
    if (room.status !== 'WAITING') return res.status(400).send({ message: "Roles already assigned or game in progress." });

    const roles = ['Raja', 'Mantri', 'Chor', 'Sipahi'];
    const shuffledRoles = shuffleArr([...roles]); 

    const roundRoles = {};
    for (let i = 0; i < room.players.length; i++) {
        const playerId = room.players[i].id;
        const role = shuffledRoles[i];

        // assign the role to the player object for this round
        room.players[i].role = role; 
        roundRoles[playerId] = role;
    }

    room.roundRoles = roundRoles;
    room.status = 'ASSIGNED';

    res.status(200).send({ message: "Roles assigned. Mantri must now guess the Chor." });
});

// GET /role/me/:roomId/:playerId - see your role only
app.get('/role/me/:roomId/:playerId', (req, res) => {
    const { roomId, playerId } = req.params;
    const room = rooms[roomId];

    if (!room) return res.status(404).send({ message: "Room not found." });

    const player = room.players.find(p => p.id === playerId);
    if (!player) return res.status(404).send({ message: "Player not found in room." });
    
    if (room.status === 'WAITING' || !player.role) {
        return res.status(200).send({ message: "Roles have not been assigned yet." });
    }

    // Crucial step: Only reveal the role for the requested player ID
    res.status(200).send({ role: player.role });
});

// POST /guess/:roomId - submit guessed playerId
app.post('/guess/:roomId', (req, res) => {
    const roomId = req.params.roomId;
    const { mantriId, guessedChorId } = req.body;
    const room = rooms[roomId];

    if (!room) {
        return res.status(404).send({ message: "Room not found." });
    }
    if (room.status !== 'ASSIGNED') {
        return res.status(400).send({ message: "Roles not assigned or results already calculated." });
}

// check if the submitting player is the mantri
    const mantriPlayer = room.players.find(p => p.id === mantriId);
    if (!mantriPlayer || mantriPlayer.role !== 'Mantri') {
        return res.status(403).send({ message: "Only the Mantri can submit a guess." });
    }

    // store the guess
    room.mantriGuess = guessedChorId;
    
    // find actual chor
    const actualChorPlayer = room.players.find(p => p.role === 'Chor');
    if (!actualChorPlayer) {
        return res.status(500).send({ message: "Internal error: Chor role not found." });
    }
    
    // determine if the guess was correct
    const isCorrect = (guessedChorId === actualChorPlayer.id);

    // calc and update scores
    let roundPoints = {};

    if (isCorrect) {
        // correct guess: mantri & sipahi keep their points, chor loses.
        room.players.forEach(p => {
            const role = p.role;
            const points = POINTS[role];
            roundPoints[p.id] = points;
            p.cumulativeScore += points;
        });
        roundPoints.result = "Mantri Guessed Correctly!";

    } else {
        // incorrect: chor steals mantri's and sipahi's points.
        const chorStolenPoints = POINTS.Mantri + POINTS.Sipahi;

        room.players.forEach(p => {
            const role = p.role;
            let points = POINTS[role];
            
            if (role === 'Mantri' || role === 'Sipahi') {
                points = 0; // mantri and sipahi lose their default points
            } else if (role === 'Chor') {
                points = chorStolenPoints; // chor steals the points
            } 
            // raja points remain the same

            roundPoints[p.id] = points;
            p.cumulativeScore += points;
        });
        roundPoints.result = "Chor Stole the Points!";
    }

    room.status = 'GUESSED';
    room.lastRoundPoints = roundPoints; // Store points for the result endpoint

    res.status(200).send({
        message: "Guess submitted. Waiting for results reveal.",
        isCorrect: isCorrect,
        mantriGuessedId: guessedChorId
    });
});
