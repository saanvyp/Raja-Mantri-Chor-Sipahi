const express = require('express');
const app = express();
const PORT = 1908;

// Middleware to parse JSON bodies from incoming requests
app.use(express.json());
