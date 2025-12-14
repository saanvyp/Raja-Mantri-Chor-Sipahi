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
